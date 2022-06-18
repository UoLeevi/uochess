cmake_minimum_required(VERSION 3.12)
project(uochess VERSION 0.1.0)

enable_testing()

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set(CMAKE_PREFIX_PATH "/usr/local/libuo" "/usr/local")

add_subdirectory(src)
