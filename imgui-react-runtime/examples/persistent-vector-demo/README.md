# PersistentVector Demo

This directory contains demos for the native PersistentVector functionality.

## C++ Demo (main.cpp)

A standalone C++ console application demonstrating the `PersistentVector` via JavaScript.
Can also load and run compiled ClojureScript benchmarks.

### Building

From the `imgui-react-runtime` directory:

```bash
# Configure (first time only)
cmake -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug -G Ninja

# Build
cmake --build cmake-build-debug --target persistent-vector-demo
```

### Running Built-in Demo

```bash
./cmake-build-debug/examples/persistent-vector-demo/persistent-vector-demo
```

### Running ClojureScript Benchmark

First, compile the ClojureScript example (from project root):

```bash
npx shadow-cljs release persistent-vector-example
```

Then run with the compiled bundle:

```bash
./cmake-build-debug/examples/persistent-vector-demo/persistent-vector-demo \
  cljs-out/main.js
```

### Command Line Options

```
Usage: persistent-vector-demo [options] [bundle.js]

Options:
  --help, -h    Show help message

If no bundle is provided, runs the built-in JavaScript demo.
If a bundle path is provided, loads and executes that ClojureScript bundle.
```

## ClojureScript Demo

A ClojureScript example showing how to use the native PersistentVector with ClojureScript collection protocols.

### Files

- `src/hermes/persistent_vector.cljs` - Protocol implementations wrapper
- `src/hermes/persistent_vector_example.cljs` - Example program with benchmarks

### Building

From the project root:

```bash
# Build the ClojureScript example
npx shadow-cljs release persistent-vector-example
```

The compiled output will be in `imgui-react-runtime/examples/persistent-vector-demo/cljs-out/main.js`.

### Features Demonstrated

1. **Creating vectors** - `PersistentVector.empty()` and `PersistentVector.from(array)`
2. **Element access** - `count()`, `nth()`, `first()`, `last()`, `empty()`
3. **Persistent operations** - `conj()`, `pop()`, `assoc()` (all return new vectors)
4. **Type support** - Numbers, strings, booleans, null, and objects
5. **Chaining** - Operations can be chained for fluent API
6. **Structural sharing** - Multiple vectors efficiently share structure
7. **Error handling** - Proper exceptions for out-of-bounds access
8. **Performance benchmarks** - Comparing native vs CLJS vector performance

## ClojureScript Protocol Support

The `hermes.persistent-vector` namespace provides a `NativePersistentVector` type that implements:

- `ICounted` - `(count v)`
- `IIndexed` - `(nth v n)`, `(nth v n not-found)`
- `ILookup` - `(get v k)`, `(get v k not-found)`
- `ICollection` - `(conj v x)`
- `IStack` - `(peek v)`, `(pop v)`
- `ISeqable` - `(seq v)`
- `IReduce` - `(reduce f v)`, `(reduce f init v)`
- `IAssociative` - `(assoc v k val)`
- `IVector` - `(assoc v n val)`
- `IFn` - `(v n)`, `(v n not-found)`
- `IEquiv` - `(= v1 v2)`
- `IHash` - `(hash v)`

This allows the native vector to be used seamlessly with all standard ClojureScript functions.

## Sample Output (C++ Demo)

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
