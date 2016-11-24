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

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CREDENTARIUS_API_PREFIX "/api" CACHE STRING "Credentarius URI Prefix")
set(CREDENTARIUS_PORT "8537" CACHE STRING "Credentarius Port")
set(CREDENTARIUS_PROJECT_ROOT "/tmp/projects" CACHE PATH "Credentarius Project Root")
configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

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

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/credentarius
        DESTINATION bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ GROUP_EXECUTE
                    WORLD_READ WORLD_EXECUTE
        COMPONENT application)
