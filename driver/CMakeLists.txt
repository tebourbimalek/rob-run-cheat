# Driver tool — manual-map loader SCAFFOLD ONLY.
# No BYOVD loading, no kernel primitives, no remote map/injection shipped.
# Windows-only. Build via MSVC:
#   cmake -S driver -B driver\build -G "Visual Studio 17 2022" -A x64
#   cmake --build driver\build --config Release
cmake_minimum_required(VERSION 3.20)
project(robcheat_mapper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)

if(MSVC)
  add_compile_options(/W3 /MD)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

add_executable(mapper_main WIN32
    mapper.cpp
    vuln_driver.cpp
)
target_include_directories(mapper_main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mapper_main PRIVATE Shlwapi)
set_target_properties(mapper_main PROPERTIES OUTPUT_NAME "mapper_main"
                      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
