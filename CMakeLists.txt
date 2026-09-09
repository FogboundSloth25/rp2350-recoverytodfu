cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(rp2350_recovery C CXX ASM)
set(CMAKE_BUILD_TYPE MinSizeRel)
set(BOARD_DEFINITION ${CMAKE_SOURCE_DIR}/boards/${PICO_BOARD}.cmake)
if(NOT EXISTS ${BOARD_DEFINITION})
  set(BOARD_DEFINITION ${CMAKE_SOURCE_DIR}/boards/unknown.cmake)
endif()
include(${BOARD_DEFINITION} OPTIONAL)
if(NOT DEFINED BOARD_NAME)
  add_compile_definitions(BOARD_NAME="Unknown RP2350")
endif()
pico_sdk_init()

add_executable(rp2350_recovery
  src/main.c
  src/state_machine.c
  src/apple_usb.c
  src/status_led.c
)
target_compile_definitions(rp2350_recovery PRIVATE PICO_STACK_SIZE=0x1000)
pico_enable_stdio_uart(rp2350_recovery 0)
pico_enable_stdio_usb(rp2350_recovery 1)

add_subdirectory(pio_usb)

target_link_libraries(rp2350_recovery
  pico_stdlib
  pico_status_led
  pico_multicore
  pio_usb
  hardware_pio
  hardware_dma
)
pico_add_extra_outputs(rp2350_recovery)
