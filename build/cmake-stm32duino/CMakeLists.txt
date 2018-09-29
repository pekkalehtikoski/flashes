cmake_minimum_required(VERSION 2.8.11)

set(STM32_PROJECT "flashes-stm32duino")
include(../../../eosal/build/cmake-stm32duino/stm32duino-defs.txt)

add_subdirectory(../cmake "${CMAKE_CURRENT_BINARY_DIR}/${STM32_PROJECT}-build-tmp")
project(${STM32_PROJECT})
