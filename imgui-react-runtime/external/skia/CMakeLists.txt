# Skia - 2D graphics library
# https://skia.org/

# Determine the target CPU architecture
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    set(SKIA_TARGET_CPU "arm64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    set(SKIA_TARGET_CPU "x64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86|i686")
    set(SKIA_TARGET_CPU "x86")
else()
    set(SKIA_TARGET_CPU "x64")
    message(WARNING "Unknown CPU architecture, defaulting to x64")
endif()

# Determine build configuration for gn
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SKIA_IS_DEBUG "true")
else()
    set(SKIA_IS_DEBUG "false")
endif()

set(SKIA_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/skia")
set(SKIA_BUILD_DIR "${SKIA_SOURCE_DIR}/out/Static")

# Check if skia has been built
if(NOT EXISTS "${SKIA_BUILD_DIR}/libskia.a" AND NOT EXISTS "${SKIA_BUILD_DIR}/skia.lib")
    message(STATUS "Skia build not found. Running Skia build configuration...")
    
    # Sync dependencies first
    execute_process(
        COMMAND python3 tools/git-sync-deps
        WORKING_DIRECTORY "${SKIA_SOURCE_DIR}"
        RESULT_VARIABLE SYNC_RESULT
    )
    
    if(NOT SYNC_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to sync Skia dependencies")
    endif()
    
    # Configure Skia with gn
    set(GN_ARGS 
        "is_official_build=true"
        "is_component_build=false"
        "skia_use_system_libpng=false"
        "skia_use_system_libjpeg_turbo=false"
        "skia_enable_pdf=false"
        "skia_use_system_libwebp=false"
        "target_cpu=\"${SKIA_TARGET_CPU}\""
    )
    
    # Convert list to space-separated string
    string(REPLACE ";" " " GN_ARGS_STR "${GN_ARGS}")
    
    execute_process(
        COMMAND bash -c "bin/gn gen out/Static --args='${GN_ARGS_STR}'"
        WORKING_DIRECTORY "${SKIA_SOURCE_DIR}"
        RESULT_VARIABLE GN_RESULT
    )
    
    if(NOT GN_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to configure Skia with gn")
    endif()
    
    # Build Skia with ninja
    execute_process(
        COMMAND ninja -C out/Static skia
        WORKING_DIRECTORY "${SKIA_SOURCE_DIR}"
        RESULT_VARIABLE NINJA_RESULT
    )
    
    if(NOT NINJA_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to build Skia with ninja")
    endif()
endif()

# Determine the static library file based on platform
if(APPLE)
    set(SKIA_LIB "${SKIA_BUILD_DIR}/libskia.a")
elseif(WIN32)
    set(SKIA_LIB "${SKIA_BUILD_DIR}/skia.lib")
else()
    set(SKIA_LIB "${SKIA_BUILD_DIR}/libskia.a")
endif()

# Create imported library target
add_library(skia-lib STATIC IMPORTED GLOBAL)

set_target_properties(skia-lib PROPERTIES
    IMPORTED_LOCATION "${SKIA_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${SKIA_SOURCE_DIR}"
)

# Add dependency libraries that Skia needs
# These are bundled with Skia when built as static
set(SKIA_DEPENDENCY_LIBS
    "${SKIA_BUILD_DIR}/libpng.a"
    "${SKIA_BUILD_DIR}/libjpeg.a"
    "${SKIA_BUILD_DIR}/libwebp.a"
    "${SKIA_BUILD_DIR}/libdng_sdk.a"
    "${SKIA_BUILD_DIR}/libskcms.a"
    "${SKIA_BUILD_DIR}/libwuffs.a"
)

# Link system zlib (for compression functions)
find_package(ZLIB REQUIRED)

set_target_properties(skia-lib PROPERTIES
    INTERFACE_LINK_LIBRARIES "${SKIA_DEPENDENCY_LIBS};ZLIB::ZLIB"
)

message(STATUS "Skia library: ${SKIA_LIB}")
message(STATUS "Skia headers: ${SKIA_SOURCE_DIR}")
