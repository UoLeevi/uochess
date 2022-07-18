cmake_minimum_required(VERSION 3.21.0)
project(uochess VERSION 0.1.0)

enable_testing()

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_subdirectory(src)
