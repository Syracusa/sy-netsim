add_executable(simulator  
                "simulator.c"
                "simulator_config.c")

target_link_libraries(simulator PUBLIC cjson)