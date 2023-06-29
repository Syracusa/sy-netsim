
find_package(Libwebsockets REQUIRED CONFIG)

add_executable(simulator  
                "simulator.c"
                "simulator_config.c"
                "sim_server.c"
                "../util/mq.c"
                "../util/ringbuffer.c")
find_package(PkgConfig REQUIRED) 

target_link_libraries(simulator m) # Math library
target_link_libraries(simulator cjson)