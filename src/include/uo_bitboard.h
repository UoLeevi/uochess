#ifndef UO_BITBOARD_H
#define UO_BITBOARD_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_square.h"
#include "uo_piece.h"

#include <stdbool.h>
#include <inttypes.h>

  // Little endian rank-file (LERF) mapping

  typedef uint64_t uo_bitboard;

#define uo_bitboard_all ((uo_bitboard)-1ll)
#define uo_bitboard_edge ((uo_bitboard)0xFF818181818181FFull)

  extern uo_bitboard uo_bitboard_file[8];          //  |
  extern uo_bitboard uo_bitboard_rank[8];          //  -
  extern uo_bitboard uo_bitboard_diagonal[15];     //  /
  extern uo_bitboard uo_bitboard_antidiagonal[15]; //  \

  int uo_bitboard_print(uo_bitboard bitboard);

  bool uo_bitboard_next_square(uo_bitboard bitboard, uo_square *square);

  uo_bitboard uo_bitboard_attacks_P(uo_square square, uint8_t color);
  uo_bitboard uo_bitboard_attacks_N(uo_square square);
  uo_bitboard uo_bitboard_attacks_B(uo_square square, uo_bitboard blockers);
  uo_bitboard uo_bitboard_attacks_R(uo_square square, uo_bitboard blockers);
  uo_bitboard uo_bitboard_attacks_K(uo_square square);

  static inline uo_bitboard uo_bitboard_attacks_Q(uo_square square, uo_bitboard blockers)
  {
    return uo_bitboard_attacks_B(square, blockers) | uo_bitboard_attacks_R(square, blockers);
  }

  uo_bitboard uo_bitboard_moves_P(uo_square square, uint8_t color, uo_bitboard own, uo_bitboard enemy);

  static inline uo_bitboard uo_bitboard_moves_N(uo_square square, uo_bitboard own, uo_bitboard enemy)
  {
    return uo_bitboard_attacks_N(square) & ~own;
  }

  static inline uo_bitboard uo_bitboard_moves_B(uo_square square, uo_bitboard own, uo_bitboard enemy)
  {
    return uo_bitboard_attacks_B(square, own | enemy) & ~own;
  }

  static inline uo_bitboard uo_bitboard_moves_R(uo_square square, uo_bitboard own, uo_bitboard enemy)
  {
    return uo_bitboard_attacks_R(square, own | enemy) & ~own;
  }

  static inline uo_bitboard uo_bitboard_moves_Q(uo_square square, uo_bitboard own, uo_bitboard enemy)
  {
    return uo_bitboard_attacks_Q(square, own | enemy) & ~own;
  }

  static inline uo_bitboard uo_bitboard_moves_K(uo_square square, uo_bitboard own, uo_bitboard enemy)
  {
    return uo_bitboard_attacks_K(square) & ~own;
  }

  // pins and discoveries
  uo_bitboard uo_bitboard_pins(uo_square square, uo_bitboard blockers, uo_bitboard diagonal_attackers, uo_bitboard line_attackers);

  void uo_bitboard_init();

#ifdef __cplusplus
}
#endif

#endif
