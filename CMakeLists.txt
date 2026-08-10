cmake_minimum_required(VERSION 4.3)
project(NezhaIDE)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets LinguistTools)
find_package(SQLite3 REQUIRED)

add_executable(NezhaIDE main.cc
        src/utilities/logger.cc
        src/utilities/logger.h
        src/services/database_helper.cc
        src/services/database_helper.h
        src/services/language_manager.cc
        src/services/language_manager.h
        src/configuration.h
        src/configuration.cc
        src/views/main_window.cc
        src/views/main_window.h
        src/views/main_window.ui
        src/views/sidebar_container.cc
        src/views/sidebar_container.h
        src/views/sidebar_container.ui
        src/views/explorer_panel.cc
        src/views/explorer_panel.h
        src/views/explorer_panel.ui
        src/views/git_panel.cc
        src/views/git_panel.h
        src/views/git_panel.ui
        src/model/column_def.h
        src/model/user_preference.h
        src/repository/orm.h
        src/model/tool_registry.h)

target_include_directories(NezhaIDE PRIVATE ${CMAKE_SOURCE_DIR})

target_compile_definitions(NezhaIDE PRIVATE NEZHA_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

target_link_libraries(NezhaIDE PRIVATE Qt6::Core Qt6::Widgets SQLite3::SQLite3)

set(TS_FILES
    resources/localisation/nezha_ide_en_US.ts
)

qt_add_lupdate(NezhaIDE TS_FILES ${TS_FILES})
qt_add_lrelease(NezhaIDE TS_FILES ${TS_FILES} QM_FILES_OUTPUT_VARIABLE QM_FILES)
