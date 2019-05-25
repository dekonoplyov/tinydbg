cmake_minimum_required(VERSION 3.14)
project(tinydbg)

set(CMAKE_CXX_STANDARD 14)

include_directories(
        thirdparty/libelfin
        thirdparty/linenoise)
add_executable(tinydbg
        src/main.cpp
        src/debugger.cpp src/debugger.h
        thirdparty/linenoise/linenoise.c)

add_executable(add example/add.cpp)
set_target_properties(add
        PROPERTIES COMPILE_FLAGS "-g -O0")