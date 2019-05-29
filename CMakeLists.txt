cmake_minimum_required(VERSION 3.10)
project(tinydbg)

set(CMAKE_CXX_STANDARD 17)

include_directories(
        thirdparty/libelfin
        thirdparty/linenoise)
add_executable(tinydbg
        src/main.cpp
        src/breakpoint.cpp src/breakpoint.h
        src/debugger.cpp src/debugger.h
        src/registers.cpp src/registers.h
        thirdparty/linenoise/linenoise.c)

add_executable(hello example/hello.cpp)
set_target_properties(hello
        PROPERTIES COMPILE_FLAGS "-g -O0")
