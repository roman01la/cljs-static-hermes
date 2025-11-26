// Copyright (c) Tzvetan Mikov and contributors
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text

#include "PersistentVector.h"

#include <sstream>
#include <stdexcept>

namespace cljs {

PersistentVectorHostObject::PersistentVectorHostObject(VectorType vec)
    : vec_(std::move(vec)) {}

std::shared_ptr<PersistentVectorHostObject> PersistentVectorHostObject::empty() {
  return std::make_shared<PersistentVectorHostObject>();
}

std::shared_ptr<PersistentVectorHostObject>
PersistentVectorHostObject::fromArray(facebook::jsi::Runtime &rt,
                                       const facebook::jsi::Array &arr) {
  VectorType vec;
  size_t len = arr.size(rt);
  for (size_t i = 0; i < len; ++i) {
    vec = vec.push_back(copyValue(rt, arr.getValueAtIndex(rt, i)));
  }
  return std::make_shared<PersistentVectorHostObject>(std::move(vec));
}

facebook::jsi::Value
PersistentVectorHostObject::get(facebook::jsi::Runtime &rt,
                                const facebook::jsi::PropNameID &name) {
  std::string prop = name.utf8(rt);

  if (prop == "count") {
    auto weak = std::weak_ptr<PersistentVectorHostObject>(shared_from_this());
    return facebook::jsi::Function::createFromHostFunction(
        rt, facebook::jsi::PropNameID::forAscii(rt, "count"), 0,
        [weak](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
               const facebook::jsi::Value *, size_t) -> facebook::jsi::Value {
          auto shared = weak.lock();
          if (!shared) {
            throw facebook::jsi::JSError(runtime,
                                         "PersistentVector instance is invalid");
          }
          return facebook::jsi::Value(static_cast<double>(shared->count()));
        });
  }

  if (prop == "nth") {
    auto weak = std::weak_ptr<PersistentVectorHostObject>(shared_from_this());
    return facebook::jsi::Function::createFromHostFunction(
        rt, facebook::jsi::PropNameID::forAscii(rt, "nth"), 1,
        [weak](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
               const facebook::jsi::Value *args,
               size_t count) -> facebook::jsi::Value {
          auto shared = weak.lock();
          if (!shared) {
            throw facebook::jsi::JSError(runtime,
                                         "PersistentVector instance is invalid");
          }
          if (count < 1 || !args[0].isNumber()) {
            throw facebook::jsi::JSError(
                runtime, "nth requires a numeric index argument");
          }
          size_t index = static_cast<size_t>(args[0].asNumber());
          return shared->nth(runtime, index);
        });
  }

  if (prop == "conj") {
    auto weak = std::weak_ptr<PersistentVectorHostObject>(shared_from_this());
    return facebook::jsi::Function::createFromHostFunction(
        rt, facebook::jsi::PropNameID::forAscii(rt, "conj"), 1,
        [weak](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
               const facebook::jsi::Value *args,
               size_t count) -> facebook::jsi::Value {
          auto shared = weak.lock();
          if (!shared) {
            throw facebook::jsi::JSError(runtime,
                                         "PersistentVector instance is invalid");
          }
          if (count < 1) {
            throw facebook::jsi::JSError(runtime,
                                         "conj requires a value argument");
          }
          auto newVec = shared->conj(runtime, args[0]);
          return facebook::jsi::Object::createFromHostObject(runtime, newVec);
        });
  }

  if (prop == "pop") {
    auto weak = std::weak_ptr<PersistentVectorHostObject>(shared_from_this());
    return facebook::jsi::Function::createFromHostFunction(
        rt, facebook::jsi::PropNameID::forAscii(rt, "pop"), 0,
        [weak](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
               const facebook::jsi::Value *,
               size_t) -> facebook::jsi::Value {
          auto shared = weak.lock();
          if (!shared) {
            throw facebook::jsi::JSError(runtime,
                                         "PersistentVector instance is invalid");
          }
          auto newVec = shared->pop();
          return facebook::jsi::Object::createFromHostObject(runtime, newVec);
        });
  }

  if (prop == "assoc") {
    auto weak = std::weak_ptr<PersistentVectorHostObject>(shared_from_this());
    return facebook::jsi::Function::createFromHostFunction(
        rt, facebook::jsi::PropNameID::forAscii(rt, "assoc"), 2,
        [weak](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
               const facebook::jsi::Value *args,
               size_t count) -> facebook::jsi::Value {
          auto shared = weak.lock();
          if (!shared) {
            throw facebook::jsi::JSError(runtime,
                                         "PersistentVector instance is invalid");
          }
          if (count < 2 || !args[0].isNumber()) {
            throw facebook::jsi::JSError(
                runtime, "assoc requires an index and value argument");
          }
          size_t index = static_cast<size_t>(args[0].asNumber());
          auto newVec = shared->assoc(runtime, index, args[1]);
          return facebook::jsi::Object::createFromHostObject(runtime, newVec);
        });
  }

  if (prop == "first") {
    auto weak = std::weak_ptr<PersistentVectorHostObject>(shared_from_this());
    return facebook::jsi::Function::createFromHostFunction(
        rt, facebook::jsi::PropNameID::forAscii(rt, "first"), 0,
        [weak](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
               const facebook::jsi::Value *,
               size_t) -> facebook::jsi::Value {
          auto shared = weak.lock();
          if (!shared) {
            throw facebook::jsi::JSError(runtime,
                                         "PersistentVector instance is invalid");
          }
          return shared->first(runtime);
        });
  }

  if (prop == "last") {
    auto weak = std::weak_ptr<PersistentVectorHostObject>(shared_from_this());
    return facebook::jsi::Function::createFromHostFunction(
        rt, facebook::jsi::PropNameID::forAscii(rt, "last"), 0,
        [weak](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
               const facebook::jsi::Value *,
               size_t) -> facebook::jsi::Value {
          auto shared = weak.lock();
          if (!shared) {
            throw facebook::jsi::JSError(runtime,
                                         "PersistentVector instance is invalid");
          }
          return shared->last(runtime);
        });
  }

  if (prop == "empty") {
    auto weak = std::weak_ptr<PersistentVectorHostObject>(shared_from_this());
    return facebook::jsi::Function::createFromHostFunction(
        rt, facebook::jsi::PropNameID::forAscii(rt, "empty"), 0,
        [weak](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
               const facebook::jsi::Value *,
               size_t) -> facebook::jsi::Value {
          auto shared = weak.lock();
          if (!shared) {
            throw facebook::jsi::JSError(runtime,
                                         "PersistentVector instance is invalid");
          }
          return facebook::jsi::Value(shared->isEmpty());
        });
  }

  if (prop == "toArray") {
    auto weak = std::weak_ptr<PersistentVectorHostObject>(shared_from_this());
    return facebook::jsi::Function::createFromHostFunction(
        rt, facebook::jsi::PropNameID::forAscii(rt, "toArray"), 0,
        [weak](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
               const facebook::jsi::Value *,
               size_t) -> facebook::jsi::Value {
          auto shared = weak.lock();
          if (!shared) {
            throw facebook::jsi::JSError(runtime,
                                         "PersistentVector instance is invalid");
          }
          return shared->toArray(runtime);
        });
  }

  if (prop == "length") {
    return facebook::jsi::Value(static_cast<double>(count()));
  }

  return facebook::jsi::Value::undefined();
}

void PersistentVectorHostObject::set(facebook::jsi::Runtime &rt,
                                     const facebook::jsi::PropNameID &name,
                                     const facebook::jsi::Value &value) {
  // PersistentVector is immutable, so we don't allow setting properties
  throw facebook::jsi::JSError(
      rt, "PersistentVector is immutable - use conj, pop, or assoc instead");
}

std::vector<facebook::jsi::PropNameID>
PersistentVectorHostObject::getPropertyNames(facebook::jsi::Runtime &rt) {
  std::vector<facebook::jsi::PropNameID> result;
  result.emplace_back(facebook::jsi::PropNameID::forAscii(rt, "count"));
  result.emplace_back(facebook::jsi::PropNameID::forAscii(rt, "nth"));
  result.emplace_back(facebook::jsi::PropNameID::forAscii(rt, "conj"));
  result.emplace_back(facebook::jsi::PropNameID::forAscii(rt, "pop"));
  result.emplace_back(facebook::jsi::PropNameID::forAscii(rt, "assoc"));
  result.emplace_back(facebook::jsi::PropNameID::forAscii(rt, "first"));
  result.emplace_back(facebook::jsi::PropNameID::forAscii(rt, "last"));
  result.emplace_back(facebook::jsi::PropNameID::forAscii(rt, "empty"));
  result.emplace_back(facebook::jsi::PropNameID::forAscii(rt, "toArray"));
  result.emplace_back(facebook::jsi::PropNameID::forAscii(rt, "length"));
  return result;
}

size_t PersistentVectorHostObject::count() const { return vec_.size(); }

facebook::jsi::Value PersistentVectorHostObject::nth(facebook::jsi::Runtime &rt,
                                                     size_t index) const {
  if (index >= vec_.size()) {
    std::ostringstream msg;
    msg << "Index " << index << " out of bounds for vector of size "
        << vec_.size();
    throw facebook::jsi::JSError(rt, msg.str());
  }
  return getValue(rt, vec_[index]);
}

std::shared_ptr<PersistentVectorHostObject> PersistentVectorHostObject::conj(
    facebook::jsi::Runtime &rt, const facebook::jsi::Value &value) const {
  VectorType newVec = vec_.push_back(copyValue(rt, value));
  return std::make_shared<PersistentVectorHostObject>(std::move(newVec));
}

std::shared_ptr<PersistentVectorHostObject>
PersistentVectorHostObject::pop() const {
  if (vec_.empty()) {
    return std::make_shared<PersistentVectorHostObject>();
  }
  // Use take to get all elements except the last one
  VectorType newVec = vec_.take(vec_.size() - 1);
  return std::make_shared<PersistentVectorHostObject>(std::move(newVec));
}

std::shared_ptr<PersistentVectorHostObject> PersistentVectorHostObject::assoc(
    facebook::jsi::Runtime &rt, size_t index,
    const facebook::jsi::Value &value) const {
  if (index >= vec_.size()) {
    std::ostringstream msg;
    msg << "Index " << index << " out of bounds for vector of size "
        << vec_.size();
    throw facebook::jsi::JSError(rt, msg.str());
  }
  VectorType newVec = vec_.set(index, copyValue(rt, value));
  return std::make_shared<PersistentVectorHostObject>(std::move(newVec));
}

facebook::jsi::Value
PersistentVectorHostObject::first(facebook::jsi::Runtime &rt) const {
  if (vec_.empty()) {
    return facebook::jsi::Value::undefined();
  }
  return getValue(rt, vec_.front());
}

facebook::jsi::Value
PersistentVectorHostObject::last(facebook::jsi::Runtime &rt) const {
  if (vec_.empty()) {
    return facebook::jsi::Value::undefined();
  }
  return getValue(rt, vec_.back());
}

bool PersistentVectorHostObject::isEmpty() const { return vec_.empty(); }

facebook::jsi::Array
PersistentVectorHostObject::toArray(facebook::jsi::Runtime &rt) const {
  facebook::jsi::Array arr(rt, vec_.size());
  for (size_t i = 0; i < vec_.size(); ++i) {
    arr.setValueAtIndex(rt, i, getValue(rt, vec_[i]));
  }
  return arr;
}

PersistentVectorHostObject::StoredValue
PersistentVectorHostObject::copyValue(facebook::jsi::Runtime &rt,
                                       const facebook::jsi::Value &value) {
  // Create a deep copy of the jsi::Value
  // For primitives, this is straightforward
  // For objects, we need to handle them specially

  if (value.isUndefined()) {
    return std::make_shared<facebook::jsi::Value>(
        facebook::jsi::Value::undefined());
  }
  if (value.isNull()) {
    return std::make_shared<facebook::jsi::Value>(facebook::jsi::Value::null());
  }
  if (value.isBool()) {
    return std::make_shared<facebook::jsi::Value>(value.getBool());
  }
  if (value.isNumber()) {
    return std::make_shared<facebook::jsi::Value>(value.getNumber());
  }
  if (value.isString()) {
    // Create a new string value
    return std::make_shared<facebook::jsi::Value>(
        rt, facebook::jsi::String::createFromUtf8(
                rt, value.getString(rt).utf8(rt)));
  }
  if (value.isSymbol()) {
    // Symbols are immutable, so we can just copy the reference
    return std::make_shared<facebook::jsi::Value>(rt, value.getSymbol(rt));
  }
  if (value.isObject()) {
    // For objects (including arrays, functions, etc.), we store a reference
    // Note: This means modifications to the original object will be visible
    // through the vector. This is consistent with how ClojureScript handles
    // JavaScript objects in vectors.
    return std::make_shared<facebook::jsi::Value>(rt, value.getObject(rt));
  }

  // Fallback: undefined
  return std::make_shared<facebook::jsi::Value>(
      facebook::jsi::Value::undefined());
}

facebook::jsi::Value
PersistentVectorHostObject::getValue(facebook::jsi::Runtime &rt,
                                     const StoredValue &stored) {
  if (!stored) {
    return facebook::jsi::Value::undefined();
  }

  const facebook::jsi::Value &value = *stored;

  if (value.isUndefined()) {
    return facebook::jsi::Value::undefined();
  }
  if (value.isNull()) {
    return facebook::jsi::Value::null();
  }
  if (value.isBool()) {
    return facebook::jsi::Value(value.getBool());
  }
  if (value.isNumber()) {
    return facebook::jsi::Value(value.getNumber());
  }
  if (value.isString()) {
    return facebook::jsi::Value(rt, value.getString(rt));
  }
  if (value.isSymbol()) {
    return facebook::jsi::Value(rt, value.getSymbol(rt));
  }
  if (value.isObject()) {
    return facebook::jsi::Value(rt, value.getObject(rt));
  }

  return facebook::jsi::Value::undefined();
}

void installPersistentVector(facebook::jsi::Runtime &rt) {
  // Create the PersistentVector factory object
  facebook::jsi::Object factory(rt);

  // PersistentVector.empty() - Create an empty vector
  factory.setProperty(
      rt, "empty",
      facebook::jsi::Function::createFromHostFunction(
          rt, facebook::jsi::PropNameID::forAscii(rt, "empty"), 0,
          [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
             const facebook::jsi::Value *,
             size_t) -> facebook::jsi::Value {
            auto vec = PersistentVectorHostObject::empty();
            return facebook::jsi::Object::createFromHostObject(runtime, vec);
          }));

  // PersistentVector.from(array) - Create a vector from a JavaScript array
  factory.setProperty(
      rt, "from",
      facebook::jsi::Function::createFromHostFunction(
          rt, facebook::jsi::PropNameID::forAscii(rt, "from"), 1,
          [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
             const facebook::jsi::Value *args,
             size_t count) -> facebook::jsi::Value {
            if (count < 1 || !args[0].isObject()) {
              throw facebook::jsi::JSError(
                  runtime, "PersistentVector.from requires an array argument");
            }
            facebook::jsi::Object obj = args[0].getObject(runtime);
            if (!obj.isArray(runtime)) {
              throw facebook::jsi::JSError(
                  runtime, "PersistentVector.from requires an array argument");
            }
            facebook::jsi::Array arr = obj.getArray(runtime);
            auto vec = PersistentVectorHostObject::fromArray(runtime, arr);
            return facebook::jsi::Object::createFromHostObject(runtime, vec);
          }));

  // Install the factory object as globalThis.PersistentVector
  rt.global().setProperty(rt, "PersistentVector", factory);
}

} // namespace cljs
