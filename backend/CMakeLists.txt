cmake_minimum_required(VERSION 2.8)

add_subdirectory(3rdparty)
add_subdirectory(src)

install(DIRECTORY resources/
        DESTINATION etc/credentarius
        COMPONENT resources)
