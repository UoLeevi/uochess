if (CMAKE_HOST_WIN32)
    # need at least CMake 3.23 for IntelLLVM support of IntelDPCPP package on Windows
    cmake_minimum_required(VERSION 3.23)
else()
    # CMake 3.20.5 is the minimum recommended for IntelLLVM on Linux
    cmake_minimum_required(VERSION 3.20.5)
endif()

find_package(Threads)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

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

# On Windows, creating symbolic links require elevated permissions. For CMake to work, enable Developer Mode:
# Start menu search: Use developer features -> Developer Mode = On

add_custom_target(link-runtime-files-syzygy ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_SOURCE_DIR}/syzygy_tables" "${CMAKE_BINARY_DIR}/bin/syzygy_tables"
  DEPENDS uochess)

add_custom_target(link-runtime-files-books ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_SOURCE_DIR}/books" "${CMAKE_BINARY_DIR}/bin/books"
  DEPENDS uochess)

add_test(NAME "Test move_generation"
  COMMAND uochess --test --datadir ${PROJECT_SOURCE_DIR}/test_data --testname move_generation
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

add_test(NAME "Test move_ordering"
  COMMAND uochess --test --datadir ${PROJECT_SOURCE_DIR}/test_data --testname move_ordering
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

add_test(NAME "Test search"
  COMMAND uochess --test --datadir ${PROJECT_SOURCE_DIR}/test_data --testname search
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

add_test(NAME "Test evaluation"
  COMMAND uochess --test --datadir ${PROJECT_SOURCE_DIR}/test_data --testname evaluation
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

add_test(NAME "Test tb_probe"
  COMMAND uochess --test --datadir ${PROJECT_SOURCE_DIR}/test_data --testname tb_probe
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

install(DIRECTORY conf/
  DESTINATION bin)

install(TARGETS uochess
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include)
