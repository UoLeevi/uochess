cmake_minimum_required(VERSION 3.12)

if(NOT TARGET uo::uo_err)
    find_package(uo_err CONFIG REQUIRED)
endif()
if(NOT TARGET uo::uo_cb)
    find_package(uo_cb CONFIG REQUIRED)
endif()

add_executable(uochess
    uochess.c
    uo_bitboard.c
    uo_position.c)

target_include_directories(uochess
    PRIVATE
        include)

target_link_libraries(uochess
    PRIVATE
        uo::uo_err
        uo::uo_cb)

target_compile_features(uochess
    PUBLIC
        c_std_11)

install(DIRECTORY conf/
    DESTINATION bin)

install(TARGETS uochess
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include)
