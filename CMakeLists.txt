cmake_minimum_required(VERSION 3.12)

project(PiBleInk)

add_executable(pibleink main.c)

target_link_libraries(pibleink bluetooth)