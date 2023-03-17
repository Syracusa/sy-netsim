add_executable(simnet  
                "simnet.c"
                "../util/util.c"
                "../util/mq.c")

target_link_libraries(simnet PUBLIC cjson)