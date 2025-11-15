cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)
pico_sdk_init()

project(pico_ultrasound)

add_executable(pico_ultrasound
    src/main.cpp
)

pico_generate_pio_header(pico_ultrasound ${CMAKE_CURRENT_LIST_DIR}/src/chirp.pio)

target_link_libraries(pico_ultrasound
    pico_stdlib
    pico_time
    pico_multicore
    hardware_adc
    hardware_dma
    hardware_irq
    hardware_pio
)

pico_enable_stdio_usb(pico_ultrasound 1)
pico_enable_stdio_uart(pico_ultrasound 0)
# pico_set_binary_type(pico_ultrasound no_flash)
pico_add_extra_outputs(pico_ultrasound)
