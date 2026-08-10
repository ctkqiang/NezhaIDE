cmake_minimum_required(VERSION 4.3)
project(NezhaIDE)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
find_package(SQLite3 REQUIRED)

add_executable(NezhaIDE main.cc
        src/utilities/logger.cc
        src/utilities/logger.h
        src/services/database_helper.cc
        src/services/database_helper.h
        src/configuration.h
        src/configuration.cc
        src/views/main_window.cc
        src/views/main_window.h
        src/views/main_window.ui
        src/model/user_preference.h
        src/repository/user_preference_repository.h
        src/repository/user_preference_repository.cc)

target_include_directories(NezhaIDE PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(NezhaIDE PRIVATE Qt6::Core Qt6::Widgets SQLite::SQLite3)
