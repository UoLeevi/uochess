cmake_minimum_required(VERSION 3.12)

if(NOT TARGET uo::uo_macro)
    find_package(uo_macro CONFIG REQUIRED)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set(CMAKE_CXX_FLAGS_DEBUG "/ZI")
  set(CMAKE_SHARED_LINKER_FLAGS "/SAFESEH:NO")
  set(CMAKE_EXE_LINKER_FLAGS "/SAFESEH:NO")
endif()

add_executable(uochess
    uochess.c
    uo_bitboard.c
    uo_position.c
    uo_piece.c
    uo_square.c
    uo_moves.c
    uo_util.c)

target_include_directories(uochess
    PRIVATE
        include)

target_link_libraries(uochess
    PRIVATE
        uo::uo_macro)

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
