cmake_minimum_required(VERSION 4.3)
project(NezhaIDE)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Svg)
find_package(SQLite3 REQUIRED)
find_package(tl-expected REQUIRED)
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
        src/model/credential_dataset.h
        src/repository/orm.h
        src/model/tool_registry.h
        src/services/http.cc
        src/services/http.h
        views/http_view_panel/http_view_panel.cc
        views/http_view_panel/http_view_panel.h
        views/hydra/hydra_view_panel.cc
        views/hydra/hydra_view_panel.h
        src/model/tools.h
        views/hex_editor/binary_parser.cc
        views/hex_editor/binary_parser.h
        views/hex_editor/disassembler.cc
        views/hex_editor/disassembler.h
        views/hex_editor/disasm_view.cc
        views/hex_editor/disasm_view.h
        views/hex_editor/hex_view.cc
        views/hex_editor/hex_view.h
        views/hex_editor/hex_editor.cc
        views/hex_editor/hex_editor.h
        views/terminal/terminal_view.cc
        views/terminal/terminal_view.h
        views/terminal/terminal_panel.cc
        views/terminal/terminal_panel.h
        src/utilities/permission.cc
        src/utilities/permission.h
        views/vectors/vectors.qrc
        src/tools/hydra.cc
        src/tools/hydra.h
        src/utilities/downloader.cc
        src/utilities/downloader.h
        src/model/password.h)

set_target_properties(NezhaIDE PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.nezha-security.nezhaide"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
)

# 未签名 bundle 会让 macOS 无法归属进程，导致 Gatekeeper 误报与
# launchd 拒绝启动；ad-hoc 签名即可避免。注意：BaseBoard 的
# "Unable to obtain a task name port right" 日志是 macOS 26 对所有
# 非真签名进程的系统噪音（WindowServer 为受保护进程，按设计拒绝
# task port），签名无法消除，与本应用代码无关。
set(NEZHAIDE_CODESIGN_IDENTITY "-" CACHE STRING
    "代码签名身份；默认 '-' 表示 ad-hoc 签名，可用开发者证书替换")
add_custom_command(TARGET NezhaIDE POST_BUILD
    COMMAND codesign --force --sign "${NEZHAIDE_CODESIGN_IDENTITY}"
            "$<TARGET_BUNDLE_DIR:NezhaIDE>"
    COMMENT "Code signing NezhaIDE.app (${NEZHAIDE_CODESIGN_IDENTITY})"
    VERBATIM)

if(APPLE)
    # permission.cc 需要 Objective-C++ 运行时调用 TCC/AVFoundation API；
    # C++26 的枚举算术限制使 SDK 的 CoreGraphics/QD 头无法编译，此文件降级为 C++23
    set_source_files_properties(src/utilities/permission.cc PROPERTIES
        COMPILE_OPTIONS "-x;objective-c++;-std=gnu++23")
    target_link_libraries(NezhaIDE PRIVATE
        "-framework CoreGraphics"
        "-framework ApplicationServices"
        "-framework AVFoundation"
        "-framework UserNotifications")
endif()

target_include_directories(NezhaIDE PRIVATE ${CMAKE_SOURCE_DIR} ${CAPSTONE_INCLUDE_DIR})

target_compile_definitions(NezhaIDE PRIVATE NEZHA_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

target_link_libraries(NezhaIDE PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Svg SQLite3::SQLite3 LIEF::LIEF ${CAPSTONE_LIBRARY})
