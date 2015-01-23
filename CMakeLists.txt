cmake_minimum_required(VERSION 2.8.4)
project(bannertool)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES source/main.cpp source/lz11.cpp source/lodepng.cpp)
add_executable(bannertool ${SOURCE_FILES})