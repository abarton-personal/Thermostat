# Thermostat
Raspberry Pi Pico, FreeRTOS, AHT20, seven segment display

## Purpose
I wanted to replace the shitty mechanical thermostat in my apartment, and learn FreeRTOS at the same time.

## Folder structure

Parent folder. Mine is named "pico"

Inside pico is
- pico/sdk (clone from https://github.com/raspberrypi/pico-sdk)
- pico/Thermostat
    - CMakeLists.txt
    - pico_sdk_import.cmake
    - README.md (this)
    - build
    - ProjectFiles
    - FreeRTOS
        - CMakeLists.txt
        - FreeRTOSConfig.h
        FreeRTOS-Kernel (clone from https://github.com/FreeRTOS/FreeRTOS-Kernel)

The project is based on this tutorial: https://learnembeddedsystems.co.uk/freertos-on-rp2040-boards-pi-pico-etc-using-vscode


