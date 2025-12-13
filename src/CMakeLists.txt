cmake_minimum_required(VERSION 3.13)

add_executable(boot3
    boot3.c
    )

# pull in common dependencies
target_link_libraries(boot3 pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(boot3)
