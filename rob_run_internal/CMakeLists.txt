cmake_minimum_required(VERSION 3.20)
project(rob_run_internal LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Windows-only internal cheat DLL — no third-party SDKs. ImGui is vendored at
# deps/imgui (the standard ImGui source tree — imgui.cpp, imgui_impl_dx11.cpp,
# imgui_impl_win32.cpp are prerequisites). If you don't have a copy, grab one
# from github.com/ocornut/imgui.
# ---------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)

if(MSVC)
  add_compile_options(/W4 /permissive- /Ob2 /utf-8 /MD)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui" CACHE PATH "Vendored ImGui sources")
if(NOT EXISTS "${IMGUI_DIR}/imgui.cpp")
  message(FATAL_ERROR "Vendored ImGui not found. Drop a fresh ImGui checkout at "
                      "${IMGUI_DIR} (files: imgui.cpp, imgui_impl_dx11.cpp, imgui_impl_win32.cpp).")
endif()

add_library(rob_run_internal SHARED
    main.cpp
    aimbot.hpp
    bone_probe.hpp
    combat.hpp
    dxgi_hook.hpp
    entity.hpp
    esp.hpp
    evasion.hpp
    features.hpp
    il2cpp_wrapper.hpp
    offsets.hpp
    unity_types.hpp
)
target_include_directories(rob_run_internal PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# ImGui 1.90.9 vendored at deps/imgui (DX11 + Win32 backends).
target_sources(rob_run_internal PRIVATE
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
)

target_link_libraries(rob_run_internal PRIVATE d3d11 dxgi user32)

set_target_properties(rob_run_internal PROPERTIES
    PREFIX ""
    OUTPUT_NAME "rob_run_internal"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

install(TARGETS rob_run_internal RUNTIME DESTINATION rob_run_internal LIBRARY DESTINATION rob_run_internal)

# ---------------------------------------------------------------------------
# Test procedure (see README.md):
#   1. cmake -S . -B build -G "Visual Studio 17 2022" -A x64
#   2. cmake --build build --config Release
#   3. Launch ROB RUN (Steam appid 4262460, BUILD_ID 26807668), wait for swapchain/menu.
#   4. Load rob_run_internal.dll by an authorized local test method you own.
#   5. Verify overlay first: Insert toggles ImGui menu.
#   6. Enable ESP only after entity/camera runtime markers are validated.
#   7. Enable aim/combat only after fire/raycast/matrix markers are validated.
#   8. Unload via StopCheat export; confirm Present/WndProc restored.
# ---------------------------------------------------------------------------
