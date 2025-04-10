cmake_minimum_required(VERSION 3.30)
project(ChessBot)

set(CMAKE_CXX_STANDARD 20)

add_executable(ChessBot main.cpp
        GameState.h
        Data.h)
