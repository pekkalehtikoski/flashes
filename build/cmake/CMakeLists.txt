# flashes/build/cmake/CmakeLists.txt - Cmake build for iocom library.
cmake_minimum_required(VERSION 2.8.11)

# Set project name (= project root folder name).
set(E_PROJECT "flashes")
project(${E_PROJECT})

# include build environment setting common to all projects and for iocom library.
include(../../../eosal/build/cmake/eosal-defs.txt)
include(flashes-defs.txt)

# Set path to where to keep libraries.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $ENV{E_LIB})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $ENV{E_LIB})

# Set path to source files.
set(BACKBONE "$ENV{E_ROOT}/${E_PROJECT}/code")

# Add main headers.
set(MAIN_HEADERS "$ENV{E_ROOT}/${E_PROJECT}/${E_PROJECT}.h")

# Add source files common to all operating systems.
file(GLOB BB_CM_SOURCES "${BACKBONE}/common/*.c")
file(GLOB BB_CM_HEADERS "${BACKBONE}/common/*.h")

# Add operating system specific files.
file(GLOB BB_OS_SOURCES "${BACKBONE}/$ENV{E_OS}/*.c" "${BACKBONE}/*/$ENV{E_OS}/*.cpp")
file(GLOB BB_OS_HEADERS "${BACKBONE}/$ENV{E_OS}/*.h")

# Generate the static library
add_library(${E_PROJECT}${E_POSTFIX} STATIC ${MAIN_HEADERS} ${BB_CM_HEADERS} ${BB_OS_HEADERS} ${BB_CM_SOURCES} ${BB_OS_SOURCES})
