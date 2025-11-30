// Copyright (c) Tzvetan Mikov and contributors
// SPDX-License-Identifier: MIT
// See LICENSE file for full license text

/**
 * PersistentVector Demo
 *
 * A standalone console application demonstrating the PersistentVector
 * functionality without any GUI dependencies.
 *
 * Can also load and run compiled ClojureScript benchmarks.
 *
 * Usage:
 *   ./persistent-vector-demo              # Run built-in JS demo
 *   ./persistent-vector-demo <bundle.js>  # Run compiled ClojureScript bundle
 */

#include <hermes/hermes.h>
#include <jsi/jsi.h>

#include "PersistentVector.h"
#include "PersistentMap.h"

#include <chrono>
#include <climits>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

// JavaScript code that demonstrates PersistentVector operations
static const char *kDemoScript = R"JS(
// ============================================
// PersistentVector Demo
// ============================================

console.log("=== PersistentVector Demo ===\n");

// 1. Create an empty vector
console.log("1. Creating an empty vector:");
const empty = PersistentVector.empty();
console.log("   empty.count() =", empty.count());
console.log("   empty.empty() =", empty.empty());

// 2. Create a vector from an array
console.log("\n2. Creating a vector from [1, 2, 3]:");
const v1 = PersistentVector.from([1, 2, 3]);
console.log("   v1.count() =", v1.count());
console.log("   v1.toArray() =", JSON.stringify(v1.toArray()));
console.log("   v1.first() =", v1.first());
console.log("   v1.last() =", v1.last());

// 3. Accessing elements with nth
console.log("\n3. Accessing elements with nth():");
console.log("   v1.nth(0) =", v1.nth(0));
console.log("   v1.nth(1) =", v1.nth(1));
console.log("   v1.nth(2) =", v1.nth(2));

// 4. Adding elements with conj (persistent operation)
console.log("\n4. Adding elements with conj() (returns new vector):");
const v2 = v1.conj(4);
console.log("   const v2 = v1.conj(4)");
console.log("   v1.toArray() =", JSON.stringify(v1.toArray()), "(original unchanged)");
console.log("   v2.toArray() =", JSON.stringify(v2.toArray()), "(new vector with 4 appended)");

// 5. Removing elements with pop (persistent operation)
console.log("\n5. Removing elements with pop() (returns new vector):");
const v3 = v2.pop();
console.log("   const v3 = v2.pop()");
console.log("   v2.toArray() =", JSON.stringify(v2.toArray()), "(original unchanged)");
console.log("   v3.toArray() =", JSON.stringify(v3.toArray()), "(new vector without last element)");

// 6. Replacing elements with assoc (persistent operation)
console.log("\n6. Replacing elements with assoc() (returns new vector):");
const v4 = v1.assoc(1, 100);
console.log("   const v4 = v1.assoc(1, 100)");
console.log("   v1.toArray() =", JSON.stringify(v1.toArray()), "(original unchanged)");
console.log("   v4.toArray() =", JSON.stringify(v4.toArray()), "(index 1 replaced with 100)");

// 7. Storing different value types
console.log("\n7. Storing different value types:");
const mixed = PersistentVector.from([42, "hello", true, null, { x: 1 }]);
console.log("   const mixed = PersistentVector.from([42, 'hello', true, null, {x: 1}])");
console.log("   mixed.nth(0) =", mixed.nth(0), "(number)");
console.log("   mixed.nth(1) =", mixed.nth(1), "(string)");
console.log("   mixed.nth(2) =", mixed.nth(2), "(boolean)");
console.log("   mixed.nth(3) =", mixed.nth(3), "(null)");
console.log("   mixed.nth(4) =", JSON.stringify(mixed.nth(4)), "(object)");

// 8. Chaining operations
console.log("\n8. Chaining operations:");
const result = PersistentVector.empty()
    .conj(1)
    .conj(2)
    .conj(3)
    .pop()
    .conj(4);
console.log("   PersistentVector.empty().conj(1).conj(2).conj(3).pop().conj(4)");
console.log("   result.toArray() =", JSON.stringify(result.toArray()));

// 9. Demonstrating structural sharing
console.log("\n9. Demonstrating structural sharing:");
const base = PersistentVector.from([1, 2, 3, 4, 5]);
const derived1 = base.conj(6);
const derived2 = base.conj(7);
console.log("   const base = PersistentVector.from([1, 2, 3, 4, 5])");
console.log("   const derived1 = base.conj(6)");
console.log("   const derived2 = base.conj(7)");
console.log("   base.toArray()     =", JSON.stringify(base.toArray()));
console.log("   derived1.toArray() =", JSON.stringify(derived1.toArray()));
console.log("   derived2.toArray() =", JSON.stringify(derived2.toArray()));
console.log("   (All three vectors share the same underlying [1,2,3,4,5] structure)");

// 10. Error handling
console.log("\n10. Error handling:");
try {
    v1.nth(100);
} catch (e) {
    console.log("   v1.nth(100) throws:", e.message);
}
try {
    v1.assoc(100, "x");
} catch (e) {
    console.log("   v1.assoc(100, 'x') throws:", e.message);
}

console.log("\n=== Demo Complete ===");
)JS";

// Read file contents into a string
std::string readFile(const char *path)
{
  std::ifstream file(path);
  if (!file.is_open())
  {
    throw std::runtime_error(std::string("Failed to open file: ") + path);
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// Install console object with log function
void installConsole(facebook::jsi::Runtime &runtime)
{
  auto console = facebook::jsi::Object(runtime);
  console.setProperty(
      runtime, "log",
      facebook::jsi::Function::createFromHostFunction(
          runtime, facebook::jsi::PropNameID::forAscii(runtime, "log"), 0,
          [](facebook::jsi::Runtime &rt, const facebook::jsi::Value &,
             const facebook::jsi::Value *args,
             size_t count) -> facebook::jsi::Value
          {
            for (size_t i = 0; i < count; ++i)
            {
              if (i > 0)
                std::cout << " ";
              if (args[i].isString())
              {
                std::cout << args[i].getString(rt).utf8(rt);
              }
              else if (args[i].isNumber())
              {
                double num = args[i].getNumber();
                // Check if number is effectively an integer
                if (std::floor(num) == num && num >= INT_MIN && num <= INT_MAX)
                {
                  std::cout << static_cast<int>(num);
                }
                else
                {
                  std::cout << num;
                }
              }
              else if (args[i].isBool())
              {
                std::cout << (args[i].getBool() ? "true" : "false");
              }
              else if (args[i].isNull())
              {
                std::cout << "null";
              }
              else if (args[i].isUndefined())
              {
                std::cout << "undefined";
              }
              else if (args[i].isObject())
              {
                std::cout << "[object]";
              }
            }
            std::cout << std::endl;
            return facebook::jsi::Value::undefined();
          }));
  runtime.global().setProperty(runtime, "console", console);
}

// Install performance.now() for benchmarking
void installPerformance(facebook::jsi::Runtime &runtime)
{
  auto performance = facebook::jsi::Object(runtime);
  performance.setProperty(
      runtime, "now",
      facebook::jsi::Function::createFromHostFunction(
          runtime, facebook::jsi::PropNameID::forAscii(runtime, "now"), 0,
          [](facebook::jsi::Runtime &, const facebook::jsi::Value &,
             const facebook::jsi::Value *,
             size_t) -> facebook::jsi::Value
          {
            auto now = std::chrono::high_resolution_clock::now();
            auto duration = now.time_since_epoch();
            auto millis = std::chrono::duration<double, std::milli>(duration).count();
            return facebook::jsi::Value(millis);
          }));
  runtime.global().setProperty(runtime, "performance", performance);
}

void printUsage(const char *programName)
{
  std::cout << "Usage: " << programName << " [options] [bundle.js]\n"
            << "\n"
            << "Options:\n"
            << "  --help, -h    Show this help message\n"
            << "\n"
            << "If no bundle is provided, runs the built-in JavaScript demo.\n"
            << "If a bundle path is provided, loads and executes that ClojureScript bundle.\n"
            << "\n"
            << "Example:\n"
            << "  " << programName << "                           # Run built-in demo\n"
            << "  " << programName << " cljs-out/main.js          # Run ClojureScript bundle\n";
}

int main(int argc, char *argv[])
{
  const char *bundlePath = nullptr;

  // Parse command line arguments
  for (int i = 1; i < argc; ++i)
  {
    std::string arg = argv[i];
    if (arg == "--help" || arg == "-h")
    {
      printUsage(argv[0]);
      return 0;
    }
    else if (arg[0] != '-')
    {
      if (bundlePath != nullptr)
      {
        std::cerr << "Error: Multiple bundle files specified\n";
        printUsage(argv[0]);
        return 1;
      }
      bundlePath = argv[i];
    }
    else
    {
      std::cerr << "Unknown option: " << arg << std::endl;
      printUsage(argv[0]);
      return 1;
    }
  }

  std::cout << "Initializing Hermes runtime...\n"
            << std::endl;

  // Create Hermes runtime
  auto runtimeConfig = ::hermes::vm::RuntimeConfig::Builder()
                           .withES6BlockScoping(true)
                           .build();
  auto runtime = facebook::hermes::makeHermesRuntime(runtimeConfig);

  // Install console.log for output
  installConsole(*runtime);

  // Install performance.now() for benchmarking
  installPerformance(*runtime);

  // Install PersistentVector
  cljs::installPersistentVector(*runtime);
  cljs::installPersistentMap(*runtime);

  try
  {
    if (bundlePath)
    {
      // Load and run the ClojureScript bundle
      std::cout << "Loading ClojureScript bundle: " << bundlePath << "\n"
                << std::endl;
      std::string bundleCode = readFile(bundlePath);
      runtime->evaluateJavaScript(
          std::make_shared<facebook::jsi::StringBuffer>(bundleCode),
          bundlePath);
    }
    else
    {
      // Run the built-in demo script
      runtime->evaluateJavaScript(
          std::make_shared<facebook::jsi::StringBuffer>(kDemoScript),
          "persistent-vector-demo.js");
    }
  }
  catch (const facebook::jsi::JSError &e)
  {
    std::cerr << "JavaScript error: " << e.getStack() << std::endl;
    return 1;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
