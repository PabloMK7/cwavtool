cmake_minimum_required(VERSION 2.8.4)
project(bannertool)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES source/main.cpp source/cmd.cpp source/pc/wav.cpp source/3ds/cbmd.cpp source/3ds/cwav.cpp source/3ds/lz11.cpp source/3ds/util.cpp source/pc/lodepng.cpp)
add_executable(bannertool ${SOURCE_FILES})