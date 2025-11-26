# PersistentVector Demo

A standalone console application demonstrating the `PersistentVector` functionality without any GUI dependencies.

## Building

From the `imgui-react-runtime` directory:

```bash
# Configure (first time only)
cmake -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug -G Ninja

# Build
cmake --build cmake-build-debug --target persistent-vector-demo
```

## Running

```bash
./cmake-build-debug/examples/persistent-vector-demo/persistent-vector-demo
```

## Features Demonstrated

1. **Creating vectors** - `PersistentVector.empty()` and `PersistentVector.from(array)`
2. **Element access** - `count()`, `nth()`, `first()`, `last()`, `empty()`
3. **Persistent operations** - `conj()`, `pop()`, `assoc()` (all return new vectors)
4. **Type support** - Numbers, strings, booleans, null, and objects
5. **Chaining** - Operations can be chained for fluent API
6. **Structural sharing** - Multiple vectors efficiently share structure
7. **Error handling** - Proper exceptions for out-of-bounds access

## Sample Output

```
=== PersistentVector Demo ===

1. Creating an empty vector:
   empty.count() = 0
   empty.empty() = true

2. Creating a vector from [1, 2, 3]:
   v1.count() = 3
   v1.toArray() = [1,2,3]
   v1.first() = 1
   v1.last() = 3

...
```
