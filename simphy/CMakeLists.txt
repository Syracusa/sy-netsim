add_executable(simphy  
                "simphy.c"
                "../util/util.c"
                "../util/mq.c"
                )

target_link_libraries(simnet PUBLIC cjson)
target_link_libraries(simnet PUBLIC timerqueue)