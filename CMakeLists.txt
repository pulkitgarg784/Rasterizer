cmake_minimum_required(VERSION 3.16)
project(Rasterizer)

set(CMAKE_CXX_STANDARD 20)

# Include headers
include_directories(include)

# Source files
set(SOURCES
    src/main.cpp
    src/tgaimage.cpp
    src/model.cpp
)

add_executable(Rasterizer ${SOURCES})