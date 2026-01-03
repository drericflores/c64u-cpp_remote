cmake_minimum_required(VERSION 3.16)
project(u64_remote_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)

add_executable(u64-remote
    src/main.cpp
    src/u64_server.cpp
    src/util.cpp
)

target_include_directories(u64-remote PRIVATE src)
target_link_libraries(u64-remote PRIVATE CURL::libcurl)
