cmake_minimum_required(VERSION 3.13)

add_executable(usb_camera
    main.c
    usb_descriptors.c
)

target_include_directories(usb_camera PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(usb_camera PRIVATE tinyusb_device tinyusb_board pico_stdlib pico_hm01b0)

pico_add_extra_outputs(usb_camera)
