add_library (sy-olsr 
             olsr_route_iface.c
             ../../util/netutil.c)

target_link_libraries(sy-olsr PUBLIC timerqueue)