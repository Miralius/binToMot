cmake_minimum_required(VERSION 3.27)

project(BinToMot LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)

add_executable(BinToMot main.cpp binToMot.h)