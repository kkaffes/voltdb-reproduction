cmake_minimum_required(VERSION 3.5)
message(STATUS "Using CMake version ${CMAKE_VERSION}")
project(voltdb-reproduction)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_COLOR_MAKEFILE   ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Targets
add_subdirectory(src)

