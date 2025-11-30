// Copyright (c) Tzvetan Mikov and contributors
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text

#pragma once

#include <hermes/hermes.h>
#include <jsi/jsi.h>

#include <immer/map.hpp>

#include "StoredValue.h"

#include <memory>
#include <optional>
#include <string>
#include <variant>

namespace cljs
{
    /**
     * PersistentMapHostObject wraps an immer::map to provide
     * a ClojureScript-compatible persistent map implementation.
     *
     * Supports arbitrary values as keys (like Clojure maps), not just strings.
     *
     * Operations:
     * - size() - Returns the number of key-value pairs
     * - get(key) - Returns the value for the given key
     * - has(key) - Returns true if the key exists
     * - assoc(key, value) - Returns a new map with the key-value pair added/updated
     * - dissoc(key) - Returns a new map with the key removed
     * - empty() - Returns true if the map is empty
     * - toObject() - Converts to a JavaScript object (only for string keys)
     * - keys() - Returns an array of all keys
     * - values() - Returns an array of all values
     * - entries() - Returns an array of [key, value] pairs
     */
    class PersistentMapHostObject
        : public facebook::jsi::HostObject,
          public std::enable_shared_from_this<PersistentMapHostObject>
    {
    public:
        using MapType = immer::map<StoredValue, StoredValue>;

        // Constructors
        PersistentMapHostObject() = default;
        explicit PersistentMapHostObject(MapType map);

        // Factory methods
        static std::shared_ptr<PersistentMapHostObject> empty();
        static std::shared_ptr<PersistentMapHostObject>
        fromObject(facebook::jsi::Runtime &rt, const facebook::jsi::Object &obj);

        // HostObject interface
        facebook::jsi::Value get(facebook::jsi::Runtime &rt,
                                 const facebook::jsi::PropNameID &name) override;
        void set(facebook::jsi::Runtime &rt, const facebook::jsi::PropNameID &name,
                 const facebook::jsi::Value &value) override;
        std::vector<facebook::jsi::PropNameID>
        getPropertyNames(facebook::jsi::Runtime &rt) override;

        // Map operations
        size_t size() const;
        facebook::jsi::Value get(facebook::jsi::Runtime &rt, const facebook::jsi::Value &key) const;
        bool has(facebook::jsi::Runtime &rt, const facebook::jsi::Value &key) const;
        bool equiv(facebook::jsi::Runtime &rt, const facebook::jsi::Value &other) const;
        std::shared_ptr<PersistentMapHostObject>
        assoc(facebook::jsi::Runtime &rt, const facebook::jsi::Value &key,
              const facebook::jsi::Value &value) const;
        std::shared_ptr<PersistentMapHostObject> dissoc(facebook::jsi::Runtime &rt, const facebook::jsi::Value &key) const;
        bool isEmpty() const;
        facebook::jsi::Object toObject(facebook::jsi::Runtime &rt) const;
        facebook::jsi::Array keys(facebook::jsi::Runtime &rt) const;
        facebook::jsi::Array values(facebook::jsi::Runtime &rt) const;
        facebook::jsi::Array entries(facebook::jsi::Runtime &rt) const;

        // High-performance reduce for iteration-heavy operations
        facebook::jsi::Value
        reduce(facebook::jsi::Runtime &rt, const facebook::jsi::Function &fn,
               const facebook::jsi::Value &initialValue) const;

        // Access the underlying map (for testing/debugging)
        const MapType &getMap() const { return map_; }

    private:
        // Helper to convert jsi::Value to StoredValue for storage
        static StoredValue convertValue(facebook::jsi::Runtime &rt,
                                        const facebook::jsi::Value &value);

        // Helper to convert StoredValue back to jsi::Value
        static facebook::jsi::Value reconstructValue(facebook::jsi::Runtime &rt,
                                                     const StoredValue &stored);

        // Helper to find a key using value-based equivalence (for object keys like keywords)
        // Returns the actual stored key if found, or std::nullopt if not found
        std::optional<StoredValue> findKeyByEquivalence(
            facebook::jsi::Runtime &rt, const facebook::jsi::Value &searchKey) const;

        MapType map_;
    };

    /**
     * Install the PersistentMap factory object into the JavaScript runtime.
     * After calling this, JavaScript code can use:
     *
     *   const m1 = PersistentMap.empty();
     *   const m2 = PersistentMap.from({x: 1, y: 2});
     */
    void installPersistentMap(facebook::jsi::Runtime &rt);

} // namespace cljs
