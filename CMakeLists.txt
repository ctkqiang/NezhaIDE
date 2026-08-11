cmake_minimum_required(VERSION 4.3)
project(NezhaIDE)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)
find_package(SQLite3 REQUIRED)
find_package(LIEF REQUIRED)
find_library(CAPSTONE_LIBRARY capstone REQUIRED)
find_path(CAPSTONE_INCLUDE_DIR capstone/capstone.h REQUIRED)

add_executable(NezhaIDE main.cc
        src/utilities/logger.cc
        src/utilities/logger.h
        src/services/database_helper.cc
        src/services/database_helper.h
        src/services/localization_service.cc
        src/services/localization_service.h
        src/services/theme_service.cc
        src/services/theme_service.h
        views/editor/code_editor.cc
        views/editor/code_editor.h
        views/editor/cpp_highlighter.cc
        views/editor/cpp_highlighter.h
        views/editor/simple_highlighter.cc
        views/editor/simple_highlighter.h
        views/editor/language_registry.cc
        views/editor/language_registry.h
        views/editor/editor_tab_host.cc
        views/editor/editor_tab_host.h
        src/configuration.h
        src/configuration.cc
        views/main_window.cc
        views/main_window.h
        views/main_window.ui
        views/sidebar_container.cc
        views/sidebar_container.h
        views/sidebar_container.ui
        views/explorer_panel.cc
        views/explorer_panel.h
        views/explorer_panel.ui
        views/preferences_dialog.cc
        views/preferences_dialog.h
        views/activity_bar.cc
        views/activity_bar.h
        views/git_panel/git_panel.cc
        views/git_panel/git_panel.h
        views/git_panel/git_panel.ui
        src/model/column_def.h
        src/model/user_preference.h
        src/model/http_request.h
        src/repository/orm.h
        src/model/tool_registry.h
        src/services/http.cc
        src/services/http.h
        views/http_view_panel/http_view_panel.cc
        views/http_view_panel/http_view_panel.h
        src/model/tools.h
        views/hex_editor/binary_parser.cc
        views/hex_editor/binary_parser.h
        views/hex_editor/disassembler.cc
        views/hex_editor/disassembler.h
        views/hex_editor/hex_view.cc
        views/hex_editor/hex_view.h
        views/hex_editor/hex_editor.cc
        views/hex_editor/hex_editor.h)

target_include_directories(NezhaIDE PRIVATE ${CMAKE_SOURCE_DIR} ${CAPSTONE_INCLUDE_DIR})

target_compile_definitions(NezhaIDE PRIVATE NEZHA_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

target_link_libraries(NezhaIDE PRIVATE Qt6::Core Qt6::Widgets Qt6::Network SQLite3::SQLite3 LIEF::LIEF ${CAPSTONE_LIBRARY})
