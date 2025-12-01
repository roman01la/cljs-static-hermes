# Copyright (c) Tzvetan Mikov and contributors
# SPDX-License-Identifier: MIT
# See LICENSE file for full license text

# Skia unit - Graphics rendering with Skia
# Provides Skia rendering functionality using Static Hermes

set(SKIA_UNIT_O skia-unit${CMAKE_C_OUTPUT_EXTENSION})
hermes_compile_native(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SKIA_UNIT_O}
    SOURCES
        ../imgui-unit/ffi_helpers.js
        ../imgui-unit/asciiz.js
        skia_externs.js
        yoga_layout.js
        renderer.js
        main.js
    UNIT_NAME skia
    FLAGS -typed -Wc,-I.
)

add_library(skia-unit STATIC skia_externs_cwrap.cpp ${CMAKE_CURRENT_BINARY_DIR}/${SKIA_UNIT_O})
set_target_properties(skia-unit PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(skia-unit skia-lib yoga)

# Ensure Hermes is built before compiling this unit
add_dependencies(skia-unit hermes)
