cmake_minimum_required(VERSION 3.16)
project(u64_remote LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(u64-remote
    src/main.cpp
    src/discovery.cpp
    src/u64_server.cpp
    src/util.cpp
)

target_include_directories(u64-remote PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

find_package(Threads REQUIRED)
target_link_libraries(u64-remote PRIVATE Threads::Threads)

find_package(PkgConfig REQUIRED)

# ---- Avahi (pkg-config works for avahi-client; avahi-common.pc is missing on your system) ----
pkg_check_modules(AVAHI_CLIENT REQUIRED avahi-client)

message(STATUS "AVAHI_CLIENT_INCLUDE_DIRS = ${AVAHI_CLIENT_INCLUDE_DIRS}")
message(STATUS "AVAHI_CLIENT_LIBRARIES    = ${AVAHI_CLIENT_LIBRARIES}")

target_include_directories(u64-remote PRIVATE
    ${AVAHI_CLIENT_INCLUDE_DIRS}
    /usr/include/avahi-client
    /usr/include/avahi-common
)

target_compile_options(u64-remote PRIVATE
    ${AVAHI_CLIENT_CFLAGS_OTHER}
)

# ---- libcurl via pkg-config (reliable on Ubuntu/Pop!_OS) ----
pkg_check_modules(LIBCURL REQUIRED libcurl)

message(STATUS "LIBCURL_INCLUDE_DIRS = ${LIBCURL_INCLUDE_DIRS}")
message(STATUS "LIBCURL_LIBRARIES    = ${LIBCURL_LIBRARIES}")

target_include_directories(u64-remote PRIVATE
    ${LIBCURL_INCLUDE_DIRS}
)

target_compile_options(u64-remote PRIVATE
    ${LIBCURL_CFLAGS_OTHER}
)

target_link_libraries(u64-remote PRIVATE
    ${AVAHI_CLIENT_LIBRARIES}
    avahi-common
    ${LIBCURL_LIBRARIES}
)
