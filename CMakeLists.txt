cmake_minimum_required(VERSION 4.3)
project(NezhaIDE)

set(CMAKE_CXX_STANDARD 26)

add_executable(NezhaIDE main.cc
        src/utilities/logger.cc
        src/utilities/logger.h)
