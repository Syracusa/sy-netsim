add_executable(simmac
                "simmac.c" "../util/util.c")

target_link_libraries(simmac PUBLIC cjson)