cmake_minimum_required(VERSION 3.16)

project(netsim)

set(CMAKE_BUILD_TYPE Debug)

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

include_directories("src" "src/util" "cjson")
add_executable(netsim 
                "cjson/cJSON.c" 
                "src/simphy.c"
                "src/util/ringbuffer.c"
                "src/config.c"
                )