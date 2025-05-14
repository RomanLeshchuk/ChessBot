cmake_minimum_required(VERSION 3.30)
project(ChessBot)

set(CMAKE_CXX_STANDARD 20)


add_executable(ChessBot
        src/main.cpp
        src/Board.cpp
        src/Move.cpp
        src/GameState.cpp
)

target_include_directories(ChessBot PRIVATE include)
