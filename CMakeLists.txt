cmake_minimum_required(VERSION 3.13)
project(fileDuplicator C)

set(CMAKE_C_STANDARD 11)

add_executable(fileDuplicator main.c)
target_link_libraries(fileDuplicator pthread rt)