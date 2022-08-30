cmake_minimum_required(VERSION 3.21.0)

find_package(Threads)

add_executable(uochess
    uochess.c
    uo_bitboard.c
    uo_position.c
    uo_square.c
    uo_search.c
    uo_zobrist.c
    uo_evaluation.c
    uo_thread.c
    uo_misc.c
    uo_uci.c
    uo_engine.c
    uo_nn.c
    uo_math.c
    uo_test.c)

target_include_directories(uochess
    PRIVATE
        include)

target_link_libraries(uochess
    PRIVATE
        ${CMAKE_THREAD_LIBS_INIT})

target_compile_features(uochess
    PUBLIC
        c_std_11)

if((CMAKE_CXX_COMPILER_ID MATCHES "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    target_compile_options(uochess
        PRIVATE -mavx -mavx2 -mbmi2 -mpopcnt)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    target_compile_options(uochess
        PRIVATE /QxKABYLAKE)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(uochess
        PRIVATE /arch:AVX2 /volatile:iso)
endif()

add_test(NAME "Test all"
    COMMAND uochess --test --datadir ${PROJECT_SOURCE_DIR}/test_data)

add_test(NAME "Test move_generation"
    COMMAND uochess --test --datadir ${PROJECT_SOURCE_DIR}/test_data --testname move_generation)

add_test(NAME "Test matmul"
    COMMAND uochess --test --datadir ${PROJECT_SOURCE_DIR}/test_data --testname matmul)

add_test(NAME "Test nn_train"
    COMMAND uochess --test --datadir ${PROJECT_SOURCE_DIR}/test_data --testname nn_train)

install(DIRECTORY conf/
    DESTINATION bin)

install(TARGETS uochess
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include)
