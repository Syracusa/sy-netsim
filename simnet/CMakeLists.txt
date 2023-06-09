add_subdirectory(olsr)

add_executable(simnet  
                "simnet.c"
                "dummy.c"
                "mac_connection.c"
                "../util/util.c"
                "../util/mq.c"
                "../util/rbtree.c"
                "../util/timerqueue.c"
                "../util/netutil.c"
                )

target_link_libraries(simnet PUBLIC cjson)
target_link_libraries(simnet PUBLIC sy-olsr)