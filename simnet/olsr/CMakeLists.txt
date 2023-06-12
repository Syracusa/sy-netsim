add_library (sy-olsr 
             olsr_route_iface.c
             olsr_context.c
             olsr_hello.c
             ../../util/netutil.c
             ../../util/ringbuffer.c)

target_link_libraries(sy-olsr PUBLIC timerqueue)