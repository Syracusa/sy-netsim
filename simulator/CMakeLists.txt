add_executable(simulator  
                "simulator.c"
                "simulator_config.c"
                "../util/mq.c")

target_link_libraries(simulator PUBLIC cjson)