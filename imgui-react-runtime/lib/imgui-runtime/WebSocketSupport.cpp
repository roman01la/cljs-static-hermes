#include "WebSocketSupport.h"

#include <libwebsockets.h>

#include <hermes/VM/static_h.h>
#include <jsi/jsi.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{

    constexpr const char *kProtocolName = "imgui-runtime-websocket";

    constexpr const char *kInstallWebSocketCtorScript = R"JS(
(function(factory) {
    function WebSocket(url) {
        const instance = factory(url);
        if (instance && typeof instance === 'object') {
            try {
                Object.setPrototypeOf(instance, WebSocket.prototype);
            } catch (e) {
                // Ignore if prototype assignment fails.
            }
        }
        return instance;
    }
    WebSocket.CONNECTING = 0;
    WebSocket.OPEN = 1;
    WebSocket.CLOSING = 2;
    WebSocket.CLOSED = 3;
    WebSocket.prototype = Object.create(null);
    WebSocket.prototype.constructor = WebSocket;
    WebSocket.prototype.addEventListener = function () {};
    WebSocket.prototype.removeEventListener = function () {};
    WebSocket.prototype.dispatchEvent = function () { return false; };
    return WebSocket;
})
)JS";

    struct ParsedUrl
    {
        std::string address;
        std::string path;
        std::string hostHeader;
        int port = 0;
        bool secure = false;
    };

    ParsedUrl parseUrl(const std::string &url)
    {
        // Lightweight parser that accepts ws:// URLs (wss is blocked earlier).
        ParsedUrl result;
        int defaultPort = 0;
        std::string rest;
        if (url.rfind("ws://", 0) == 0)
        {
            result.secure = false;
            defaultPort = 80;
            rest = url.substr(5);
        }
        else if (url.rfind("wss://", 0) == 0)
        {
            result.secure = true;
            defaultPort = 443;
            rest = url.substr(6);
        }
        else
        {
            throw std::runtime_error("WebSocket URL must start with ws:// or wss://");
        }

        auto slashPos = rest.find('/');
        std::string authority = slashPos == std::string::npos ? rest : rest.substr(0, slashPos);
        result.path = slashPos == std::string::npos ? "/" : rest.substr(slashPos);

        if (authority.empty())
        {
            throw std::runtime_error("WebSocket URL missing host");
        }

        auto colonPos = authority.rfind(':');
        if (colonPos != std::string::npos)
        {
            result.address = authority.substr(0, colonPos);
            std::string portStr = authority.substr(colonPos + 1);
            if (portStr.empty())
            {
                throw std::runtime_error("WebSocket URL has empty port");
            }
            try
            {
                result.port = std::stoi(portStr);
            }
            catch (const std::exception &)
            {
                throw std::runtime_error("WebSocket URL has invalid port");
            }
            result.hostHeader = authority;
        }
        else
        {
            result.address = authority;
            result.port = defaultPort;
            result.hostHeader = authority;
        }

        if (result.address.empty())
        {
            throw std::runtime_error("WebSocket URL has invalid host");
        }

        if (result.path.empty())
        {
            result.path = "/";
        }

        return result;
    }

    int websocketCallback(struct lws *wsi, enum lws_callback_reasons reason,
                          void *user, void *in, size_t len);

    const lws_protocols kProtocols[] = {
        {kProtocolName, websocketCallback, 0, 0, 0, nullptr, 0},
        {nullptr, nullptr, 0, 0, 0, nullptr, 0}};

    class WebSocketInstance;
    class WebSocketHostObject;

    class WebSocketManager
    {
    public:
        static WebSocketManager &get();

        void initialize(facebook::hermes::HermesRuntime &runtime);
        void shutdown();
        void pump();
        std::shared_ptr<WebSocketInstance> createInstance(const std::string &url);

        struct lws_context *context() const { return context_; }

    private:
        WebSocketManager() = default;

        void cleanupExpired();

        facebook::hermes::HermesRuntime *runtime_ = nullptr;
        struct lws_context *context_ = nullptr;
        std::vector<std::weak_ptr<WebSocketInstance>> instances_;
    };

    // Represents a single JS-visible WebSocket backed by libwebsockets.
    class WebSocketInstance : public std::enable_shared_from_this<WebSocketInstance>
    {
    public:
        enum class ReadyState
        {
            Connecting = 0,
            Open = 1,
            Closing = 2,
            Closed = 3
        };

        WebSocketInstance(facebook::hermes::HermesRuntime &runtime, std::string url)
            : runtime_(runtime), jsRuntime_(static_cast<facebook::jsi::Runtime &>(runtime)),
              url_(std::move(url)) {}

        void connect()
        {
            parsed_ = parseUrl(url_);

            if (parsed_.secure)
            {
                throw facebook::jsi::JSError(jsRuntime_, "wss:// URLs are not supported in this build");
            }

            if (!WebSocketManager::get().context())
            {
                throw facebook::jsi::JSError(jsRuntime_, "WebSocket context unavailable");
            }

            lws_client_connect_info info;
            std::memset(&info, 0, sizeof(info));
            info.context = WebSocketManager::get().context();
            info.address = parsed_.address.c_str();
            info.host = parsed_.hostHeader.c_str();
            info.origin = parsed_.hostHeader.c_str();
            info.port = parsed_.port;
            info.path = parsed_.path.c_str();
            info.protocol = kProtocolName;
            info.pwsi = &wsi_;

            wsi_ = lws_client_connect_via_info(&info);
            if (!wsi_)
            {
                throw facebook::jsi::JSError(jsRuntime_, "Failed to initiate WebSocket connection");
            }

            lws_set_opaque_user_data(wsi_, this);
        }

        void handleConnected(struct lws *wsi)
        {
            wsi_ = wsi;
            state_ = ReadyState::Open;
            dispatch(onOpen_, {});
            if (!outbound_.empty())
            {
                lws_callback_on_writable(wsi_);
            }
        }

        void handleMessage(const void *data, size_t len)
        {
            if (!data || !len)
            {
                return;
            }
            std::string payload(static_cast<const char *>(data), len);
            dispatchMessageToJs(payload);
        }

        void handleWritable()
        {
            if (!wsi_ || outbound_.empty())
            {
                return;
            }

            auto &buffer = outbound_.front();
            int written = lws_write(wsi_, buffer.data() + LWS_PRE,
                                    buffer.size() - LWS_PRE, LWS_WRITE_TEXT);
            if (written < 0)
            {
                handleError("WebSocket write failed");
                return;
            }

            outbound_.pop_front();
            if (!outbound_.empty())
            {
                lws_callback_on_writable(wsi_);
            }
        }

        void handlePeerInitiatedClose(uint16_t code, const std::string &reason)
        {
            peerInitiatedClose_ = true;
            closeCode_ = code;
            closeReason_ = reason;
            state_ = ReadyState::Closing;
        }

        void handleClosed()
        {
            if (state_ == ReadyState::Closed)
            {
                return;
            }

            wsi_ = nullptr;
            bool wasClean = peerInitiatedClose_ || state_ == ReadyState::Closing;
            transitionToClosed(closeCode_, closeReason_, wasClean);
        }

        void handleError(const std::string &message)
        {
            dispatchErrorToJs(message.empty() ? "WebSocket connection error" : message);
            transitionToClosed(1006, message, false);
        }

        void send(facebook::jsi::Runtime &rt, const facebook::jsi::Value &value)
        {
            if (!value.isString())
            {
                throw facebook::jsi::JSError(rt, "WebSocket.send currently supports only string data");
            }

            if (state_ != ReadyState::Open && state_ != ReadyState::Connecting)
            {
                throw facebook::jsi::JSError(rt, "WebSocket is not open");
            }

            std::string data = value.asString(rt).utf8(rt);
            std::vector<unsigned char> buffer(LWS_PRE + data.size());
            if (!data.empty())
            {
                std::memcpy(buffer.data() + LWS_PRE, data.data(), data.size());
            }
            outbound_.push_back(std::move(buffer));
            if (wsi_)
            {
                lws_callback_on_writable(wsi_);
            }
        }

        void close(uint16_t code, const std::string &reason)
        {
            if (state_ == ReadyState::Closed || state_ == ReadyState::Closing)
            {
                return;
            }

            if (code < 1000 || code > 4999)
            {
                code = 1000;
            }

            closeCode_ = code;
            closeReason_ = reason;
            state_ = ReadyState::Closing;
            peerInitiatedClose_ = false;

            if (wsi_)
            {
                std::array<unsigned char, 125> reasonBuffer{};
                size_t copyLen = std::min(reason.size(), reasonBuffer.size());
                if (copyLen > 0)
                {
                    std::memcpy(reasonBuffer.data(), reason.data(), copyLen);
                }
                lws_close_reason(wsi_, static_cast<enum lws_close_status>(code), reasonBuffer.data(), copyLen);
                lws_callback_on_writable(wsi_);
            }
            else
            {
                transitionToClosed(code, reason, true);
            }
        }

        facebook::jsi::Value getOnOpen(facebook::jsi::Runtime &rt) const
        {
            if (!onOpen_)
            {
                return facebook::jsi::Value::undefined();
            }
            return facebook::jsi::Value(rt, *onOpen_);
        }

        facebook::jsi::Value getOnMessage(facebook::jsi::Runtime &rt) const
        {
            if (!onMessage_)
            {
                return facebook::jsi::Value::undefined();
            }
            return facebook::jsi::Value(rt, *onMessage_);
        }

        facebook::jsi::Value getOnClose(facebook::jsi::Runtime &rt) const
        {
            if (!onClose_)
            {
                return facebook::jsi::Value::undefined();
            }
            return facebook::jsi::Value(rt, *onClose_);
        }

        facebook::jsi::Value getOnError(facebook::jsi::Runtime &rt) const
        {
            if (!onError_)
            {
                return facebook::jsi::Value::undefined();
            }
            return facebook::jsi::Value(rt, *onError_);
        }

        void setOnOpen(facebook::jsi::Runtime &rt, const facebook::jsi::Value &value)
        {
            setHandler(rt, value, onOpen_, "onopen");
        }

        void setOnMessage(facebook::jsi::Runtime &rt, const facebook::jsi::Value &value)
        {
            setHandler(rt, value, onMessage_, "onmessage");
        }

        void setOnClose(facebook::jsi::Runtime &rt, const facebook::jsi::Value &value)
        {
            setHandler(rt, value, onClose_, "onclose");
        }

        void setOnError(facebook::jsi::Runtime &rt, const facebook::jsi::Value &value)
        {
            setHandler(rt, value, onError_, "onerror");
        }

        int readyStateAsInt() const { return static_cast<int>(state_); }
        const std::string &url() const { return url_; }

    private:
        void setHandler(facebook::jsi::Runtime &rt, const facebook::jsi::Value &value,
                        std::unique_ptr<facebook::jsi::Function> &slot, const char *name)
        {
            if (value.isUndefined() || value.isNull())
            {
                slot.reset();
                return;
            }

            if (!value.isObject() || !value.asObject(rt).isFunction(rt))
            {
                std::string message = std::string(name) + " must be a function";
                throw facebook::jsi::JSError(rt, message.c_str());
            }

            slot = std::make_unique<facebook::jsi::Function>(value.asObject(rt).asFunction(rt));
        }
        void dispatch(std::unique_ptr<facebook::jsi::Function> &handler,
                      std::vector<facebook::jsi::Value> &&args)
        {
            if (!handler)
            {
                return;
            }

            try
            {
                const facebook::jsi::Value *argv = args.empty() ? nullptr : args.data();
                handler->call(jsRuntime_, argv, args.size());
            }
            catch (facebook::jsi::JSIException &e)
            {
                std::printf("WebSocket handler exception: %s\n", e.what());
            }
        }
        void dispatchMessageToJs(const std::string &message)
        {
            if (!onMessage_)
            {
                return;
            }

            facebook::jsi::Object event(jsRuntime_);
            event.setProperty(jsRuntime_, "data",
                              facebook::jsi::String::createFromUtf8(jsRuntime_, message));

            std::vector<facebook::jsi::Value> args;
            args.emplace_back(jsRuntime_, event);
            dispatch(onMessage_, std::move(args));
        }

        void dispatchCloseToJs(uint16_t code, const std::string &reason, bool wasClean)
        {
            if (!onClose_)
            {
                closeDispatched_ = true;
                return;
            }

            facebook::jsi::Object event(jsRuntime_);
            event.setProperty(jsRuntime_, "code", facebook::jsi::Value(code));
            event.setProperty(jsRuntime_, "reason",
                              facebook::jsi::String::createFromUtf8(jsRuntime_, reason));
            event.setProperty(jsRuntime_, "wasClean", facebook::jsi::Value(wasClean));

            std::vector<facebook::jsi::Value> args;
            args.emplace_back(jsRuntime_, event);
            dispatch(onClose_, std::move(args));
            closeDispatched_ = true;
        }

        void dispatchErrorToJs(const std::string &message)
        {
            if (!onError_)
            {
                return;
            }

            std::vector<facebook::jsi::Value> args;
            auto str = facebook::jsi::String::createFromUtf8(jsRuntime_, message);
            args.emplace_back(jsRuntime_, str);
            dispatch(onError_, std::move(args));
        }

        void transitionToClosed(uint16_t code, const std::string &reason, bool wasClean)
        {
            if (state_ == ReadyState::Closed)
            {
                return;
            }

            state_ = ReadyState::Closed;
            dispatchCloseToJs(code, reason, wasClean);
        }

        facebook::hermes::HermesRuntime &runtime_;
        facebook::jsi::Runtime &jsRuntime_;
        std::string url_;
        ParsedUrl parsed_;
        ReadyState state_ = ReadyState::Connecting;
        struct lws *wsi_ = nullptr;
        std::deque<std::vector<unsigned char>> outbound_;
        std::unique_ptr<facebook::jsi::Function> onOpen_;
        std::unique_ptr<facebook::jsi::Function> onMessage_;
        std::unique_ptr<facebook::jsi::Function> onClose_;
        std::unique_ptr<facebook::jsi::Function> onError_;
        std::string closeReason_;
        uint16_t closeCode_ = 1000;
        bool peerInitiatedClose_ = false;
        bool closeDispatched_ = false;
    };

    class WebSocketHostObject : public facebook::jsi::HostObject
    {
    public:
        explicit WebSocketHostObject(std::shared_ptr<WebSocketInstance> instance)
            : instance_(std::move(instance)) {}

        facebook::jsi::Value get(facebook::jsi::Runtime &rt,
                                 const facebook::jsi::PropNameID &name) override
        {
            auto prop = name.utf8(rt);

            if (prop == "readyState")
            {
                return facebook::jsi::Value(static_cast<double>(instance_->readyStateAsInt()));
            }
            if (prop == "url")
            {
                auto urlString = facebook::jsi::String::createFromUtf8(rt, instance_->url());
                return facebook::jsi::Value(rt, urlString);
            }
            if (prop == "onopen")
            {
                return instance_->getOnOpen(rt);
            }
            if (prop == "onmessage")
            {
                return instance_->getOnMessage(rt);
            }
            if (prop == "onclose")
            {
                return instance_->getOnClose(rt);
            }
            if (prop == "onerror")
            {
                return instance_->getOnError(rt);
            }
            if (prop == "send")
            {
                auto weak = std::weak_ptr<WebSocketInstance>(instance_);
                return facebook::jsi::Function::createFromHostFunction(
                    rt, facebook::jsi::PropNameID::forAscii(rt, "send"), 1,
                    [weak](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
                           const facebook::jsi::Value *args, size_t count) -> facebook::jsi::Value
                    {
                        auto shared = weak.lock();
                        if (!shared)
                        {
                            throw facebook::jsi::JSError(runtime, "WebSocket instance is closed");
                        }
                        if (count < 1)
                        {
                            throw facebook::jsi::JSError(runtime, "WebSocket.send requires at least one argument");
                        }
                        shared->send(runtime, args[0]);
                        return facebook::jsi::Value::undefined();
                    });
            }
            if (prop == "close")
            {
                auto weak = std::weak_ptr<WebSocketInstance>(instance_);
                return facebook::jsi::Function::createFromHostFunction(
                    rt, facebook::jsi::PropNameID::forAscii(rt, "close"), 0,
                    [weak](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
                           const facebook::jsi::Value *args, size_t count) -> facebook::jsi::Value
                    {
                        auto shared = weak.lock();
                        if (!shared)
                        {
                            return facebook::jsi::Value::undefined();
                        }

                        uint16_t code = 1000;
                        std::string reason;
                        if (count >= 1 && args[0].isNumber())
                        {
                            code = static_cast<uint16_t>(args[0].asNumber());
                        }
                        if (count >= 2 && args[1].isString())
                        {
                            reason = args[1].asString(runtime).utf8(runtime);
                        }
                        shared->close(code, reason);
                        return facebook::jsi::Value::undefined();
                    });
            }

            return facebook::jsi::Value::undefined();
        }

        void set(facebook::jsi::Runtime &rt, const facebook::jsi::PropNameID &name,
                 const facebook::jsi::Value &value) override
        {
            auto prop = name.utf8(rt);

            if (prop == "onopen")
            {
                instance_->setOnOpen(rt, value);
            }
            else if (prop == "onmessage")
            {
                instance_->setOnMessage(rt, value);
            }
            else if (prop == "onclose")
            {
                instance_->setOnClose(rt, value);
            }
            else if (prop == "onerror")
            {
                instance_->setOnError(rt, value);
            }
        }

        std::vector<facebook::jsi::PropNameID> getPropertyNames(facebook::jsi::Runtime &rt) override
        {
            std::vector<facebook::jsi::PropNameID> result;
            result.emplace_back(facebook::jsi::PropNameID::forAscii(rt, "readyState"));
            result.emplace_back(facebook::jsi::PropNameID::forAscii(rt, "url"));
            result.emplace_back(facebook::jsi::PropNameID::forAscii(rt, "onopen"));
            result.emplace_back(facebook::jsi::PropNameID::forAscii(rt, "onmessage"));
            result.emplace_back(facebook::jsi::PropNameID::forAscii(rt, "onclose"));
            result.emplace_back(facebook::jsi::PropNameID::forAscii(rt, "onerror"));
            result.emplace_back(facebook::jsi::PropNameID::forAscii(rt, "send"));
            result.emplace_back(facebook::jsi::PropNameID::forAscii(rt, "close"));
            return result;
        }

    private:
        std::shared_ptr<WebSocketInstance> instance_;
    };

    WebSocketManager &WebSocketManager::get()
    {
        static WebSocketManager instance;
        return instance;
    }

    void WebSocketManager::initialize(facebook::hermes::HermesRuntime &runtime)
    {
        if (context_)
        {
            return;
        }

        runtime_ = &runtime;
        auto &jsRuntime = static_cast<facebook::jsi::Runtime &>(runtime);

        lws_context_creation_info info;
        std::memset(&info, 0, sizeof(info));
        info.port = CONTEXT_PORT_NO_LISTEN;
        info.protocols = kProtocols;
        info.options = 0;
        info.pt_serv_buf_size = 16 * 1024 * 1024 + LWS_PRE;

        context_ = lws_create_context(&info);

        auto factory = facebook::jsi::Function::createFromHostFunction(
            jsRuntime, facebook::jsi::PropNameID::forAscii(jsRuntime, "__createNativeWebSocket"), 1,
            [](facebook::jsi::Runtime &rt, const facebook::jsi::Value &, const facebook::jsi::Value *args,
               size_t count) -> facebook::jsi::Value
            {
                if (count < 1 || !args[0].isString())
                {
                    throw facebook::jsi::JSError(rt, "WebSocket constructor expects a URL string");
                }

                std::string url = args[0].asString(rt).utf8(rt);
                auto instance = WebSocketManager::get().createInstance(url);
                auto hostObject = std::make_shared<WebSocketHostObject>(instance);
                return facebook::jsi::Object::createFromHostObject(
                    rt, std::static_pointer_cast<facebook::jsi::HostObject>(hostObject));
            });

        auto global = jsRuntime.global();
        auto installer = jsRuntime
                             .evaluateJavaScript(
                                 std::make_shared<facebook::jsi::StringBuffer>(kInstallWebSocketCtorScript),
                                 "websocket_ctor.js")
                             .asObject(jsRuntime)
                             .asFunction(jsRuntime);
        auto ctorValue = installer.call(jsRuntime, factory);
        global.setProperty(jsRuntime, "WebSocket", ctorValue);
    }

    void WebSocketManager::shutdown()
    {
        if (!context_)
        {
            runtime_ = nullptr;
            instances_.clear();
            return;
        }

        for (auto &weak : instances_)
        {
            if (auto shared = weak.lock())
            {
                shared->close(1001, "runtime shutdown");
            }
        }

        lws_context_destroy(context_);
        context_ = nullptr;

        cleanupExpired();
        instances_.clear();

        if (runtime_)
        {
            auto &jsRuntime = static_cast<facebook::jsi::Runtime &>(*runtime_);
            auto global = jsRuntime.global();
            if (global.hasProperty(jsRuntime, "WebSocket"))
            {
                global.setProperty(jsRuntime, "WebSocket", facebook::jsi::Value());
            }
        }

        runtime_ = nullptr;
    }

    void WebSocketManager::pump()
    {
        if (!context_)
        {
            return;
        }

        // Use a non-blocking pump to avoid stalling the main thread; libwebsockets treats
        // negative timeouts as "do not wait" while still processing pending work.
        lws_service(context_, -1);
        cleanupExpired();
    }

    std::shared_ptr<WebSocketInstance> WebSocketManager::createInstance(const std::string &url)
    {
        if (!runtime_)
        {
            throw std::runtime_error("WebSocket runtime is not initialized");
        }

        auto instance = std::make_shared<WebSocketInstance>(*runtime_, url);
        try
        {
            instance->connect();
        }
        catch (const facebook::jsi::JSIException &)
        {
            throw;
        }
        catch (const std::exception &e)
        {
            auto &jsRuntime = static_cast<facebook::jsi::Runtime &>(*runtime_);
            throw facebook::jsi::JSError(jsRuntime, e.what());
        }
        instances_.push_back(instance);
        return instance;
    }

    void WebSocketManager::cleanupExpired()
    {
        instances_.erase(
            std::remove_if(instances_.begin(), instances_.end(),
                           [](const std::weak_ptr<WebSocketInstance> &w)
                           { return w.expired(); }),
            instances_.end());
    }

    int websocketCallback(struct lws *wsi, enum lws_callback_reasons reason,
                          void *user, void *in, size_t len)
    {
        (void)user;
        auto *instance = static_cast<WebSocketInstance *>(lws_get_opaque_user_data(wsi));

        if (!instance)
        {
            return 0;
        }

        switch (reason)
        {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            instance->handleConnected(wsi);
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
            instance->handleMessage(in, len);
            break;
        case LWS_CALLBACK_CLIENT_WRITEABLE:
            instance->handleWritable();
            break;
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        {
            std::string message;
            if (in && len)
            {
                message.assign(static_cast<const char *>(in), len);
            }
            instance->handleError(message);
            break;
        }
        case LWS_CALLBACK_CLIENT_CLOSED:
            instance->handleClosed();
            break;
        case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
        {
            uint16_t code = 1005;
            std::string reasonText;
            if (in && len >= 2)
            {
                const unsigned char *data = static_cast<const unsigned char *>(in);
                code = static_cast<uint16_t>(data[0] << 8 | data[1]);
                if (len > 2)
                {
                    reasonText.assign(reinterpret_cast<const char *>(data + 2), len - 2);
                }
            }
            instance->handlePeerInitiatedClose(code, reasonText);
            break;
        }
        default:
            break;
        }

        return 0;
    }

} // namespace

void initializeWebSocketSupport(facebook::hermes::HermesRuntime &runtime)
{
    WebSocketManager::get().initialize(runtime);
}

void pumpWebSocketSupport() { WebSocketManager::get().pump(); }

void shutdownWebSocketSupport() { WebSocketManager::get().shutdown(); }
