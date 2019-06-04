cmake_minimum_required(VERSION 3.0)
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
        src/symbol.cpp src/symbol.h
        thirdparty/linenoise/linenoise.c)

add_executable(hello example/hello.cpp)
set_target_properties(hello
        PROPERTIES COMPILE_FLAGS "-g -O0")

add_executable(variables example/variables.cpp)
set_target_properties(variables
        PROPERTIES COMPILE_FLAGS "-g -O0")

add_executable(unwinding example/stack_unwinding.cpp)
set_target_properties(unwinding
                      PROPERTIES COMPILE_FLAGS "-g -O0")

add_custom_target(
   libelfin
   COMMAND make
   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/thirdparty/libelfin
)
target_link_libraries(tinydbg
                      ${PROJECT_SOURCE_DIR}/thirdparty/libelfin/dwarf/libdwarf++.so
                      ${PROJECT_SOURCE_DIR}/thirdparty/libelfin/elf/libelf++.so)
add_dependencies(tinydbg libelfin)