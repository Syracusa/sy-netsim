add_library (sy-olsr 
             olsr_route_iface.c
             olsr_context.c
             olsr_hello.c
             olsr_mantissa.c
             olsr_route.c
             olsr_mantissa.c
             olsr_link.c
             olsr_neighbor.c
             olsr_mpr.c
             olsr_tc.c
             ../../util/cll.c
             ../../util/util.c
             ../../util/netutil.c
             ../../util/ringbuffer.c)

target_link_libraries(sy-olsr PUBLIC timerqueue)