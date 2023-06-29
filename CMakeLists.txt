cmake_minimum_required(VERSION 3.16)

project(netsim
    VERSION 0.1
    DESCRIPTION "A network simulator"
    LANGUAGES C)

set(CMAKE_BUILD_TYPE Debug)

add_compile_options(-fsanitize=address -Wall -Wno-address-of-packed-member -Wno-unused-function)
add_link_options(-fsanitize=address)

include_directories(ext/cjson ext/timerqueue include util)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(ext)
add_subdirectory(simulator)
add_subdirectory(simnet)
add_subdirectory(simmac)
add_subdirectory(simphy)