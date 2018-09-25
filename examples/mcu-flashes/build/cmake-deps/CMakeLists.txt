# mcu-flashes/build/cmake-deps/CmakeLists.txt - cmake build for mcu-flashes example + dependencies.
cmake_minimum_required(VERSION 2.8.11)
set(E_PROJECT "mcu-flashes-deps")
project(${E_PROJECT})

# include build information common to all projects (only to get E_ROOT).
include(../../../../../eosal/build/cmake/eosal-defs.txt)

# Build individual projects.
add_subdirectory($ENV{E_ROOT}/eosal/build/cmake "${CMAKE_CURRENT_BINARY_DIR}/eosal")
add_subdirectory($ENV{E_ROOT}/flashes/build/cmake "${CMAKE_CURRENT_BINARY_DIR}/flashes")
add_subdirectory($ENV{E_ROOT}/flashes/examples/mcu-flashes/build/cmake "${CMAKE_CURRENT_BINARY_DIR}/mcu-flashes")

