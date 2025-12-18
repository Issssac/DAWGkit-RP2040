# cmake version
cmake_minimum_required(VERSION 3.13)

include("$ENV{PICO_SDK_PATH}/pico_sdk_init.cmake")
project(DAWGkit C CXX ASM)
pico_sdk_init()

set(PICO_PLATFORM rp2040)

add_compile_options(
	-Wall
        -Wno-format
        -Wno-unused-function
        #-Wno-maybe-uninitialized
        -O3)

add_executable(DAWGkit 
	boot3.c
	bootrom/bootrom_misc.S
)

pico_set_linker_script(DAWGkit ${CMAKE_CURRENT_SOURCE_DIR}/boot3.ld)

target_link_libraries(DAWGkit pico_stdlib)
pico_enable_stdio_usb(DAWGkit 1)
pico_enable_stdio_uart(DAWGkit 0)
# create map/bin/hex file etc.
pico_add_extra_outputs(DAWGkit)