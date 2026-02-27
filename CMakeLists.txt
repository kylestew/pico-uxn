cmake_minimum_required(VERSION 3.13...3.27)

include(pico-sdk/pico_sdk_init.cmake)

project(pico_uxn C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

pico_sdk_init()

# --- Self-hosted assembler pipeline ---
# Assembles a .tal test file into a ROM at build time using drifblim (a Uxn
# assembler ROM) run on a host-compiled uxn2 emulator.

set(TEST_TAL "opctest" CACHE STRING "Name of .tal file in etc/tests/ to assemble (without extension)")

execute_process(COMMAND sdl2-config --cflags OUTPUT_VARIABLE SDL2_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND sdl2-config --libs OUTPUT_VARIABLE SDL2_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
separate_arguments(SDL2_CFLAGS)
separate_arguments(SDL2_LIBS)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/uxn2
    COMMAND cc -DNDEBUG -O2
            ${SDL2_CFLAGS}
            ${CMAKE_CURRENT_SOURCE_DIR}/etc/utils/uxn2.c
            -o ${CMAKE_CURRENT_BINARY_DIR}/uxn2
            ${SDL2_LIBS}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/etc/utils/uxn2.c
    COMMENT "Building host uxn2 emulator"
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/drifblim.rom
    COMMAND xxd -r -p ${CMAKE_CURRENT_SOURCE_DIR}/etc/utils/drifblim.rom.txt
                      ${CMAKE_CURRENT_BINARY_DIR}/drifblim.rom
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/etc/utils/drifblim.rom.txt
    COMMENT "Converting drifblim.rom.txt -> drifblim.rom"
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/prog.rom
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/uxn2
            ${CMAKE_CURRENT_BINARY_DIR}/drifblim.rom
            ${CMAKE_CURRENT_SOURCE_DIR}/etc/tests/${TEST_TAL}.tal
            ${CMAKE_CURRENT_BINARY_DIR}/prog.rom
    DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/uxn2
        ${CMAKE_CURRENT_BINARY_DIR}/drifblim.rom
        ${CMAKE_CURRENT_SOURCE_DIR}/etc/tests/${TEST_TAL}.tal
    COMMENT "Assembling etc/tests/${TEST_TAL}.tal -> prog.rom"
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/rom.h
    COMMAND xxd -i prog.rom rom.h
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/prog.rom
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating rom.h from prog.rom"
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
