add_executable(simmac
                "simmac.c" 
                "../util/util.c"
                "../util/mq.c"
                )

target_link_libraries(simmac PUBLIC cjson)