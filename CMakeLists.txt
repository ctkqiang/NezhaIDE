cmake_minimum_required(VERSION 4.3)
project(NezhaIDE)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_executable(NezhaIDE main.cc
        src/utilities/logger.cc
        src/utilities/logger.h
        src/services/database_helper.cc
        src/services/database_helper.h
        src/configuration.h
        src/configuration.cc
        src/views/main_window.cc
        src/views/main_window.h
        src/views/main_window.ui)

target_link_libraries(NezhaIDE PRIVATE Qt6::Core Qt6::Widgets)
