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
        src/services/localization_service.cc
        src/services/localization_service.h
        src/services/theme_service.cc
        src/services/theme_service.h
        src/editor/code_editor.cc
        src/editor/code_editor.h
        src/editor/cpp_highlighter.cc
        src/editor/cpp_highlighter.h
        src/editor/simple_highlighter.cc
        src/editor/simple_highlighter.h
        src/editor/language_registry.cc
        src/editor/language_registry.h
        src/editor/editor_tab_host.cc
        src/editor/editor_tab_host.h
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
        src/views/preferences_dialog.cc
        src/views/preferences_dialog.h
        src/views/activity_bar.cc
        src/views/activity_bar.h
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
