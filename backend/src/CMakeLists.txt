cmake_minimum_required(VERSION 2.8)

project(credentarius C)

include(CheckCSourceCompiles)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckTypeSize)

check_include_file(stddef.h    HAVE_STDDEF_H)
check_include_file(stdint.h    HAVE_STDINT_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(unistd.h    HAVE_UNISTD_H)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/ulfius/code/lib/orcania
    ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/ulfius/code/lib/yder/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/ulfius/code/src
)

set(credentarius_SRCS
    "compile.c"
    "main.c"
    "mcu.c"
    "project.c"
)

add_executable(credentarius ${credentarius_SRCS})

target_link_libraries(credentarius microhttpd ulfius)

