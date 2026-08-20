cmake_minimum_required(VERSION 3.16)

project(ChessEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(ChessEngine
    src/main.cpp
)

target_link_libraries(ChessEngine PRIVATE Qt6::Widgets)
