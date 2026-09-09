cmake_minimum_required(VERSION 3.13)
set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/pico-sdk CACHE PATH "Path to pico-sdk")
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(rp2350_recovery C CXX ASM)
set(LED_PIN 16 CACHE STRING "WS2812 data GPIO (-1 disables status LED)")
set(LED_RGB_ORDER_GRB 1 CACHE STRING "Use GRB packing for WS2812 (1=yes, 0=no)")
pico_sdk_init()
add_executable(rp2350_recovery
    src/main.c
    src/state_machine.c
    src/apple_usb.c
    src/status_led.c
)
pico_generate_pio_header(rp2350_recovery ${CMAKE_CURRENT_LIST_DIR}/src/ws2812.pio)
target_include_directories(rp2350_recovery PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
target_compile_definitions(rp2350_recovery PRIVATE
    PICO_STDIO_USB=1
    CFG_TUSB_OS=OPT_OS_NONE
    CFG_TUH_ENABLED=1
    CFG_TUD_ENABLED=0
    CFG_TUH_MAX_SPEED=OPT_MODE_FULL_SPEED
    LED_STATE_ON=1
    LED_PIN=${LED_PIN}
    LED_RGB_ORDER_GRB=${LED_RGB_ORDER_GRB}
)
target_link_libraries(rp2350_recovery pico_stdlib tinyusb_host tinyusb_board hardware_pio)
pico_enable_stdio_usb(rp2350_recovery 1)
pico_enable_stdio_uart(rp2350_recovery 0)
pico_add_extra_outputs(rp2350_recovery)
