if (TARGET tinyusb_device)
    add_executable(Thermostat
            main.c
            seven_seg.h
            seven_seg.c
            i2c_module.h
            i2c_module.c         
            aht20.h
            aht20.c  
            )

    # pull in common dependencies
    target_link_libraries(Thermostat pico_stdlib freertos hardware_i2c)

    # enable usb output, disable uart output
    pico_enable_stdio_usb(Thermostat 1)
    pico_enable_stdio_uart(Thermostat 0)

    # create map/bin/hex/uf2 file etc.
    pico_add_extra_outputs(Thermostat)

    # add url via pico_set_program_url
#     example_auto_set_url(Thermostat)
elseif(PICO_ON_DEVICE)
    message(WARNING "not building Thermostat because TinyUSB submodule is not initialized in the SDK")
endif()
