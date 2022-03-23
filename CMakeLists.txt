cmake_minimum_required(VERSION 3.0)

project(modularize)

add_executable(modularize main.cpp)

install(TARGETS modularize RUNTIME DESTINATION bin)
