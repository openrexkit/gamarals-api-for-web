cmake_minimum_required(VERSION 2.8)

project(ulfius C)

include(CheckCSourceCompiles)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckTypeSize)

check_include_file(stddef.h    HAVE_STDDEF_H)
check_include_file(stdint.h    HAVE_STDINT_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(unistd.h    HAVE_UNISTD_H)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../jansson/code/src
    ${CMAKE_CURRENT_SOURCE_DIR}/code/lib/orcania
    ${CMAKE_CURRENT_SOURCE_DIR}/code/lib/yder/src
    ${CMAKE_CURRENT_SOURCE_DIR}/code/src
)

set(ulfius_SRCS
    code/lib/orcania/orcania.c
    code/lib/yder/src/yder.c
    code/src/u_map.c
    code/src/u_request.c
    code/src/u_response.c
    code/src/ulfius.c
)

add_library(ulfius ${ulfius_SRCS})

target_link_libraries(ulfius c microhttpd jansson)

