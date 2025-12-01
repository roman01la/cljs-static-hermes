# Note: Native libraries (cimgui, sokol, stb, soloud) are built in external/
# Note: Hermes configuration is in root CMakeLists.txt
# Note: jslib-unit, imgui-unit, and skia-unit are built in lib/

# Set up bundle path
set(REACT_UNIT_BUNDLE ${CMAKE_CURRENT_BINARY_DIR}/react-unit-bundle.js)

# Collect app source files automatically
file(GLOB_RECURSE APP_FILES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.jsx
    ${CMAKE_CURRENT_SOURCE_DIR}/*.js
)

# Make sure the entry point is listed with an absolute path
set(ENTRY_POINT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/out/main.js)
list(APPEND REACT_UNIT_DEPS ${ENTRY_POINT_PATH})

# Check if npm install has been run
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/../node_modules")
    message(FATAL_ERROR "node_modules/ directory not found. Please run 'npm install' in the project root before building.")
endif()

# Build dependency list
set(REACT_UNIT_DEPS
    ${RECONCILER_FILES}
    ${APP_FILES}
    ${CMAKE_SOURCE_DIR}/scripts/bundle-react-unit.js
)

# Bundle with esbuild
add_custom_command(OUTPUT ${REACT_UNIT_BUNDLE}
    COMMAND node ${CMAKE_SOURCE_DIR}/scripts/bundle-react-unit.js
        ${ENTRY_POINT_PATH}
        ${REACT_UNIT_BUNDLE}
        $<IF:$<CONFIG:Debug>,development,production>
    DEPENDS ${REACT_UNIT_DEPS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Bundling skia React unit with esbuild (NODE_ENV=$<IF:$<CONFIG:Debug>,development,production>)"
)

message(STATUS "skia: React bundle path: ${REACT_UNIT_BUNDLE}")

# Build the custom Skia+Hermes application
add_executable(skia core.cpp ${REACT_UNIT_BUNDLE})

# Link against all required libraries
target_link_libraries(skia
    glfw
    jslib-unit
    skia-unit
    native-support
    native-cljs
    websockets
    yoga
    "-framework OpenGL"
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreFoundation"
    "-framework QuartzCore"
)

# Link against Hermes
target_link_directories(skia PRIVATE
    ${HERMES_BUILD}/lib
    ${HERMES_BUILD}/jsi
    ${HERMES_BUILD}/external/boost/boost_1_86_0/libs/context
)
target_link_libraries(skia
    $<$<CONFIG:Release>:hermesvm_a jsi boost_context>
    $<$<CONFIG:Debug>:hermesvm>
)

# Add GLFW and Skia include directories
target_include_directories(skia PRIVATE
    ${CMAKE_SOURCE_DIR}/external/glfw/glfw/include
    ${CMAKE_SOURCE_DIR}/external/skia/skia
    ${CMAKE_SOURCE_DIR}/lib/native-support
    ${CMAKE_SOURCE_DIR}/lib/native
)

if(NOT DEFINED REACT_BUNDLE_MODE)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(REACT_BUNDLE_MODE 0)
    else()
        set(REACT_BUNDLE_MODE 2)
    endif()
endif()

# Compile based on REACT_BUNDLE_MODE
if(REACT_BUNDLE_MODE EQUAL 0)
    # Mode 0: Native compilation with shermes (slowest build, fastest runtime)
    set(REACT_UNIT_O react-unit${CMAKE_C_OUTPUT_EXTENSION})
    hermes_compile_native(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${REACT_UNIT_O}
        SOURCES ${REACT_UNIT_BUNDLE}
        UNIT_NAME react
        DEPENDS ${REACT_UNIT_BUNDLE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Compiling React unit to native code"
    )
    set(REACT_UNIT_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${REACT_UNIT_O})

elseif(REACT_BUNDLE_MODE EQUAL 1)
    # Mode 1: Bytecode compilation with hermes (medium build/runtime speed)
    set(REACT_UNIT_HBC ${CMAKE_CURRENT_BINARY_DIR}/react-unit-bundle.hbc)
    hermes_compile_bytecode(
        OUTPUT ${REACT_UNIT_HBC}
        SOURCE ${REACT_UNIT_BUNDLE}
        SOURCE_MAP ${REACT_UNIT_BUNDLE}.map
        DEPENDS ${REACT_UNIT_BUNDLE}
    )
    set(REACT_UNIT_OUTPUT ${REACT_UNIT_HBC})

elseif(REACT_BUNDLE_MODE EQUAL 2)
    # Mode 2: Source bundle only (fastest build, slowest runtime)
    set(REACT_UNIT_OUTPUT ${REACT_UNIT_BUNDLE})

else()
    message(FATAL_ERROR "Invalid REACT_BUNDLE_MODE: ${REACT_BUNDLE_MODE}. Must be 0, 1, or 2.")
endif()

# Link the React unit output if available
if(REACT_BUNDLE_MODE EQUAL 0)
    target_sources(skia PRIVATE ${REACT_UNIT_OUTPUT})
else()
    add_custom_target(skia_react_unit DEPENDS ${REACT_UNIT_OUTPUT})
    add_dependencies(skia skia_react_unit)
endif()

# Set compile definitions for Skia example
target_compile_definitions(skia PRIVATE
    REACT_BUNDLE_MODE=${REACT_BUNDLE_MODE}
    REACT_BUNDLE_PATH="${REACT_UNIT_BUNDLE}"
)
