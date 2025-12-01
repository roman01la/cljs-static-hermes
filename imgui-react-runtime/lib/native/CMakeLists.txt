# Copyright (c) Tzvetan Mikov and contributors
# SPDX-License-Identifier: MIT
# See LICENSE file for full license text

# Native data structures library for ClojureScript/Hermes integration
# Provides persistent data structures using Immer library

add_library(native-cljs
    PersistentVector.cpp
    PersistentVector.h
    PersistentMap.cpp
    PersistentMap.h
)

# Include Immer headers (header-only library)
target_include_directories(native-cljs PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add Hermes include directories
target_include_directories(native-cljs PRIVATE
    ${CMAKE_BINARY_DIR}/hermes/lib/config
    ${CMAKE_BINARY_DIR}/hermes-src/include
    ${CMAKE_BINARY_DIR}/hermes-src/public
    ${CMAKE_BINARY_DIR}/hermes-src/API
    ${CMAKE_BINARY_DIR}/hermes-src/API/jsi
)

# Immer headers need to be found via the 'immer' subdirectory
target_include_directories(native-cljs SYSTEM PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_directories(native-cljs INTERFACE
    ${CMAKE_BINARY_DIR}/hermes/lib
    ${CMAKE_BINARY_DIR}/hermes/jsi
)

# Link against Hermes for JSI support
target_link_libraries(native-cljs
    $<$<CONFIG:Release>:hermesvm_a jsi>
    $<$<CONFIG:Debug>:hermesvm>
)

# Ensure Hermes is built before this library
add_dependencies(native-cljs hermes)
