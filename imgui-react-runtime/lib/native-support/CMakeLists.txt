# Copyright (c) Tzvetan Mikov and contributors
# SPDX-License-Identifier: MIT
# See LICENSE file for full license text

# Shared native support library for WebSocket and file mapping
# Used by imgui-runtime and skia examples

add_library(native-support STATIC
    WebSocketSupport.cpp
    WebSocketSupport.h
    MappedFileBuffer.cpp
    MappedFileBuffer.h
)

target_include_directories(native-support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(native-support PUBLIC websockets)

# Add Hermes include directories
target_include_directories(native-support PRIVATE
    ${CMAKE_BINARY_DIR}/hermes/lib/config
    ${CMAKE_BINARY_DIR}/hermes-src/include
    ${CMAKE_BINARY_DIR}/hermes-src/public
    ${CMAKE_BINARY_DIR}/hermes-src/API
    ${CMAKE_BINARY_DIR}/hermes-src/API/jsi
)

# Add libwebsockets include directories
target_include_directories(native-support PRIVATE
    ${CMAKE_SOURCE_DIR}/external/libwebsockets/libwebsockets/lib/../include
    ${CMAKE_BINARY_DIR}/external/libwebsockets/libwebsockets/lib/../include
)

# Ensure Hermes is built before this library
add_dependencies(native-support hermes)
