#ifndef UO_BITBOARD_H
#define UO_BITBOARD_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_square.h"

#include <stdbool.h>
#include <inttypes.h>

  // Little endian rank-file (LERF) mapping

  typedef uint64_t uo_bitboard;

#define uo_bitboard_all ((uo_bitboard)-1ll)

  extern uo_bitboard uo_bitboard_file[8];          //  |
  extern uo_bitboard uo_bitboard_rank[8];          //  -
  extern uo_bitboard uo_bitboard_diagonal[15];     //  /
  extern uo_bitboard uo_bitboard_antidiagonal[15]; //  \

  int uo_bitboard_print(uo_bitboard bitboard);

  bool uo_bitboard_next_square(uo_bitboard bitboard, uo_square* square);

  void uo_bitboard_init();

#ifdef __cplusplus
}
#endif

#endif
