# ioboard2/build/cmake-deps/CmakeLists.txt - cmake build for ioboard2 + dependencies.
cmake_minimum_required(VERSION 2.8.11)
set(E_PROJECT "ioboard2-deps")
project(${E_PROJECT})

# include build information common to all projects (only to get E_ROOT).
include(../../../../../eosal/build/cmake/eosal-defs.txt)

# Build individual projects.
add_subdirectory($ENV{E_ROOT}/eosal/build/cmake "${CMAKE_CURRENT_BINARY_DIR}/eosal")
add_subdirectory($ENV{E_ROOT}/iocom/build/cmake "${CMAKE_CURRENT_BINARY_DIR}/iocom")
add_subdirectory($ENV{E_ROOT}/iocom/examples/ioboard2/build/cmake "${CMAKE_CURRENT_BINARY_DIR}/ioboard2")

