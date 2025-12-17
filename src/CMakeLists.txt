# cmake version
cmake_minimum_required(VERSION 3.13)

# include the sdk.cmake file
include("$ENV{PICO_SDK_PATH}/pico_sdk_init.cmake")

# give the project a name (anything you want)
project(DAWGkit C CXX ASM)

# initialize the sdk
pico_sdk_init()
set(PICO_PLATFORM rp2040)
set(PICO_FLASH_OFFSET 0x00F00000)

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        -Os)

add_executable(DAWGkit 
	boot3.c
	boot3_start.S
	bootrom/program_flash_generic.c 
	bootrom/bootrom_misc.S 
	bootrom/mufplib.S 
	bootrom/mufplib-double.S
	bootrom/bit_functions.S
	bootrom/runtime.c
	bootrom/resets.S
)

pico_set_linker_script(DAWGkit ${CMAKE_CURRENT_SOURCE_DIR}/boot3.ld)

target_link_libraries(DAWGkit pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(DAWGkit)