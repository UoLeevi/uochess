cmake_minimum_required(VERSION 3.12)

add_executable(uochess
    uochess.c
    uo_bitboard.c
    uo_position.c
    uo_piece.c
    uo_square.c
    uo_move.c
    uo_search.c
    uo_util.c
    uo_test.c)

target_include_directories(uochess
    PRIVATE
        include)

target_link_libraries(uochess)

target_compile_features(uochess
    PUBLIC
        c_std_11)

add_test(NAME uochess_test
    COMMAND uochess --test --datadir ${PROJECT_SOURCE_DIR}/test_data)

install(DIRECTORY conf/
    DESTINATION bin)

install(TARGETS uochess
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include)
