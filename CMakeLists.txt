cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pico_hm01b0)

pico_sdk_init()

add_library(pico_hm01b0 INTERFACE)

target_sources(pico_hm01b0 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/hm01b0.c
)

target_include_directories(pico_hm01b0 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/include
)

target_link_libraries(pico_hm01b0 INTERFACE
    cmsis_core
    hardware_dma
    hardware_i2c
    hardware_pio
    hardware_pwm
)

add_subdirectory("examples/serial_terminal")
add_subdirectory("examples/usb_camera")
