#define GL_SILENCE_DEPRECATION

#include <GLFW/glfw3.h>
#include <hermes/VM/static_h.h>

#include <cstdint>
#include <iostream>
#include <cstring>

#include "WebSocketSupport.h"
#include "PersistentVector.h"
#include "PersistentMap.h"
#include "MappedFileBuffer.h"

#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/GrBackendSurface.h"

#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkColorSpace.h"

#include "include/core/SkFontMgr.h"
#include "include/core/SkFont.h"
#include "include/ports/SkFontMgr_directory.h"

// OpenGL headers
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

// Define GL_FRAMEBUFFER_SRGB if not available
#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif

class HermesApp
{
public:
    std::unique_ptr<SHRuntime, decltype(&_sh_done)> shRuntime;
    facebook::hermes::HermesRuntime *hermes = nullptr;
    facebook::jsi::Function peekMacroTask;
    facebook::jsi::Function runMacroTask;
    facebook::jsi::Function flushRaf;
    facebook::jsi::Function onEvent;

    HermesApp(SHRuntime *shr, facebook::jsi::Function &&peek,
              facebook::jsi::Function &&run, facebook::jsi::Function &&flushRaf,
              facebook::jsi::Function &&onEvent)
        : shRuntime(shr, &_sh_done), hermes(_sh_get_hermes_runtime(shr)),
          peekMacroTask(std::move(peek)), runMacroTask(std::move(run)),
          flushRaf(std::move(flushRaf)), onEvent(std::move(onEvent)) {}

    // Delete copy/move to ensure singleton behavior
    HermesApp(const HermesApp &) = delete;
    HermesApp &operator=(const HermesApp &) = delete;
    HermesApp(HermesApp &&) = delete;
    HermesApp &operator=(HermesApp &&) = delete;
};

static HermesApp *s_hermesApp = nullptr;

GrDirectContext *sContext = nullptr;
SkSurface *sSurface = nullptr;
SkCanvas *canvas = nullptr;
sk_sp<SkFontMgr> fontMgr = nullptr;
int sWindowWidth = 0;
int sWindowHeight = 0;
float sDpiScale = 1.0f;

float calculateDpiScale(GLFWwindow *window)
{
    int fbWidth, fbHeight;
    int winWidth, winHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glfwGetWindowSize(window, &winWidth, &winHeight);

    int scale;

    if (winWidth > 0 && fbWidth > 0)
    {
        scale = (float)fbWidth / (float)winWidth;
    }
    else
    {
        scale = 1.0f;
    }

    s_hermesApp->hermes->global().setProperty(*s_hermesApp->hermes,
                                              "devicePixelRatio",
                                              facebook::jsi::Value(scale));

    return scale;
}

void init_skia(int w, int h)
{
    sWindowWidth = w;
    sWindowHeight = h;

    auto interface = GrGLMakeNativeInterface();
    sContext = GrDirectContexts::MakeGL(interface).release();

    GrGLFramebufferInfo framebufferInfo;
    framebufferInfo.fFBOID = 0;
    framebufferInfo.fFormat = GL_RGBA8;

    SkColorType colorType = kRGBA_8888_SkColorType;
    // Enable 4x MSAA for antialiasing
    auto backendRenderTarget = GrBackendRenderTargets::MakeGL(w, h, 4, 8, framebufferInfo);

    sSurface = SkSurfaces::WrapBackendRenderTarget(sContext, backendRenderTarget, kBottomLeft_GrSurfaceOrigin, colorType, SkColorSpace::MakeSRGB(), nullptr).release();
}

void recreate_skia_surface(int w, int h)
{
    if (sSurface)
    {
        sSurface->unref();
        sSurface = nullptr;
    }

    sWindowWidth = w;
    sWindowHeight = h;

    glViewport(0, 0, w, h);

    GrGLFramebufferInfo framebufferInfo;
    framebufferInfo.fFBOID = 0;
    framebufferInfo.fFormat = GL_RGBA8;

    SkColorType colorType = kRGBA_8888_SkColorType;
    // Enable 4x MSAA for antialiasing
    auto backendRenderTarget = GrBackendRenderTargets::MakeGL(w, h, 4, 8, framebufferInfo);

    sSurface = SkSurfaces::WrapBackendRenderTarget(sContext, backendRenderTarget, kBottomLeft_GrSurfaceOrigin, colorType, SkColorSpace::MakeSRGB(), nullptr).release();
    canvas = sSurface->getCanvas();
}

void error_callback(int error, const char *description)
{
    fputs(description, stderr);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    s_hermesApp->onEvent.call(*s_hermesApp->hermes,
                              facebook::jsi::String::createFromUtf8(*s_hermesApp->hermes, "keydown"),
                              facebook::jsi::Value(key),
                              facebook::jsi::Value(scancode),
                              facebook::jsi::Value(action),
                              facebook::jsi::Value(mods));

    s_hermesApp->hermes->drainMicrotasks();
}

void cursor_position_callback(GLFWwindow *window, double xpos, double ypos)
{
    // Scale mouse coordinates by DPI
    double scaledX = xpos * sDpiScale;
    double scaledY = ypos * sDpiScale;

    s_hermesApp->onEvent.call(*s_hermesApp->hermes,
                              facebook::jsi::String::createFromUtf8(*s_hermesApp->hermes, "mousemove"),
                              facebook::jsi::Value(scaledX),
                              facebook::jsi::Value(scaledY));
    s_hermesApp->hermes->drainMicrotasks();
}
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    s_hermesApp->onEvent.call(*s_hermesApp->hermes,
                              facebook::jsi::String::createFromUtf8(*s_hermesApp->hermes, "mousebutton"),
                              facebook::jsi::Value(button),
                              facebook::jsi::Value(action),
                              facebook::jsi::Value(mods));
    s_hermesApp->hermes->drainMicrotasks();
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    s_hermesApp->onEvent.call(*s_hermesApp->hermes,
                              facebook::jsi::String::createFromUtf8(*s_hermesApp->hermes, "scroll"),
                              facebook::jsi::Value(xoffset),
                              facebook::jsi::Value(yoffset));
    s_hermesApp->hermes->drainMicrotasks();
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    if (width > 0 && height > 0)
    {
        recreate_skia_surface(width, height);
    }
}

void window_size_callback(GLFWwindow *window, int width, int height)
{
    // Get actual framebuffer size (accounts for retina displays)
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

    // Update DPI scale
    sDpiScale = calculateDpiScale(window);

    if (fbWidth > 0 && fbHeight > 0)
    {
        recreate_skia_surface(fbWidth, fbHeight);
    }
}

const int kWidth = 960;
const int kHeight = 640;

extern "C" SHUnit *sh_export_jslib(void);
extern "C" SHUnit *sh_export_skia(void);

#if REACT_BUNDLE_MODE == 0
extern "C" SHUnit *sh_export_react(void);
#elif !(REACT_BUNDLE_MODE >= 0 && REACT_BUNDLE_MODE < 4)
#error "Invalid REACT_BUNDLE_MODE"
#endif

void load_unit(facebook::hermes::HermesRuntime *hermes,
               SHUnitCreator nativeUnit, bool bytecode,
               const char *jsPath, const char *sourceURL)
{
    if (nativeUnit)
    {
        hermes->evaluateSHUnit(nativeUnit);
        printf("Native unit loaded.\n");
    }

    if (jsPath && bytecode)
    {
        // Mode 1: Bytecode - load .hbc file via evaluateJavaScript
        printf("Loading React unit from bytecode: '%s'\n", jsPath);
        auto buffer = mapFileBuffer(jsPath, false);
        hermes->evaluateJavaScript(buffer, sourceURL ? sourceURL : jsPath);
        printf("React unit loaded (bytecode).\n");
    }
    else if (jsPath && !bytecode)
    {
        // Mode 2: Source - load .js file with source map
        printf("Loading React unit from source: '%s'\n", jsPath);
        auto buffer = mapFileBuffer(jsPath, true);

        // Try to load source map (bundle path + ".map")
        std::string sourceMapPath = std::string(jsPath) + ".map";
        std::shared_ptr<const facebook::jsi::Buffer> sourceMapBuf;
        bool hasSourceMap = false;
        try
        {
            sourceMapBuf = mapFileBuffer(sourceMapPath.c_str(), true);
            printf("Loaded source map: '%s'\n", sourceMapPath.c_str());
            hasSourceMap = true;
        }
        catch (const std::exception &e)
        {
            printf("Source map not found: %s\n", e.what());
        }

        // Evaluate JavaScript with or without source map
        if (hasSourceMap)
        {
            hermes->evaluateJavaScriptWithSourceMap(buffer, sourceMapBuf,
                                                    sourceURL ? sourceURL : jsPath);
        }
        else
        {
            hermes->evaluateJavaScript(buffer, sourceURL ? sourceURL : jsPath);
        }
        printf("React unit loaded (source).\n");
    }
}

template <int BUNDLE_MODE>
void main_default(facebook::hermes::HermesRuntime *hermes,
                  SHUnitCreator sh_export_react, const char *bundlePath)
{
    // Load react unit based on compilation mode
    if constexpr (BUNDLE_MODE == 0)
    {
        load_unit(hermes, sh_export_react, false, nullptr, nullptr);
    }
    else if constexpr (BUNDLE_MODE == 1)
    {
        // Mode 1: Bytecode - load .hbc file via evaluateJavaScript
        load_unit(hermes, nullptr, true, bundlePath, "react-unit-bundle.hbc");
    }
    else if constexpr (BUNDLE_MODE == 2)
    {
        // Mode 2: Source - load .js file with source map
        load_unit(hermes, nullptr, false, bundlePath, "react-unit-bundle.js");
    }
}

int main(void)
{
    GLFWwindow *window;
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
    {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_DEPTH_BITS, 0);
    glfwWindowHint(GLFW_SAMPLES, 4); // Enable 4x MSAA

    window = glfwCreateWindow(kWidth, kHeight, "Simple example", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    glEnable(GL_FRAMEBUFFER_SRGB);

    // Get framebuffer size (accounts for retina displays)
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

    auto runtimeConfig = ::hermes::vm::RuntimeConfig::Builder()
                             .withMicrotaskQueue(true)
                             .withES6BlockScoping(true)
                             .build();
    SHRuntime *shr = _sh_init(runtimeConfig);
    facebook::hermes::HermesRuntime *hermes = _sh_get_hermes_runtime(shr);
    // Load jslib unit first to set up event loop and extract helper functions
    // from jslib result
    facebook::jsi::Object helpers =
        hermes->evaluateSHUnit(sh_export_jslib).asObject(*hermes);

    // Set NODE_ENV based on build configuration
#ifdef NDEBUG
    const char *nodeEnv = "production";
#else
    const char *nodeEnv = "development";
#endif
    hermes->global()
        .getPropertyAsObject(*hermes, "process")
        .getPropertyAsObject(*hermes, "env")
        .setProperty(*hermes, "NODE_ENV", nodeEnv);

    hermes->evaluateSHUnit(sh_export_skia);

    s_hermesApp =
        new HermesApp(shr, helpers.getPropertyAsFunction(*hermes, "peek"),
                      helpers.getPropertyAsFunction(*hermes, "run"),
                      helpers.getPropertyAsFunction(*hermes, "flushRaf"),
                      hermes->global()
                          .getPropertyAsFunction(*hermes, "on_event"));

    // Calculate and store DPI scale
    sDpiScale = calculateDpiScale(window);

    initializeWebSocketSupport(*s_hermesApp->hermes);

    // Install ClojureScript native data structures
    cljs::installPersistentVector(*s_hermesApp->hermes);
    cljs::installPersistentMap(*s_hermesApp->hermes);

    // Initialize jslib's current time
    double curTimeMs = glfwGetTime() * 1000.0;
    s_hermesApp->runMacroTask.call(*s_hermesApp->hermes, curTimeMs);

    // Add performance.now() host function using Sokol time
    auto perf = facebook::jsi::Object(*s_hermesApp->hermes);
    perf.setProperty(
        *s_hermesApp->hermes, "now",
        facebook::jsi::Function::createFromHostFunction(
            *s_hermesApp->hermes,
            facebook::jsi::PropNameID::forAscii(*s_hermesApp->hermes, "now"), 0,
            [](facebook::jsi::Runtime &, const facebook::jsi::Value &,
               const facebook::jsi::Value *,
               size_t) -> facebook::jsi::Value
            { return glfwGetTime() * 1000.0; }));
    s_hermesApp->hermes->global().setProperty(*s_hermesApp->hermes,
                                              "performance", perf);

#if REACT_BUNDLE_MODE != 0
    static constexpr SHUnit *(*sh_export_react)(void) = nullptr;
#endif
    main_default<REACT_BUNDLE_MODE>(hermes, sh_export_react,
                                    REACT_BUNDLE_PATH);

    init_skia(fbWidth, fbHeight);

    glfwSwapInterval(1);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);

    canvas = sSurface->getCanvas();

    // Call on_init callback
    s_hermesApp->hermes->global().getPropertyAsFunction(*s_hermesApp->hermes, "on_init").call(*s_hermesApp->hermes);
    s_hermesApp->hermes->drainMicrotasks();

    // Apply sappConfig settings to GLFW window
    auto sappConfig = s_hermesApp->hermes->global().getPropertyAsObject(*s_hermesApp->hermes, "sappConfig");

    auto title = sappConfig.getProperty(*s_hermesApp->hermes, "title").asString(*s_hermesApp->hermes).utf8(*s_hermesApp->hermes);
    glfwSetWindowTitle(window, title.c_str());

    int configWidth = (int)sappConfig.getProperty(*s_hermesApp->hermes, "width").asNumber();
    int configHeight = (int)sappConfig.getProperty(*s_hermesApp->hermes, "height").asNumber();

    glfwSetWindowSize(window, configWidth, configHeight);

    auto onFrame = s_hermesApp->hermes->global().getPropertyAsFunction(*s_hermesApp->hermes, "on_frame");

    SkPaint paint;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        double curTimeMs = glfwGetTime() * 1000.0;

        pumpWebSocketSupport();

        // Run all ready macrotasks before rendering frame
        double nextTimeMs;
        while ((nextTimeMs = s_hermesApp->peekMacroTask.call(*s_hermesApp->hermes)
                                 .getNumber()) >= 0 &&
               nextTimeMs <= curTimeMs)
        {
            s_hermesApp->runMacroTask.call(*s_hermesApp->hermes, curTimeMs);
            s_hermesApp->hermes->drainMicrotasks();
        }

        s_hermesApp->flushRaf.call(*s_hermesApp->hermes, curTimeMs);

        paint.setColor(SK_ColorWHITE);
        canvas->drawPaint(paint);

        onFrame.call(*s_hermesApp->hermes, facebook::jsi::Value(sWindowWidth), facebook::jsi::Value(sWindowHeight), facebook::jsi::Value(curTimeMs));

        s_hermesApp->hermes->drainMicrotasks();

        sContext->flush();

        glfwSwapBuffers(window);
    }

    // Call on_exit callback to clean up resources
    try
    {
        s_hermesApp->hermes->global().getPropertyAsFunction(*s_hermesApp->hermes, "on_exit").call(*s_hermesApp->hermes);
        s_hermesApp->hermes->drainMicrotasks();
    }
    catch (...)
    {
        // Ignore errors during cleanup
    }

    _sh_done(shr);
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}