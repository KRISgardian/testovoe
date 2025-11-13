cmake_minimum_required(VERSION 4.0)
project(max_internals LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



add_executable(testovoe src/main.cpp)

install(TARGETS testovoe DESTINATION ${CMAKE_SOURCE_DIR}/bin/)