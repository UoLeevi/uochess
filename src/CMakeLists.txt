cmake_minimum_required(VERSION 3.21.0)

find_package(Threads)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

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
  uo_math.c
  uo_test.c
  uo_book.c
  uo_tb.c)

target_include_directories(uochess
  PRIVATE
    "include"
    "syzygy")

target_link_libraries(uochess
  PRIVATE
    ${CMAKE_THREAD_LIBS_INIT})

target_compile_features(uochess
  PUBLIC
    c_std_11)

set_target_properties(uochess PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

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

add_custom_target(copy-runtime-files-syzygy ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/syzygy_tables" "${CMAKE_BINARY_DIR}/bin/syzygy_tables"
  DEPENDS uochess)

add_custom_target(copy-runtime-files-books ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/books" "${CMAKE_BINARY_DIR}/bin/books"
  DEPENDS uochess)

add_test(NAME "Test all"
  COMMAND uochess --test --datadir ${PROJECT_SOURCE_DIR}/test_data
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

add_test(NAME "Test move_generation"
  COMMAND uochess --test --datadir ${PROJECT_SOURCE_DIR}/test_data --testname move_generation
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

add_test(NAME "Test tb_probe"
  COMMAND uochess --test --datadir ${PROJECT_SOURCE_DIR}/test_data --testname tb_probe
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

add_test(NAME "Test matmul"
  COMMAND uochess --test --datadir ${PROJECT_SOURCE_DIR}/test_data --testname matmul
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

install(DIRECTORY conf/
  DESTINATION bin)

install(TARGETS uochess
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include)
