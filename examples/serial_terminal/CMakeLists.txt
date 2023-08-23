cmake_minimum_required(VERSION 3.13)

add_executable(serial_terminal
    main.c
)

target_link_libraries(serial_terminal PRIVATE pico_stdlib pico_hm01b0)

# enable usb output, disable uart output
pico_enable_stdio_usb(serial_terminal 1)
pico_enable_stdio_uart(serial_terminal 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(serial_terminal)
