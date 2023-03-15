cmake_minimum_required(VERSION 3.16)

project(netsim)

set(CMAKE_BUILD_TYPE Debug)

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

# include_directories(include)
add_executable(netsim src/simphy.c cjson/cJSON.c)