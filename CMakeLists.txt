cmake_minimum_required(VERSION 4.1)
project(Rasterizer)

set(CMAKE_CXX_STANDARD 20)

add_executable(Rasterizer main.cpp
        tgaimage.cpp
        tgaimage.h
        tgaimage.h)
