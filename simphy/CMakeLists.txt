add_executable(simphy  
                "main.c"
                "simphy.c"
                "../util/util.c"
                "../util/mq.c"
                )

target_link_libraries(simnet PUBLIC cjson)
target_link_libraries(simnet PUBLIC timerqueue)