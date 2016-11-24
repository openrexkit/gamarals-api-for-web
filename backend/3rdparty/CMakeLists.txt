cmake_minimum_required(VERSION 2.8)

add_subdirectory(libmicrohttpd)

option(JANSSON_BUILD_DOCS "Build documentation (uses python-sphinx)." OFF)
option(JANSSON_EXAMPLES "Compile example applications" OFF)
option(JANSSON_INSTALL "Generate installation target" OFF)
option(JANSSON_WITHOUT_TESTS "Don't build tests ('make test' to execute tests)" ON)
add_subdirectory(jansson/code)

add_subdirectory(ulfius)
