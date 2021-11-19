cmake_minimum_required(VERSION 2.8)

set(CMAKE_C_COMPILER "/home/yuayu/Data/gcc11.2.0/bin/gcc")
set(CMAKE_CXX_COMPILER "/home/yuayu/Data/gcc11.2.0/bin/g++")

project(QueryGenerator)

include_directories(
        ${PROJECT_SOURCE_DIR}/
)

set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -g -Wall -march=native -pthread")

add_executable(QueryGenerator main.cc query_command.cpp)

add_subdirectory(utility)

target_link_libraries(QueryGenerator PUBLIC utility)