# ioboard2/build/cmake-deps/CmakeLists.txt - cmake build for iocontroller1 + dependencies.
cmake_minimum_required(VERSION 2.8.11)
set(E_PROJECT "iocontroller1-deps")
project(${E_PROJECT})

# include build information common to all projects (only to get E_ROOT).
include(../../../../../eosal/build/cmake/eosal-defs.txt)

# Build individual projects.
add_subdirectory($ENV{E_ROOT}/eosal/build/cmake "${CMAKE_CURRENT_BINARY_DIR}/eosal")
add_subdirectory($ENV{E_ROOT}/iocom/build/cmake "${CMAKE_CURRENT_BINARY_DIR}/iocom")
add_subdirectory($ENV{E_ROOT}/iocom/examples/iocontroller1/build/cmake "${CMAKE_CURRENT_BINARY_DIR}/iocontroller1")

