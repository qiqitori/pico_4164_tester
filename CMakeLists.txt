cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico_examples C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

# Initialize the SDK
pico_sdk_init()

include(example_auto_set_url.cmake)

add_compile_options(-Wall -Wextra
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        -O0
        )

add_executable(4164_test
        4164_test.c
        )

# pull in common dependencies
target_link_libraries(4164_test pico_stdlib)

# enable usb output, disable uart output
pico_enable_stdio_usb(4164_test 1)
pico_enable_stdio_uart(4164_test 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(4164_test)

# add url via pico_set_program_url
example_auto_set_url(4164_test)
