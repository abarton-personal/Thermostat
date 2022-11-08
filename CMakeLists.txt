cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

project(Thermostat)

pico_sdk_init()

add_subdirectory(FreeRTOS)
add_subdirectory(ProjectFiles)
