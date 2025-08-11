cmake_minimum_required(VERSION 3.10)
project(json.c LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(json_parser STATIC json.c)
