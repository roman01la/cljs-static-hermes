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

# Set compile definitions for Skia example
target_compile_definitions(skia PRIVATE
    REACT_BUNDLE_MODE=2
    REACT_BUNDLE_PATH="${REACT_UNIT_BUNDLE}"
)
