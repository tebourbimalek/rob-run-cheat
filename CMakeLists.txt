cmake_minimum_required(VERSION 3.20)

project(robrun VERSION 1.0.0 LANGUAGES CXX)

# x64 is mandatory — the chain and the offsets are sized for it.
if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "x64 required: re-run with -A x64")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
  add_compile_options(/permissive- /W4 /utf-8)
endif()

# Products of the new structure:
#   rob_run_internal/ — the in-game cheat DLL (DXGI hook, il2cpp, aimbot).
#   driver/           — the BYOVD loader + manual-map exe.
add_subdirectory(rob_run_internal)
add_subdirectory(driver)
