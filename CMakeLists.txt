cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(solidsnake-ece362project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(solidsnake-ece362project
    src/main.c
    src/matrix.c
    src/keypad.c
)

# Generate hub75_shift.pio.h from src/hub75_shift.pio
pico_generate_pio_header(solidsnake-ece362project
    ${CMAKE_CURRENT_LIST_DIR}/src/hub75_shift.pio
)

target_link_libraries(solidsnake-ece362project
    pico_stdlib
    hardware_pio
    hardware_timer
)

pico_add_extra_outputs(solidsnake-ece362project)
