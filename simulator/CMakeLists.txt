
find_package(Libwebsockets REQUIRED CONFIG)

add_executable(simulator  
                "simulator.c"
                "simulator_config.c"
                "httpserver.c"
                "../util/mq.c")
find_package(PkgConfig REQUIRED) 

target_link_libraries(simulator m) # Math library
target_link_libraries(simulator "/usr/local/lib/libwebsockets.so")
target_link_libraries(simulator cjson)