// Copyright (c) Tzvetan Mikov and contributors
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text

#include "PersistentVector.h"

#include <sstream>
#include <stdexcept>

namespace cljs
{

  // Helper function to log messages to JavaScript console
  void logToConsole(facebook::jsi::Runtime &rt, const std::string &message)
  {
    auto log = rt.global().getProperty(rt, "print").getObject(rt).asFunction(rt);
    log.call(rt, facebook::jsi::String::createFromUtf8(rt, message));
  }

  PersistentVectorHostObject::PersistentVectorHostObject(VectorType vec)
      : vec_(std::move(vec)) {}

  std::shared_ptr<PersistentVectorHostObject> PersistentVectorHostObject::empty()
  {
    return std::make_shared<PersistentVectorHostObject>();
  }

  std::shared_ptr<PersistentVectorHostObject>
  PersistentVectorHostObject::fromArray(facebook::jsi::Runtime &rt,
                                        const facebook::jsi::Array &arr)
  {
    auto transient = VectorType{}.transient();
    size_t len = arr.size(rt);
    for (size_t i = 0; i < len; ++i)
    {
      transient.push_back(convertValue(rt, arr.getValueAtIndex(rt, i)));
    }
    return std::make_shared<PersistentVectorHostObject>(transient.persistent());
  }

  facebook::jsi::Value
  PersistentVectorHostObject::get(facebook::jsi::Runtime &rt,
                                  const facebook::jsi::PropNameID &name)
  {
    std::string prop = name.utf8(rt);

    if (prop == "count" || prop == "length")
    {
      return facebook::jsi::Value(static_cast<double>(count()));
    }

    return facebook::jsi::Value::undefined();
  }

  void PersistentVectorHostObject::set(facebook::jsi::Runtime &rt,
                                       const facebook::jsi::PropNameID &name,
                                       const facebook::jsi::Value &value)
  {
  }

  std::vector<facebook::jsi::PropNameID>
  PersistentVectorHostObject::getPropertyNames(facebook::jsi::Runtime &rt)
  {
    std::vector<facebook::jsi::PropNameID> result;
    return result;
  }

  size_t PersistentVectorHostObject::count() const { return vec_.size(); }

  facebook::jsi::Value PersistentVectorHostObject::nth(facebook::jsi::Runtime &rt,
                                                       size_t index) const
  {
    if (index >= vec_.size())
    {
      std::ostringstream msg;
      msg << "Index " << index << " out of bounds for vector of size "
          << vec_.size();
      throw facebook::jsi::JSError(rt, msg.str());
    }
    return reconstructValue(rt, vec_[index]);
  }

  bool PersistentVectorHostObject::equiv(facebook::jsi::Runtime &rt, const facebook::jsi::Value &other) const
  {
    if (!other.isObject())
    {
      return false;
    }
    auto otherObj = other.getObject(rt);
    auto otherHostObj = otherObj.getHostObject<PersistentVectorHostObject>(rt);
    if (!otherHostObj)
    {
      return false;
    }

    // Check sizes first
    if (vec_.size() != otherHostObj->vec_.size())
    {
      return false;
    }

    // Compare elements deeply
    for (size_t i = 0; i < vec_.size(); ++i)
    {
      const StoredValue &a = vec_[i];
      const StoredValue &b = otherHostObj->vec_[i];

      // Compare by type first
      if (a.type != b.type)
      {
        return false;
      }

      // Compare values based on type
      switch (a.type)
      {
      case StoredValue::NIL:
        // Both are null, they're equal
        continue;
      case StoredValue::BOOL:
        if (a.primitive.bool_val != b.primitive.bool_val)
          return false;
        break;
      case StoredValue::NUMBER:
        if (a.primitive.number_val != b.primitive.number_val)
          return false;
        break;
      case StoredValue::STRING:
        // Both strings must be equal
        if ((!a.string_val && b.string_val) || (a.string_val && !b.string_val))
          return false;
        if (a.string_val && b.string_val && *a.string_val != *b.string_val)
          return false;
        break;
      case StoredValue::OBJECT_REF:
        // Try to use equiv for objects, fall back to reference equality

        if (!a.object_val && !b.object_val)
        {
          // Both null
          continue;
        }
        if ((!a.object_val && b.object_val) || (a.object_val && !b.object_val))
        {
          return false;
        }

        // Try to call equiv method on the objects
        try
        {
          facebook::jsi::Value aVal(rt, *a.object_val);
          facebook::jsi::Value bVal(rt, *b.object_val);

          // Check if object has an equiv method
          if (aVal.isObject() && aVal.getObject(rt).hasProperty(rt, "equiv"))
          {
            auto equivProp = aVal.getObject(rt).getProperty(rt, "equiv");
            if (equivProp.isObject() && equivProp.getObject(rt).isFunction(rt))
            {
              auto equivFn = equivProp.getObject(rt).asFunction(rt);
              auto result = equivFn.callWithThis(rt, aVal.getObject(rt), bVal);
              if (result.isBool() && !result.getBool())
              {
                return false;
              }
            }
          }
          // If no equiv method, compare by reference
          else if (a.object_val.get() != b.object_val.get())
          {
            return false;
          }
        }
        catch (const facebook::jsi::JSError &)
        {
          // If equiv call fails, fall back to reference comparison
          if (a.object_val.get() != b.object_val.get())
          {
            return false;
          }
        }
        break;
      }
    }

    return true;
  }

  std::shared_ptr<PersistentVectorHostObject> PersistentVectorHostObject::conj(
      facebook::jsi::Runtime &rt, const facebook::jsi::Value &value) const
  {
    VectorType newVec = vec_.push_back(convertValue(rt, value));
    return std::make_shared<PersistentVectorHostObject>(std::move(newVec));
  }

  std::shared_ptr<PersistentVectorHostObject>
  PersistentVectorHostObject::pop() const
  {
    if (vec_.empty())
    {
      return std::make_shared<PersistentVectorHostObject>();
    }
    // Use take to get all elements except the last one
    VectorType newVec = vec_.take(vec_.size() - 1);
    return std::make_shared<PersistentVectorHostObject>(std::move(newVec));
  }

  std::shared_ptr<PersistentVectorHostObject> PersistentVectorHostObject::assoc(
      facebook::jsi::Runtime &rt, size_t index,
      const facebook::jsi::Value &value) const
  {
    if (index >= vec_.size())
    {
      std::ostringstream msg;
      msg << "Index " << index << " out of bounds for vector of size "
          << vec_.size();
      throw facebook::jsi::JSError(rt, msg.str());
    }
    VectorType newVec = vec_.set(index, convertValue(rt, value));
    return std::make_shared<PersistentVectorHostObject>(std::move(newVec));
  }

  facebook::jsi::Value
  PersistentVectorHostObject::first(facebook::jsi::Runtime &rt) const
  {
    if (vec_.empty())
    {
      return facebook::jsi::Value::undefined();
    }
    return reconstructValue(rt, vec_.front());
  }

  facebook::jsi::Value
  PersistentVectorHostObject::last(facebook::jsi::Runtime &rt) const
  {
    if (vec_.empty())
    {
      return facebook::jsi::Value::undefined();
    }
    return reconstructValue(rt, vec_.back());
  }

  bool PersistentVectorHostObject::isEmpty() const { return vec_.empty(); }

  facebook::jsi::Array
  PersistentVectorHostObject::toArray(facebook::jsi::Runtime &rt) const
  {
    facebook::jsi::Array arr(rt, vec_.size());
    for (size_t i = 0; i < vec_.size(); ++i)
    {
      arr.setValueAtIndex(rt, i, reconstructValue(rt, vec_[i]));
    }
    return arr;
  }

  std::shared_ptr<PersistentVectorHostObject>
  PersistentVectorHostObject::batchConj(facebook::jsi::Runtime &rt,
                                        const facebook::jsi::Array &values) const
  {
    auto transient = vec_.transient();
    size_t len = values.size(rt);
    for (size_t i = 0; i < len; ++i)
    {
      transient.push_back(convertValue(rt, values.getValueAtIndex(rt, i)));
    }
    return std::make_shared<PersistentVectorHostObject>(transient.persistent());
  }

  std::shared_ptr<PersistentVectorHostObject>
  PersistentVectorHostObject::batchAssoc(facebook::jsi::Runtime &rt,
                                         const facebook::jsi::Object &updates) const
  {
    auto transient = vec_.transient();

    // Get all property names from the updates object
    auto keys = updates.getPropertyNames(rt);
    for (size_t i = 0; i < keys.size(rt); ++i)
    {
      auto keyId = keys.getValueAtIndex(rt, i);
      if (!keyId.isString())
      {
        continue;
      }
      std::string key = keyId.getString(rt).utf8(rt);

      // Parse the key as an index
      try
      {
        size_t index = std::stoul(key);
        if (index < transient.size())
        {
          transient.set(index, convertValue(rt, updates.getProperty(rt, key.c_str())));
        }
      }
      catch (...)
      {
        // Skip non-numeric keys
      }
    }

    return std::make_shared<PersistentVectorHostObject>(transient.persistent());
  }

  facebook::jsi::Value
  PersistentVectorHostObject::reduce(facebook::jsi::Runtime &rt,
                                     const facebook::jsi::Function &fn,
                                     const facebook::jsi::Value &initialValue) const
  {
    // For the first iteration, we need initialValue which we can use directly
    // For subsequent iterations, we call fn and use the result

    if (vec_.empty())
    {
      // Return a copy of initialValue
      if (initialValue.isUndefined())
        return facebook::jsi::Value();
      if (initialValue.isNull())
        return facebook::jsi::Value(facebook::jsi::Value::null());
      if (initialValue.isBool())
        return facebook::jsi::Value(initialValue.getBool());
      if (initialValue.isNumber())
        return facebook::jsi::Value(initialValue.getNumber());
      if (initialValue.isString())
        return facebook::jsi::Value(initialValue.getString(rt));
      if (initialValue.isObject())
        return facebook::jsi::Value(initialValue.getObject(rt));
    }

    // Process first element
    facebook::jsi::Value element = reconstructValue(rt, vec_[0]);
    facebook::jsi::Value accumulator = fn.call(rt, initialValue, element, facebook::jsi::Value(0.0));

    // Process remaining elements
    for (size_t i = 1; i < vec_.size(); ++i)
    {
      facebook::jsi::Value nextElement = reconstructValue(rt, vec_[i]);
      accumulator = fn.call(rt, accumulator, nextElement, facebook::jsi::Value(static_cast<double>(i)));
    }

    return accumulator;
  }
  StoredValue
  PersistentVectorHostObject::convertValue(facebook::jsi::Runtime &rt,
                                           const facebook::jsi::Value &value)
  {
    // Optimized value storage:
    // - Primitives are stored directly in the union (no allocation)
    // - Objects are stored as shared_ptr for proper lifecycle management

    if (value.isUndefined())
    {
      return StoredValue(); // Default-constructed NIL value
    }
    if (value.isNull())
    {
      StoredValue result;
      result.type = StoredValue::NIL;
      return result;
    }
    if (value.isBool())
    {
      return StoredValue(value.getBool());
    }
    if (value.isNumber())
    {
      return StoredValue(value.getNumber());
    }
    if (value.isString())
    {
      return StoredValue(value.getString(rt).utf8(rt));
    }
    if (value.isSymbol())
    {
      // Symbols can't be stored directly; fall back to wrapping in object
      auto sym = value.getSymbol(rt);
      // Create a wrapper object if needed (or we could store as object ref)
      // For now, treat as NIL as symbols are rare in vectors
      return StoredValue();
    }
    if (value.isObject())
    {
      auto obj = value.getObject(rt);
      return StoredValue(std::make_shared<facebook::jsi::Object>(std::move(obj)));
    }

    // Fallback: undefined
    return StoredValue();
  }

  facebook::jsi::Value
  PersistentVectorHostObject::reconstructValue(facebook::jsi::Runtime &rt,
                                               const StoredValue &stored)
  {
    // Fast path for primitives - no allocation needed
    switch (stored.type)
    {
    case StoredValue::NIL:
      return facebook::jsi::Value::null();
    case StoredValue::BOOL:
      return facebook::jsi::Value(stored.primitive.bool_val);
    case StoredValue::NUMBER:
      return facebook::jsi::Value(stored.primitive.number_val);
    case StoredValue::STRING:
      return stored.string_val
                 ? facebook::jsi::Value(rt, facebook::jsi::String::createFromUtf8(rt, *stored.string_val))
                 : facebook::jsi::Value::null();
    case StoredValue::OBJECT_REF:
      return stored.object_val
                 ? facebook::jsi::Value(rt, *stored.object_val)
                 : facebook::jsi::Value::null();
    }

    return facebook::jsi::Value::null();
  }
  void installPersistentVector(facebook::jsi::Runtime &rt)
  {
    // Create the PersistentVector factory object
    facebook::jsi::Object factory(rt);

    // PersistentVector.empty() - Create an empty vector
    factory.setProperty(
        rt, "empty",
        facebook::jsi::Function::createFromHostFunction(
            rt, facebook::jsi::PropNameID::forAscii(rt, "empty"), 0,
            [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
               const facebook::jsi::Value *,
               size_t) -> facebook::jsi::Value
            {
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
               size_t count) -> facebook::jsi::Value
            {
              if (count < 1 || !args[0].isObject())
              {
                throw facebook::jsi::JSError(
                    runtime, "PersistentVector.from requires an array argument");
              }
              facebook::jsi::Object obj = args[0].getObject(runtime);
              if (!obj.isArray(runtime))
              {
                throw facebook::jsi::JSError(
                    runtime, "PersistentVector.from requires an array argument");
              }
              facebook::jsi::Array arr = obj.getArray(runtime);
              auto vec = PersistentVectorHostObject::fromArray(runtime, arr);
              return facebook::jsi::Object::createFromHostObject(runtime, vec);
            }));

    factory.setProperty(rt, "conj",
                        facebook::jsi::Function::createFromHostFunction(
                            rt, facebook::jsi::PropNameID::forAscii(rt, "conj"), 2,
                            [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &,
                               const facebook::jsi::Value *args,
                               size_t count) -> facebook::jsi::Value
                            {
                              auto vec = args[0].getObject(runtime).getHostObject<PersistentVectorHostObject>(runtime);
                              if (!vec)
                              {
                                throw facebook::jsi::JSError(runtime,
                                                             "PersistentVector instance is invalid");
                              }
                              if (count < 2)
                              {
                                throw facebook::jsi::JSError(runtime,
                                                             "conj requires two arguments: the vector and the value to add");
                              }
                              auto newVec = vec->conj(runtime, args[1]);
                              return facebook::jsi::Object::createFromHostObject(runtime, newVec);
                            }));

    factory.setProperty(rt, "nth", facebook::jsi::Function::createFromHostFunction(rt, facebook::jsi::PropNameID::forAscii(rt, "nth"), 2, [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &, const facebook::jsi::Value *args, size_t count) -> facebook::jsi::Value
                                                                                   {
            auto vec = args[0].getObject(runtime).getHostObject<PersistentVectorHostObject>(runtime);
            if (!vec)
            {
              throw facebook::jsi::JSError(runtime,
                                           "PersistentVector instance is invalid");
            }
            if (count < 2 || !args[1].isNumber())
            {
              throw facebook::jsi::JSError(
                  runtime, "nth requires a numeric index argument");
            }
            size_t index = static_cast<size_t>(args[1].asNumber());
            return vec->nth(runtime, index); }));

    factory.setProperty(rt, "equiv", facebook::jsi::Function::createFromHostFunction(rt, facebook::jsi::PropNameID::forAscii(rt, "equiv"), 2, [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &, const facebook::jsi::Value *args, size_t count) -> facebook::jsi::Value
                                                                                     {
            if (count < 2)
            {
              throw facebook::jsi::JSError(
                  runtime, "equiv requires two arguments");
            }
            
            if (!args[0].isObject())
            {
              throw facebook::jsi::JSError(runtime,
                                           "First argument to equiv must be an object");
            }
            
            auto vec1 = args[0].getObject(runtime).getHostObject<PersistentVectorHostObject>(runtime);
            if (!vec1)
            {
              throw facebook::jsi::JSError(runtime,
                                           "First argument must be a PersistentVector instance");
            }
            
            bool isEqual = vec1->equiv(runtime, args[1]);
            return facebook::jsi::Value(isEqual); }));

    factory.setProperty(rt, "pop", facebook::jsi::Function::createFromHostFunction(rt, facebook::jsi::PropNameID::forAscii(rt, "pop"), 1, [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &, const facebook::jsi::Value *args, size_t count) -> facebook::jsi::Value
                                                                                   {
            auto vec = args[0].getObject(runtime).getHostObject<PersistentVectorHostObject>(runtime);
            if (!vec)
            {
              throw facebook::jsi::JSError(runtime,
                                           "PersistentVector instance is invalid");
            }
            auto newVec = vec->pop();
            return facebook::jsi::Object::createFromHostObject(runtime, newVec); }));

    factory.setProperty(rt, "assoc", facebook::jsi::Function::createFromHostFunction(rt, facebook::jsi::PropNameID::forAscii(rt, "assoc"), 3, [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &, const facebook::jsi::Value *args, size_t count) -> facebook::jsi::Value
                                                                                     {
            auto vec = args[0].getObject(runtime).getHostObject<PersistentVectorHostObject>(runtime);
            if (!vec)
            {
              throw facebook::jsi::JSError(runtime,
                                           "PersistentVector instance is invalid");
            }
            if (count < 3 || !args[1].isNumber())
            {
              throw facebook::jsi::JSError(
                  runtime, "assoc requires a vector, index, and value argument");
            }
            size_t index = static_cast<size_t>(args[1].asNumber());
            auto newVec = vec->assoc(runtime, index, args[2]);
            return facebook::jsi::Object::createFromHostObject(runtime, newVec); }));

    factory.setProperty(rt, "first", facebook::jsi::Function::createFromHostFunction(rt, facebook::jsi::PropNameID::forAscii(rt, "first"), 1, [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &, const facebook::jsi::Value *args, size_t count) -> facebook::jsi::Value
                                                                                     {
            auto vec = args[0].getObject(runtime).getHostObject<PersistentVectorHostObject>(runtime);
            if (!vec)
            {
              throw facebook::jsi::JSError(runtime,
                                           "PersistentVector instance is invalid");
            }
            return vec->first(runtime); }));

    factory.setProperty(rt, "last", facebook::jsi::Function::createFromHostFunction(rt, facebook::jsi::PropNameID::forAscii(rt, "last"), 1, [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &, const facebook::jsi::Value *args, size_t count) -> facebook::jsi::Value
                                                                                    {
            auto vec = args[0].getObject(runtime).getHostObject<PersistentVectorHostObject>(runtime);
            if (!vec)
            {
              throw facebook::jsi::JSError(runtime,
                                           "PersistentVector instance is invalid");
            }
            return vec->last(runtime); }));

    factory.setProperty(rt, "isEmpty", facebook::jsi::Function::createFromHostFunction(rt, facebook::jsi::PropNameID::forAscii(rt, "isEmpty"), 1, [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &, const facebook::jsi::Value *args, size_t count) -> facebook::jsi::Value
                                                                                       {
            auto vec = args[0].getObject(runtime).getHostObject<PersistentVectorHostObject>(runtime);
            if (!vec)
            {
              throw facebook::jsi::JSError(runtime,
                                           "PersistentVector instance is invalid");
            }
            return facebook::jsi::Value(vec->isEmpty()); }));

    factory.setProperty(rt, "toArray", facebook::jsi::Function::createFromHostFunction(rt, facebook::jsi::PropNameID::forAscii(rt, "toArray"), 1, [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &, const facebook::jsi::Value *args, size_t count) -> facebook::jsi::Value
                                                                                       {
            auto vec = args[0].getObject(runtime).getHostObject<PersistentVectorHostObject>(runtime);
            if (!vec)
            {
              throw facebook::jsi::JSError(runtime,
                                           "PersistentVector instance is invalid");
            }
            return vec->toArray(runtime); }));

    factory.setProperty(rt, "reduce", facebook::jsi::Function::createFromHostFunction(rt, facebook::jsi::PropNameID::forAscii(rt, "reduce"), 3, [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &, const facebook::jsi::Value *args, size_t count) -> facebook::jsi::Value
                                                                                      {
            auto vec = args[0].getObject(runtime).getHostObject<PersistentVectorHostObject>(runtime);
            if (!vec)
            {
              throw facebook::jsi::JSError(runtime,
                                           "PersistentVector instance is invalid");
            }
            if (count < 3 || !args[1].isObject())
            {
              throw facebook::jsi::JSError(runtime,
                                           "reduce requires a vector, function, and initial value");
            }
            auto fn = args[1].getObject(runtime).asFunction(runtime);
            return vec->reduce(runtime, fn, args[2]); }));

    factory.setProperty(rt, "batchConj", facebook::jsi::Function::createFromHostFunction(rt, facebook::jsi::PropNameID::forAscii(rt, "batchConj"), 2, [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &, const facebook::jsi::Value *args, size_t count) -> facebook::jsi::Value
                                                                                         {
            auto vec = args[0].getObject(runtime).getHostObject<PersistentVectorHostObject>(runtime);
            if (!vec)
            {
              throw facebook::jsi::JSError(runtime,
                                           "PersistentVector instance is invalid");
            }
            if (count < 2 || !args[1].isObject())
            {
              throw facebook::jsi::JSError(runtime,
                                           "batchConj requires a vector and array argument");
            }
            auto arr = args[1].getObject(runtime).getArray(runtime);
            auto newVec = vec->batchConj(runtime, arr);
            return facebook::jsi::Object::createFromHostObject(runtime, newVec); }));

    factory.setProperty(rt, "batchAssoc", facebook::jsi::Function::createFromHostFunction(rt, facebook::jsi::PropNameID::forAscii(rt, "batchAssoc"), 2, [](facebook::jsi::Runtime &runtime, const facebook::jsi::Value &, const facebook::jsi::Value *args, size_t count) -> facebook::jsi::Value
                                                                                          {
            auto vec = args[0].getObject(runtime).getHostObject<PersistentVectorHostObject>(runtime);
            if (!vec)
            {
              throw facebook::jsi::JSError(runtime,
                                           "PersistentVector instance is invalid");
            }
            if (count < 2 || !args[1].isObject())
            {
              throw facebook::jsi::JSError(runtime,
                                           "batchAssoc requires a vector and object argument");
            }
            auto obj = args[1].getObject(runtime);
            auto newVec = vec->batchAssoc(runtime, obj);
            return facebook::jsi::Object::createFromHostObject(runtime, newVec); }));

    // Install the factory object as globalThis.PersistentVector
    rt.global()
        .setProperty(rt, "PersistentVector", factory);
  }

} // namespace cljs
