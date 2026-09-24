cmake_minimum_required(VERSION 3.16)

project(ChessEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(ChessEngine
    src/main.cpp
    src/movegen.cpp
    src/eval.cpp
    src/search.cpp
    src/zobrist.cpp
    src/uci.cpp
)

target_link_libraries(ChessEngine)
