add_subdirectory(olsr)
add_subdirectory(aodv)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(simnet  
                "simnet.c"
                "dummy.c"
                "mac_connection.c"
                "../util/util.c"
                "../util/mq.c"
                "../util/netutil.c"
                )

target_link_libraries(simnet PUBLIC cjson)
target_link_libraries(simnet PUBLIC timerqueue)
target_link_libraries(simnet PUBLIC sy-olsr)
target_link_libraries(simnet PUBLIC sy-aodv)