// Copyright (c) Tzvetan Mikov and contributors
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text

#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <string>

namespace cljs
{

    /**
     * Optimized value storage: primitives are stored directly, objects wrapped in
     * shared_ptr. This reduces allocation overhead for common cases (numbers,
     * booleans, strings) while maintaining proper lifecycle for object references.
     */
    struct StoredValue
    {
        enum Type
        {
            NIL,
            BOOL,
            NUMBER,
            STRING,
            OBJECT_REF
        } type;

        union
        {
            bool bool_val;
            double number_val;
        } primitive;

        // Shared pointers for types requiring complex lifecycle management
        std::shared_ptr<std::string> string_val;
        std::shared_ptr<facebook::jsi::Object> object_val;

        StoredValue() : type(NIL) {}
        explicit StoredValue(bool v) : type(BOOL) { primitive.bool_val = v; }
        explicit StoredValue(double v) : type(NUMBER) { primitive.number_val = v; }
        explicit StoredValue(std::string s) : type(STRING),
                                              string_val(std::make_shared<std::string>(std::move(s))) {}
        explicit StoredValue(std::shared_ptr<facebook::jsi::Object> obj)
            : type(OBJECT_REF), object_val(std::move(obj)) {}

        // Equality operators required by immer for comparison
        bool operator==(const StoredValue &other) const
        {
            if (type != other.type)
                return false;

            switch (type)
            {
            case NIL:
                return true;
            case BOOL:
                return primitive.bool_val == other.primitive.bool_val;
            case NUMBER:
                return primitive.number_val == other.primitive.number_val;
            case STRING:
                if ((string_val == nullptr) != (other.string_val == nullptr))
                    return false;
                if (string_val == nullptr)
                    return true;
                return *string_val == *other.string_val;
            case OBJECT_REF:
                return object_val.get() == other.object_val.get();
            }
            return false;
        }

        bool operator!=(const StoredValue &other) const
        {
            return !(*this == other);
        }

        bool operator<(const StoredValue &other) const
        {
            if (type != other.type)
                return type < other.type;

            switch (type)
            {
            case NIL:
                return false;
            case BOOL:
                return primitive.bool_val < other.primitive.bool_val;
            case NUMBER:
                return primitive.number_val < other.primitive.number_val;
            case STRING:
                if (string_val && other.string_val)
                    return *string_val < *other.string_val;
                return string_val.get() < other.string_val.get();
            case OBJECT_REF:
                return object_val.get() < other.object_val.get();
            }
            return false;
        }
    };

    // Hash function for StoredValue to support use as map/set keys in immer
    inline size_t hash_value(const StoredValue &v) noexcept
    {
        std::hash<double> hash_double;
        std::hash<bool> hash_bool;
        std::hash<std::string> hash_string;
        std::hash<void *> hash_ptr;

        // Combine type and value hash
        size_t h = std::hash<int>{}(static_cast<int>(v.type));

        switch (v.type)
        {
        case StoredValue::NIL:
            return h;
        case StoredValue::BOOL:
            return h ^ (hash_bool(v.primitive.bool_val) << 1);
        case StoredValue::NUMBER:
            return h ^ (hash_double(v.primitive.number_val) << 1);
        case StoredValue::STRING:
            return h ^ (v.string_val ? hash_string(*v.string_val) << 1 : 0);
        case StoredValue::OBJECT_REF:
            return h ^ (hash_ptr(v.object_val.get()) << 1);
        }
        return h;
    }

} // namespace cljs

namespace std
{
    template <>
    struct hash<cljs::StoredValue>
    {
        size_t operator()(const cljs::StoredValue &v) const noexcept
        {
            return cljs::hash_value(v);
        }
    };
} // namespace std
