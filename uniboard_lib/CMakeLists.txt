add_library(uniboard_lib STATIC util.c lcd_1602_i2c.c onewire_library.c ws2812.c usb_descriptors.c)
target_include_directories(uniboard_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

pico_generate_pio_header(uniboard_lib ${CMAKE_CURRENT_LIST_DIR}/onewire_library.pio)

target_link_libraries(uniboard_lib PUBLIC
        pico_stdlib
        hardware_pio
        hardware_i2c
        hardware_gpio
        hardware_pwm
        hardware_dma
        hardware_adc
        hardware_timer
        tinyusb_device
        tinyusb_board
        )


# target_sources(onewire_library INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/onewire_library.c)
