cmake_minimum_required(VERSION 3.21.0)

find_package(Threads)

find_library(TENSORFLOW_LIBRARY
    NAMES tensorflow
    HINTS "${TensorFlow_DIR}/lib")

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
    uo_nn_funcs.c
    uo_math.c
    uo_test.c)

target_include_directories(uochess
    PRIVATE
        "include"
        "${TensorFlow_DIR}/include")

target_link_libraries(uochess
    PRIVATE
        ${CMAKE_THREAD_LIBS_INIT}
        ${TENSORFLOW_LIBRARY})

target_compile_features(uochess
    PUBLIC
        c_std_11)

set_target_properties(uochess PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/src")

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

add_custom_target(copy-runtime-files ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/src/nn" "${CMAKE_BINARY_DIR}/bin/nn"
    DEPENDS uochess)

add_test(NAME "Test all"
    COMMAND uochess --test --datadir ${PROJECT_SOURCE_DIR}/test_data)

add_test(NAME "Test move_generation"
    COMMAND uochess --test --datadir ${PROJECT_SOURCE_DIR}/test_data --testname move_generation)

add_test(NAME "Test matmul"
    COMMAND uochess --test --datadir ${PROJECT_SOURCE_DIR}/test_data --testname matmul)

add_test(NAME "Test nn_train_xor"
    COMMAND uochess --test --datadir ${PROJECT_SOURCE_DIR}/test_data --testname nn_train_xor)

install(DIRECTORY conf/
    DESTINATION bin)

install(TARGETS uochess
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include)
