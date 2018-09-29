Pack flashes as Arduino library.
notes 28.9.2018/pekka

The cmake file CMakeLists.txt in this directory packs source code as Arduino libary .zip file. 
The resulting library is intended to be used with stmduino (STM32F4xx microcontrollers). 

Usage:
  mkdir tmp
  cd tmp
  cmake ..
  make

The resulting .zip file will be in lib/arduino directory.
