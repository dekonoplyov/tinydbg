cmake_minimum_required(VERSION 3.14)
project(tinydbg)

set(CMAKE_CXX_STANDARD 14)

include_directories(
        thirdparty/libelfin
        thirdparty/linenoise)
add_executable(tinydbg
        src/main.cpp
        thirdparty/linenoise/linenoise.c)