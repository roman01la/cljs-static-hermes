# GLFW - Multi-platform library for OpenGL, window and input handling
# https://www.glfw.org/

# Disable GLFW-specific options we don't need
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target" FORCE)

# Use static library
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

# Platform-specific configuration
if(APPLE)
    set(GLFW_BUILD_COCOA ON CACHE BOOL "Build support for Cocoa" FORCE)
elseif(WIN32)
    set(GLFW_BUILD_WIN32 ON CACHE BOOL "Build support for Win32" FORCE)
elseif(UNIX)
    set(GLFW_BUILD_X11 ON CACHE BOOL "Build support for X11" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "Build support for Wayland" FORCE)
endif()

# Add GLFW as subdirectory
add_subdirectory(glfw)

# Expose GLFW for downstream targets
# The GLFW target 'glfw' is automatically created by the GLFW CMakeLists.txt
