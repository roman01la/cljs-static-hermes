// Copyright (c) Tzvetan Mikov and contributors
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text

#pragma once

#include <hermes/hermes.h>
#include <jsi/jsi.h>

#include <immer/flex_vector.hpp>
#include <immer/flex_vector_transient.hpp>

#include "StoredValue.h"

#include <memory>
#include <string>
#include <variant>

namespace cljs
{

  /**
   * PersistentVectorHostObject wraps an immer::flex_vector to provide
   * a ClojureScript-compatible persistent vector implementation.
   *
   * Operations:
   * - count() - Returns the number of elements
   * - nth(index) - Returns the element at the given index
   * - conj(value) - Returns a new vector with value appended
   * - pop() - Returns a new vector without the last element
   * - assoc(index, value) - Returns a new vector with value at index
   * - first() - Returns the first element
   * - last() - Returns the last element
   * - empty() - Returns true if the vector is empty
   * - toArray() - Converts to a JavaScript array
   * - batchConj(values) - Batch append multiple values (optimized)
   * - batchAssoc(updates) - Batch update multiple indices (optimized)
   */
  class PersistentVectorHostObject
      : public facebook::jsi::HostObject,
        public std::enable_shared_from_this<PersistentVectorHostObject>
  {
  public:
    using VectorType = immer::flex_vector<StoredValue>;

    // Constructors
    PersistentVectorHostObject() = default;
    explicit PersistentVectorHostObject(VectorType vec);

    // Factory methods
    static std::shared_ptr<PersistentVectorHostObject> empty();
    static std::shared_ptr<PersistentVectorHostObject>
    fromArray(facebook::jsi::Runtime &rt, const facebook::jsi::Array &arr);

    // HostObject interface
    facebook::jsi::Value get(facebook::jsi::Runtime &rt,
                             const facebook::jsi::PropNameID &name) override;
    void set(facebook::jsi::Runtime &rt, const facebook::jsi::PropNameID &name,
             const facebook::jsi::Value &value) override;
    std::vector<facebook::jsi::PropNameID>
    getPropertyNames(facebook::jsi::Runtime &rt) override;

    // Vector operations
    size_t count() const;
    facebook::jsi::Value nth(facebook::jsi::Runtime &rt, size_t index) const;
    bool equiv(facebook::jsi::Runtime &rt, const facebook::jsi::Value &other) const;
    std::shared_ptr<PersistentVectorHostObject>
    conj(facebook::jsi::Runtime &rt, const facebook::jsi::Value &value) const;
    std::shared_ptr<PersistentVectorHostObject> pop() const;
    std::shared_ptr<PersistentVectorHostObject>
    assoc(facebook::jsi::Runtime &rt, size_t index,
          const facebook::jsi::Value &value) const;
    facebook::jsi::Value first(facebook::jsi::Runtime &rt) const;
    facebook::jsi::Value last(facebook::jsi::Runtime &rt) const;
    bool isEmpty() const;
    facebook::jsi::Array toArray(facebook::jsi::Runtime &rt) const;

    // Batch operations using transient vectors for efficiency
    std::shared_ptr<PersistentVectorHostObject>
    batchConj(facebook::jsi::Runtime &rt, const facebook::jsi::Array &values) const;

    std::shared_ptr<PersistentVectorHostObject>
    batchAssoc(facebook::jsi::Runtime &rt,
               const facebook::jsi::Object &updates) const;

    // High-performance reduce/forEach for iteration-heavy operations
    facebook::jsi::Value
    reduce(facebook::jsi::Runtime &rt, const facebook::jsi::Function &fn,
           const facebook::jsi::Value &initialValue) const;

    // Access the underlying vector (for testing/debugging)
    const VectorType &getVector() const { return vec_; }

  private:
    // Helper to convert jsi::Value to StoredValue for storage
    static StoredValue convertValue(facebook::jsi::Runtime &rt,
                                    const facebook::jsi::Value &value);

    // Helper to convert StoredValue back to jsi::Value
    static facebook::jsi::Value reconstructValue(facebook::jsi::Runtime &rt,
                                                 const StoredValue &stored);

    VectorType vec_;
  };

  /**
   * Install the PersistentVector factory object into the JavaScript runtime.
   * After calling this, JavaScript code can use:
   *
   *   const v1 = PersistentVector.empty();
   *   const v2 = PersistentVector.from([1, 2, 3]);
   */
  void installPersistentVector(facebook::jsi::Runtime &rt);

} // namespace cljs
