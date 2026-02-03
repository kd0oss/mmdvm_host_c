cmake_minimum_required(VERSION 3.16)
project(mmdvm_host_c LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_compile_options(-Wall -Wextra -Werror=return-type)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(core_c
  src/modem_posix.c
  src/ringbuf.c
  src/repeater_control.c
  src/thread_setup.c
  src/rx_thread.c
  src/tx_thread.c
  src/mode_dmr.c
  src/mode_ysf.c
  src/mode_nxdn.c
  src/set_config.c
  src/log.c               # NEW
  src/time_util.c            # NEW
  src/dmr_csbk.c
)

add_library(adapters_cpp
  src/config_loader.cpp
  src/dmr_csbk_port.cpp
  third_party/mmdvmhost/BPTC19696.cpp
  third_party/mmdvmhost/CRC.cpp
  third_party/mmdvmhost/DMRCSBK.cpp
  third_party/mmdvmhost/Utils.cpp
  third_party/mmdvmhost/Hamming.cpp
)

target_include_directories(adapters_cpp PRIVATE ${CMAKE_SOURCE_DIR}/third_party/mmdvmhost)

# Enable GNU extensions (CPU_ZERO/CPU_SET) for this target
target_compile_definitions(core_c PRIVATE _GNU_SOURCE _POSIX_C_SOURCE=200809L)
target_link_libraries(core_c PUBLIC pthread rt)

target_compile_options(adapters_cpp PRIVATE -fno-rtti)
target_link_libraries(adapters_cpp PUBLIC pthread)

add_executable(mmdvm_host_c
  src/main.c
)
target_link_libraries(mmdvm_host_c PRIVATE core_c adapters_cpp)
