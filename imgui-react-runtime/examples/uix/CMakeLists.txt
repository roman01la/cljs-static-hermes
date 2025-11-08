# Note: Native libraries (cimgui, sokol, stb, soloud) are built in external/
# Note: Hermes configuration is in root CMakeLists.txt
# Note: jslib-unit and imgui-unit are built in lib/

# Build the UIx React+ImGui application
add_react_imgui_app(
    TARGET uix
    ENTRY_POINT out/main.js
    SOURCES core.cpp
)
