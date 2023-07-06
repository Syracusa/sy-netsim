add_executable(simulator  
                simulator.c
                simulator_config.c
                sim_server.c
                sim_remote_conf.c
                sim_childproc.c
                sim_hdlreport.c
                ../util/netutil.c
                ../util/mq.c
                ../util/ringbuffer.c)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(simulator Threads::Threads m cjson)