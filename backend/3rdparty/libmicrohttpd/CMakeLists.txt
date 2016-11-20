cmake_minimum_required(VERSION 2.8)

project(microhttpd C)

include(CheckCSourceCompiles)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckTypeSize)

check_include_file(stddef.h    HAVE_STDDEF_H)
check_include_file(stdint.h    HAVE_STDINT_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(unistd.h    HAVE_UNISTD_H)

add_definitions(-DHAVE_CONFIG_H)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/code/src/include
)

set(microhttpd_SRCS
    code/src/microhttpd/base64.c
    code/src/microhttpd/basicauth.c
    code/src/microhttpd/connection.c
    code/src/microhttpd/daemon.c
    code/src/microhttpd/digestauth.c
    code/src/microhttpd/internal.c
    code/src/microhttpd/md5.c
    code/src/microhttpd/memorypool.c
    code/src/microhttpd/mhd_compat.c
    code/src/microhttpd/mhd_itc.c
    code/src/microhttpd/mhd_mono_clock.c
    code/src/microhttpd/mhd_sockets.c
    code/src/microhttpd/mhd_str.c
    code/src/microhttpd/mhd_threads.c
    code/src/microhttpd/postprocessor.c
    code/src/microhttpd/reason_phrase.c
    code/src/microhttpd/response.c
    code/src/microhttpd/sysfdsetsize.c
)

add_library(microhttpd ${microhttpd_SRCS})

target_link_libraries(microhttpd c pthread)

