cmake_minimum_required(VERSION 2.8.0)
project(hypergrace)

# TODO #1: Figure out how to force CTest to not strip output from
# test suite
# TODO #2: Running CTest in verbose mode forces it to display test
# suite's output, however, all color information is lost.
#enable_testing(true)

# Ninja linking -lrt
set(CMAKE_CXX_FLAGS "-std=c++0x -pthread -lrt -fstrict-aliasing -Wall -Wstrict-aliasing=2 -Wno-unused-local-typedefs")
add_definitions(-D_FILE_OFFSET_BITS=64)

add_subdirectory(libhypergrace)
add_subdirectory(gui-qt4)
add_subdirectory(tests)
