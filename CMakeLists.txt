cmake_minimum_required(VERSION 3.16)

project(netsim)

set(CMAKE_BUILD_TYPE Debug)

add_compile_options(-fsanitize=address -Wno-address-of-packed-member)
add_link_options(-fsanitize=address)

include_directories(cjson include util)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(cjson)
add_subdirectory(simulator)
add_subdirectory(simnet)
add_subdirectory(simmac)
add_subdirectory(simphy)