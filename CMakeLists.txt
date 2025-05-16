cmake_minimum_required(VERSION 3.16)

project(ThreadPool)

set(CMAKE_CXX_STANDARD 20)

add_executable(ThreadPool
    src/main.cpp
)

target_include_directories(ThreadPool PRIVATE src)
target_compile_options(ThreadPool PRIVATE -Wall -Wextra -Wpedantic)