cmake_minimum_required(VERSION 3.16)
project(portugolc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB PORTUGOLC_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(portugolc ${PORTUGOLC_SRCS})
