// Copyright (c) Tzvetan Mikov and contributors
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text

#include "PersistentMap.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

void logToConsole(facebook::jsi::Runtime &rt, const std::string &message)
{
    auto log = rt.global().getProperty(rt, "print").getObject(rt).asFunction(rt);
    log.call(rt, facebook::jsi::String::createFromUtf8(rt, message));
}

namespace cljs
{

    PersistentMapHostObject::PersistentMapHostObject(MapType map)
        : map_(std::move(map)) {}

    std::shared_ptr<PersistentMapHostObject> PersistentMapHostObject::empty()
    {
        return std::make_shared<PersistentMapHostObject>();
    }

    std::shared_ptr<PersistentMapHostObject>
    PersistentMapHostObject::fromObject(facebook::jsi::Runtime &rt,
                                        const facebook::jsi::Object &obj)
    {
        auto map = MapType{};
        auto keys = obj.getPropertyNames(rt);
        for (size_t i = 0; i < keys.size(rt); ++i)
        {
            auto keyStr = keys.getValueAtIndex(rt, i).asString(rt).utf8(rt);
            auto value = obj.getProperty(rt, keyStr.c_str());
            map = map.insert({StoredValue(keyStr), convertValue(rt, value)});
        }
        return std::make_shared<PersistentMapHostObject>(map);
    }

    facebook::jsi::Value
    PersistentMapHostObject::get(facebook::jsi::Runtime &rt,
                                 const facebook::jsi::PropNameID &name)
    {
        std::string prop = name.utf8(rt);

        if (prop == "size" || prop == "length")
        {
            return facebook::jsi::Value(static_cast<double>(size()));
        }

        return facebook::jsi::Value::undefined();
    }

    void PersistentMapHostObject::set(facebook::jsi::Runtime &rt,
                                      const facebook::jsi::PropNameID &name,
                                      const facebook::jsi::Value &value)
    {
    }

    std::vector<facebook::jsi::PropNameID>
    PersistentMapHostObject::getPropertyNames(facebook::jsi::Runtime &rt)
    {
        std::vector<facebook::jsi::PropNameID> result;
        return result;
    }

    size_t PersistentMapHostObject::size() const { return map_.size(); }

    // Helper function to find a key using value-based equivalence (for objects like keywords)
    // Returns the stored key if found, or optional empty if not found
    std::optional<StoredValue> PersistentMapHostObject::findKeyByEquivalence(
        facebook::jsi::Runtime &rt, const facebook::jsi::Value &searchKey) const
    {
        auto searchStored = convertValue(rt, searchKey);

        // First try direct lookup (works for primitives)
        if (map_.count(searchStored) != 0)
        {
            return searchStored;
        }

        // If not found by direct equality, search by equivalence for objects
        if (searchKey.isObject())
        {
            for (const auto &entry : map_)
            {
                // Only compare object keys with object keys
                if (entry.first.type == StoredValue::OBJECT_REF && entry.first.object_val)
                {
                    try
                    {
                        auto storedKeyObj = facebook::jsi::Value(rt, *entry.first.object_val);

                        // Use ClojureScript's equiv if available
                        if (storedKeyObj.isObject())
                        {

                            auto keyObjImpl = storedKeyObj.getObject(rt);
                            // Check if the object has an equiv method
                            if (keyObjImpl.hasProperty(rt, "equiv"))
                            {
                                auto equivProp = keyObjImpl.getProperty(rt, "equiv");
                                if (equivProp.isObject() && equivProp.asObject(rt).isFunction(rt))
                                {

                                    auto equivFn = equivProp.asObject(rt).asFunction(rt);
                                    auto result = equivFn.callWithThis(rt, keyObjImpl, searchKey);
                                    if (result.isBool() && result.getBool())
                                    {
                                        return entry.first;
                                    }
                                }
                            }
                        }
                    }
                    catch (const std::exception &)
                    {
                        // Ignore errors during equiv check and continue
                    }
                }
            }
        }

        return std::nullopt;
    }

    facebook::jsi::Value PersistentMapHostObject::get(facebook::jsi::Runtime &rt,
                                                      const facebook::jsi::Value &key) const
    {
        auto storedKey = convertValue(rt, key);
        if (map_.count(storedKey) == 0)
        {
            return facebook::jsi::Value::undefined();
        }
        return reconstructValue(rt, map_[storedKey]);
    }

    bool PersistentMapHostObject::has(facebook::jsi::Runtime &rt, const facebook::jsi::Value &key) const
    {
        auto storedKey = convertValue(rt, key);
        return map_.count(storedKey) != 0;
    }

    bool PersistentMapHostObject::equiv(facebook::jsi::Runtime &rt,
                                        const facebook::jsi::Value &other) const
    {
        if (!other.isObject())
        {
            return false;
        }
        auto otherObj = other.getObject(rt);
        auto otherHostObj = otherObj.getHostObject<PersistentMapHostObject>(rt);
        if (!otherHostObj)
        {
            return false;
        }

        // Check sizes first
        if (map_.size() != otherHostObj->map_.size())
        {
            return false;
        }

        // Check all entries using equivalence-based lookup
        for (const auto &entry : map_)
        {
            // Reconstruct the key to use equivalence-based lookup
            facebook::jsi::Value keyValue = reconstructValue(rt, entry.first);
            if (auto otherFoundKey = otherHostObj->findKeyByEquivalence(rt, keyValue))
            {
                // Key found in other map, check if values are equivalent
                auto otherValue = otherHostObj->map_.at(*otherFoundKey);
                if (!(entry.second == otherValue))
                {
                    return false;
                }
            }
            else
            {
                // Key not found in other map
                return false;
            }
        }

        return true;
    }

    std::shared_ptr<PersistentMapHostObject>
    PersistentMapHostObject::assoc(facebook::jsi::Runtime &rt,
                                   const facebook::jsi::Value &key,
                                   const facebook::jsi::Value &value) const
    {
        auto storedKey = convertValue(rt, key);
        auto newMap = map_.insert({storedKey, convertValue(rt, value)});
        return std::make_shared<PersistentMapHostObject>(newMap);
    }
    std::shared_ptr<PersistentMapHostObject>
    PersistentMapHostObject::dissoc(facebook::jsi::Runtime &rt, const facebook::jsi::Value &key) const
    {
        auto storedKey = convertValue(rt, key);
        auto newMap = map_.erase(storedKey);
        return std::make_shared<PersistentMapHostObject>(newMap);
    }

    bool PersistentMapHostObject::isEmpty() const { return map_.empty(); }

    facebook::jsi::Object PersistentMapHostObject::toObject(facebook::jsi::Runtime &rt) const
    {
        facebook::jsi::Object result(rt);
        for (const auto &entry : map_)
        {
            // Only include string keys in the object
            if (entry.first.type == StoredValue::STRING && entry.first.string_val)
            {
                result.setProperty(rt, entry.first.string_val->c_str(), reconstructValue(rt, entry.second));
            }
        }
        return result;
    }

    facebook::jsi::Array PersistentMapHostObject::keys(facebook::jsi::Runtime &rt) const
    {
        facebook::jsi::Array result(rt, map_.size());
        size_t i = 0;
        for (const auto &entry : map_)
        {
            result.setValueAtIndex(rt, i++, reconstructValue(rt, entry.first));
        }
        return result;
    }

    facebook::jsi::Array PersistentMapHostObject::values(facebook::jsi::Runtime &rt) const
    {
        facebook::jsi::Array result(rt, map_.size());
        size_t i = 0;
        for (const auto &entry : map_)
        {
            result.setValueAtIndex(rt, i++, reconstructValue(rt, entry.second));
        }
        return result;
    }

    facebook::jsi::Array PersistentMapHostObject::entries(facebook::jsi::Runtime &rt) const
    {
        facebook::jsi::Array result(rt, map_.size());
        size_t i = 0;
        for (const auto &entry : map_)
        {
            facebook::jsi::Array pair(rt, 2);
            pair.setValueAtIndex(rt, 0, reconstructValue(rt, entry.first));
            pair.setValueAtIndex(rt, 1, reconstructValue(rt, entry.second));
            result.setValueAtIndex(rt, i++, pair);
        }
        return result;
    }

    facebook::jsi::Value
    PersistentMapHostObject::reduce(facebook::jsi::Runtime &rt,
                                    const facebook::jsi::Function &fn,
                                    const facebook::jsi::Value &initialValue) const
    {
        facebook::jsi::Value accumulator = fn.call(rt, initialValue);
        for (const auto &entry : map_)
        {
            facebook::jsi::Value keyValue = reconstructValue(rt, entry.first);
            facebook::jsi::Value valValue = reconstructValue(rt, entry.second);
            accumulator = fn.call(rt, accumulator, valValue, keyValue);
        }
        return accumulator;
    }

    StoredValue PersistentMapHostObject::convertValue(facebook::jsi::Runtime &rt,
                                                      const facebook::jsi::Value &value)
    {
        if (value.isUndefined() || value.isNull())
        {
            return StoredValue();
        }
        if (value.isBool())
        {
            return StoredValue(value.getBool());
        }
        if (value.isNumber())
        {
            return StoredValue(value.asNumber());
        }
        if (value.isString())
        {
            return StoredValue(value.asString(rt).utf8(rt));
        }
        if (value.isObject())
        {
            auto obj = std::make_shared<facebook::jsi::Object>(value.getObject(rt));
            return StoredValue(obj);
        }
        return StoredValue();
    }

    facebook::jsi::Value PersistentMapHostObject::reconstructValue(facebook::jsi::Runtime &rt,
                                                                   const StoredValue &stored)
    {
        switch (stored.type)
        {
        case StoredValue::NIL:
            return facebook::jsi::Value::null();
        case StoredValue::BOOL:
            return facebook::jsi::Value(stored.primitive.bool_val);
        case StoredValue::NUMBER:
            return facebook::jsi::Value(stored.primitive.number_val);
        case StoredValue::STRING:
            if (stored.string_val)
            {
                return facebook::jsi::String::createFromUtf8(rt, *stored.string_val);
            }
            return facebook::jsi::Value::null();
        case StoredValue::OBJECT_REF:
            if (stored.object_val)
            {
                return facebook::jsi::Value(rt, *stored.object_val);
            }
            return facebook::jsi::Value::null();
        }
        return facebook::jsi::Value::null();
    }

    void installPersistentMap(facebook::jsi::Runtime &rt)
    {
        // Create the PersistentMap factory object
        facebook::jsi::Object factory(rt);

        // PersistentMap.empty() - Create an empty map
        factory.setProperty(
            rt, "empty",
            facebook::jsi::Function::createFromHostFunction(
                rt, facebook::jsi::PropNameID::forAscii(rt, "empty"), 0,
                [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
                   const facebook::jsi::Value *,
                   size_t) -> facebook::jsi::Value
                {
                    auto map = PersistentMapHostObject::empty();
                    return facebook::jsi::Object::createFromHostObject(runtime, map);
                }));

        // PersistentMap.from(object) - Create a map from a JavaScript object
        factory.setProperty(
            rt, "from",
            facebook::jsi::Function::createFromHostFunction(
                rt, facebook::jsi::PropNameID::forAscii(rt, "from"), 1,
                [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
                   const facebook::jsi::Value *args,
                   size_t count) -> facebook::jsi::Value
                {
                    if (count < 1 || !args[0].isObject())
                    {
                        throw facebook::jsi::JSError(
                            runtime, "PersistentMap.from requires an object argument");
                    }
                    facebook::jsi::Object obj = args[0].getObject(runtime);
                    auto map = PersistentMapHostObject::fromObject(runtime, obj);
                    return facebook::jsi::Object::createFromHostObject(runtime, map);
                }));

        // PersistentMap.assoc(map, key, value)
        factory.setProperty(
            rt, "assoc",
            facebook::jsi::Function::createFromHostFunction(
                rt, facebook::jsi::PropNameID::forAscii(rt, "assoc"), 3,
                [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
                   const facebook::jsi::Value *args,
                   size_t count) -> facebook::jsi::Value
                {
                    auto map = args[0].getObject(runtime).getHostObject<PersistentMapHostObject>(runtime);
                    if (!map)
                    {
                        throw facebook::jsi::JSError(runtime,
                                                     "PersistentMap instance is invalid");
                    }
                    if (count < 3)
                    {
                        throw facebook::jsi::JSError(
                            runtime, "assoc requires a map, key, and value");
                    }
                    auto newMap = map->assoc(runtime, args[1], args[2]);
                    return facebook::jsi::Object::createFromHostObject(runtime, newMap);
                }));

        // PersistentMap.dissoc(map, key)
        factory.setProperty(
            rt, "dissoc",
            facebook::jsi::Function::createFromHostFunction(
                rt, facebook::jsi::PropNameID::forAscii(rt, "dissoc"), 2,
                [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
                   const facebook::jsi::Value *args,
                   size_t count) -> facebook::jsi::Value
                {
                    auto map = args[0].getObject(runtime).getHostObject<PersistentMapHostObject>(runtime);
                    if (!map)
                    {
                        throw facebook::jsi::JSError(runtime,
                                                     "PersistentMap instance is invalid");
                    }
                    if (count < 2)
                    {
                        throw facebook::jsi::JSError(
                            runtime, "dissoc requires a map and key");
                    }
                    auto newMap = map->dissoc(runtime, args[1]);
                    return facebook::jsi::Object::createFromHostObject(runtime, newMap);
                }));

        // PersistentMap.get(map, key)
        factory.setProperty(
            rt, "get",
            facebook::jsi::Function::createFromHostFunction(
                rt, facebook::jsi::PropNameID::forAscii(rt, "get"), 2,
                [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
                   const facebook::jsi::Value *args,
                   size_t count) -> facebook::jsi::Value
                {
                    auto map = args[0].getObject(runtime).getHostObject<PersistentMapHostObject>(runtime);
                    if (!map)
                    {
                        throw facebook::jsi::JSError(runtime,
                                                     "PersistentMap instance is invalid");
                    }
                    if (count < 2)
                    {
                        throw facebook::jsi::JSError(
                            runtime, "get requires a map and key");
                    }
                    return map->get(runtime, args[1]);
                }));

        // PersistentMap.has(map, key)
        factory.setProperty(
            rt, "has",
            facebook::jsi::Function::createFromHostFunction(
                rt, facebook::jsi::PropNameID::forAscii(rt, "has"), 2,
                [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
                   const facebook::jsi::Value *args,
                   size_t count) -> facebook::jsi::Value
                {
                    auto map = args[0].getObject(runtime).getHostObject<PersistentMapHostObject>(runtime);
                    if (!map)
                    {
                        throw facebook::jsi::JSError(runtime,
                                                     "PersistentMap instance is invalid");
                    }
                    if (count < 2)
                    {
                        throw facebook::jsi::JSError(
                            runtime, "has requires a map and key");
                    }
                    return facebook::jsi::Value(map->has(runtime, args[1]));
                }));

        // PersistentMap.equiv(map1, map2)
        factory.setProperty(
            rt, "equiv",
            facebook::jsi::Function::createFromHostFunction(
                rt, facebook::jsi::PropNameID::forAscii(rt, "equiv"), 2,
                [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
                   const facebook::jsi::Value *args,
                   size_t count) -> facebook::jsi::Value
                {
                    if (count < 2 || !args[0].isObject())
                    {
                        throw facebook::jsi::JSError(
                            runtime, "equiv requires two map arguments");
                    }
                    auto map = args[0].getObject(runtime).getHostObject<PersistentMapHostObject>(runtime);
                    if (!map)
                    {
                        throw facebook::jsi::JSError(runtime,
                                                     "First argument must be a PersistentMap instance");
                    }
                    bool isEqual = map->equiv(runtime, args[1]);
                    return facebook::jsi::Value(isEqual);
                }));

        // PersistentMap.isEmpty(map)
        factory.setProperty(
            rt, "isEmpty",
            facebook::jsi::Function::createFromHostFunction(
                rt, facebook::jsi::PropNameID::forAscii(rt, "isEmpty"), 1,
                [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
                   const facebook::jsi::Value *args,
                   size_t count) -> facebook::jsi::Value
                {
                    auto map = args[0].getObject(runtime).getHostObject<PersistentMapHostObject>(runtime);
                    if (!map)
                    {
                        throw facebook::jsi::JSError(runtime,
                                                     "PersistentMap instance is invalid");
                    }
                    return facebook::jsi::Value(map->isEmpty());
                }));

        // PersistentMap.toObject(map)
        factory.setProperty(
            rt, "toObject",
            facebook::jsi::Function::createFromHostFunction(
                rt, facebook::jsi::PropNameID::forAscii(rt, "toObject"), 1,
                [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
                   const facebook::jsi::Value *args,
                   size_t count) -> facebook::jsi::Value
                {
                    auto map = args[0].getObject(runtime).getHostObject<PersistentMapHostObject>(runtime);
                    if (!map)
                    {
                        throw facebook::jsi::JSError(runtime,
                                                     "PersistentMap instance is invalid");
                    }
                    return map->toObject(runtime);
                }));

        // PersistentMap.keys(map)
        factory.setProperty(
            rt, "keys",
            facebook::jsi::Function::createFromHostFunction(
                rt, facebook::jsi::PropNameID::forAscii(rt, "keys"), 1,
                [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
                   const facebook::jsi::Value *args,
                   size_t count) -> facebook::jsi::Value
                {
                    auto map = args[0].getObject(runtime).getHostObject<PersistentMapHostObject>(runtime);
                    if (!map)
                    {
                        throw facebook::jsi::JSError(runtime,
                                                     "PersistentMap instance is invalid");
                    }
                    return map->keys(runtime);
                }));

        // PersistentMap.values(map)
        factory.setProperty(
            rt, "values",
            facebook::jsi::Function::createFromHostFunction(
                rt, facebook::jsi::PropNameID::forAscii(rt, "values"), 1,
                [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
                   const facebook::jsi::Value *args,
                   size_t count) -> facebook::jsi::Value
                {
                    auto map = args[0].getObject(runtime).getHostObject<PersistentMapHostObject>(runtime);
                    if (!map)
                    {
                        throw facebook::jsi::JSError(runtime,
                                                     "PersistentMap instance is invalid");
                    }
                    return map->values(runtime);
                }));

        // PersistentMap.entries(map)
        factory.setProperty(
            rt, "entries",
            facebook::jsi::Function::createFromHostFunction(
                rt, facebook::jsi::PropNameID::forAscii(rt, "entries"), 1,
                [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
                   const facebook::jsi::Value *args,
                   size_t count) -> facebook::jsi::Value
                {
                    auto map = args[0].getObject(runtime).getHostObject<PersistentMapHostObject>(runtime);
                    if (!map)
                    {
                        throw facebook::jsi::JSError(runtime,
                                                     "PersistentMap instance is invalid");
                    }
                    return map->entries(runtime);
                }));

        // PersistentMap.reduce(map, fn, initialValue)
        factory.setProperty(
            rt, "reduce",
            facebook::jsi::Function::createFromHostFunction(
                rt, facebook::jsi::PropNameID::forAscii(rt, "reduce"), 3,
                [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
                   const facebook::jsi::Value *args,
                   size_t count) -> facebook::jsi::Value
                {
                    auto map = args[0].getObject(runtime).getHostObject<PersistentMapHostObject>(runtime);
                    if (!map)
                    {
                        throw facebook::jsi::JSError(runtime,
                                                     "PersistentMap instance is invalid");
                    }
                    if (count < 3 || !args[1].isObject())
                    {
                        throw facebook::jsi::JSError(runtime,
                                                     "reduce requires a map, function, and initial value");
                    }
                    auto fn = args[1].getObject(runtime).asFunction(runtime);
                    return map->reduce(runtime, fn, args[2]);
                }));

        // Install the factory object as globalThis.PersistentMap
        rt.global()
            .setProperty(rt, "PersistentMap", factory);
    }

} // namespace cljs
