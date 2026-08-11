cmake_minimum_required(VERSION 3.25)
project(http_probe)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

add_executable(http_probe http_probe.cc ../src/services/http.cc ../src/services/http.h ../src/model/http_request.h)
target_include_directories(http_probe PRIVATE ${CMAKE_SOURCE_DIR}/..)
target_link_libraries(http_probe PRIVATE Qt6::Core Qt6::Network)
