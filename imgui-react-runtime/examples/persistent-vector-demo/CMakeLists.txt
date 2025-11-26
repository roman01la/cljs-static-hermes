# Copyright (c) Tzvetan Mikov and contributors
# SPDX-License-Identifier: MIT
# See LICENSE file for full license text

# PersistentVector Demo - A standalone console application demonstrating
# PersistentVector functionality without GUI dependencies.

add_executable(persistent-vector-demo main.cpp)

# Include directories
target_include_directories(persistent-vector-demo PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/native
)

# Link directories for Hermes
target_link_directories(persistent-vector-demo PRIVATE
    ${HERMES_BUILD}/lib
    ${HERMES_BUILD}/jsi
)

# Link libraries
target_link_libraries(persistent-vector-demo
    native-cljs
    $<$<CONFIG:Release>:hermesvm_a jsi>
    $<$<CONFIG:Debug>:hermesvm>
    $<$<PLATFORM_ID:Linux>:icuuc icui18n icudata>
)
