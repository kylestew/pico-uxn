cmake_minimum_required(VERSION 3.13...3.27)

include(pico-sdk/pico_sdk_init.cmake)

project(pico_uxn C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

pico_sdk_init()

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/rom.h
    COMMAND xxd -i test.rom ${CMAKE_CURRENT_BINARY_DIR}/rom.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test.rom
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating rom.h from test.rom"
)

add_executable(pico_uxn
    src/main.c
    src/uxn.c
    src/sharp_display.c
)

target_sources(pico_uxn PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/rom.h)
target_include_directories(pico_uxn PRIVATE ${CMAKE_CURRENT_BINARY_DIR} src)

pico_enable_stdio_usb(pico_uxn 1)
pico_enable_stdio_uart(pico_uxn 1)
pico_add_extra_outputs(pico_uxn)

target_link_libraries(pico_uxn pico_stdlib hardware_spi)

add_custom_command(TARGET pico_uxn POST_BUILD
    COMMAND arm-none-eabi-size $<TARGET_FILE:pico_uxn>
    COMMENT "Memory usage:"
)
