cmake_minimum_required(VERSION 3.21.0)

find_package(Threads)

find_package(Torch REQUIRED HINT "${Torch_DIR}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

add_library(uo_nn_impl
  "uo_nn_impl.cpp")

target_include_directories(uo_nn_impl
  PRIVATE
    "include"
    "${Torch_DIR}/include")

target_link_libraries(uo_nn_impl "${TORCH_LIBRARIES}")



# The following code block is suggested to be used on Windows.
# According to https://github.com/pytorch/pytorch/issues/25457,
# the DLLs need to be copied to avoid memory errors.
if (MSVC)
  file(GLOB TORCH_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll")
  add_custom_command(TARGET uo_nn_impl
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${TORCH_DLLS}
    "${CMAKE_BINARY_DIR}/bin/")
endif (MSVC)

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
    "include")

target_link_libraries(uochess
  PRIVATE
    ${CMAKE_THREAD_LIBS_INIT}
    uo_nn_impl)

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
  COMMAND uochess --test --datadir ${PROJECT_SOURCE_DIR}/test_data
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/src")

add_test(NAME "Test move_generation"
  COMMAND uochess --test --datadir ${PROJECT_SOURCE_DIR}/test_data --testname move_generation
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/src")

add_test(NAME "Test matmul"
  COMMAND uochess --test --datadir ${PROJECT_SOURCE_DIR}/test_data --testname matmul
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/src")

add_test(NAME "Test nn_train_xor"
  COMMAND uochess --test --datadir ${PROJECT_SOURCE_DIR}/test_data --testname nn_train_xor
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/src")

set_tests_properties("Test nn_train_xor"
  PROPERTIES ENVIRONMENT "UO_OPT_NNDIR=nn")

install(DIRECTORY conf/
  DESTINATION bin)

install(TARGETS uochess
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include)
