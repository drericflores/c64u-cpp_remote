#!/usr/bin/env bash
set -e

BASE_DIR="/home/eric/development/c++/u64-remote/u64-remote-cpp"
SRC_DIR="$BASE_DIR/src"

echo "Creating project at: $BASE_DIR"

mkdir -p "$SRC_DIR"

########################
# CMakeLists.txt
########################
cat > "$BASE_DIR/CMakeLists.txt" <<'EOF'
cmake_minimum_required(VERSION 3.16)
project(u64_remote_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(u64-remote
  src/main.cpp
  src/u64_server.cpp
  src/util.cpp
)

target_include_directories(u64-remote PRIVATE src)

target_link_libraries(u64-remote
  PRIVATE
    CURL::libcurl
    nlohmann_json::nlohmann_json
)

if (WIN32)
  target_compile_definitions(u64-remote PRIVATE U64REMOTE_WINDOWS=1)
endif()
EOF

########################
# README.md
########################
cat > "$BASE_DIR/README.md" <<'EOF'
# u64-remote (C++)

Remote command-line client for a U64 / C64 Ultimate REST API.

## Features
- Upload and run PRG files
- Read (peek) memory
- Write (poke) memory
- Auth via X-Password header

## Build (Linux / Pop!_OS)
```bash
sudo apt install -y build-essential cmake libcurl4-openssl-dev nlohmann-json3-dev
mkdir -p build
cmake -S . -B build
cmake --build build -j
