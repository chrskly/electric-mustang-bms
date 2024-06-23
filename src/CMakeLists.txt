cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include(pico_sdk_import.cmake)

project(bms)

set(PICO_CXX_ENABLE_EXCEPTIONS 1)

pico_sdk_init()

add_executable(bms
        settings.h
        util.cpp
        mcp2515/mcp2515.cpp
        io.cpp        
        module.cpp
        pack.cpp
        battery.cpp
        bms.cpp
        statemachine.cpp
        led.cpp
        CRC8.cpp
        shunt.cpp
        main.cpp
        )

pico_add_extra_outputs(bms)

target_link_libraries(bms pico_stdlib hardware_spi pico_multicore)

target_include_directories(bms PRIVATE include . )
