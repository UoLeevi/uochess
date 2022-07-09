#ifndef UO_BITBOARD_H
#define UO_BITBOARD_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_square.h"
#include "uo_piece.h"
#include "uo_util.h"

#include <stdbool.h>
#include <inttypes.h>

  // Little endian rank-file (LERF) mapping

  typedef uint64_t uo_bitboard;

  typedef struct uo_slider_moves
  {
    uo_bitboard *moves;
    uo_bitboard mask;
  } uo_slider_moves;

#define uo_bitboard_all ((uo_bitboard)-1ll)
#define uo_bitboard_edge ((uo_bitboard)0xFF818181818181FFull)

#define uo_bitboard_rank_first   ((uo_bitboard)0x00000000000000FFull)
#define uo_bitboard_rank_second  ((uo_bitboard)0x000000000000FF00ull)
#define uo_bitboard_rank_third   ((uo_bitboard)0x0000000000FF0000ull)
#define uo_bitboard_rank_fourth  ((uo_bitboard)0x00000000FF000000ull)
#define uo_bitboard_rank_fifth   ((uo_bitboard)0x000000FF00000000ull)
#define uo_bitboard_rank_sixth   ((uo_bitboard)0x0000FF0000000000ull)
#define uo_bitboard_rank_seventh ((uo_bitboard)0x00FF000000000000ull)
#define uo_bitboard_rank_last    ((uo_bitboard)0xFF00000000000000ull)

  extern uo_bitboard uo_bitboard_file[8];          //  |
  extern uo_bitboard uo_bitboard_rank[8];          //  -
  extern uo_bitboard uo_bitboard_diagonal[15];     //  /
  extern uo_bitboard uo_bitboard_antidiagonal[15]; //  \

  extern uo_bitboard uo_moves_K[64];
  extern uo_bitboard uo_moves_N[64];
  extern uo_bitboard uo_attacks_P[2][64];
  extern uo_slider_moves uo_moves_B[64];
  extern uo_slider_moves uo_moves_R[64];

  int uo_bitboard_print(uo_bitboard bitboard);

  static inline uo_square uo_bitboard_next_square(uo_bitboard *bitboard)
  {
    uo_square lsb = uo_tzcnt(*bitboard);
    *bitboard = uo_blsr(*bitboard);
    return lsb;
  }

  static inline uo_bitboard uo_bitboard_attacks_N(uo_square square)
  {
    return uo_moves_N[square];
  }

  static inline uo_bitboard uo_bitboard_attacks_B(uo_square square, uo_bitboard blockers)
  {
    uo_slider_moves slider_moves = uo_moves_B[square];
    return slider_moves.moves[uo_pext(blockers, slider_moves.mask)];
  }

  static inline uo_bitboard uo_bitboard_attacks_R(uo_square square, uo_bitboard blockers)
  {
    uo_slider_moves slider_moves = uo_moves_R[square];
    return slider_moves.moves[uo_pext(blockers, slider_moves.mask)];
  }

  static inline uo_bitboard uo_bitboard_attacks_K(uo_square square)
  {
    return uo_moves_K[square];
  }

  static inline uo_bitboard uo_bitboard_attacks_P(uo_square square, uint8_t color)
  {
    return uo_attacks_P[color][square];
  }

  static inline uo_bitboard uo_bitboard_attacks_Q(uo_square square, uo_bitboard blockers)
  {
    return uo_bitboard_attacks_B(square, blockers) | uo_bitboard_attacks_R(square, blockers);
  }

  static inline uo_bitboard uo_bitboard_single_push_P(uo_bitboard pawns, uo_bitboard empty)
  {
    return (pawns << 8) & empty;
  }

  static inline uo_bitboard uo_bitboard_double_push_P(uo_bitboard pawns, uo_bitboard empty)
  {
    uo_bitboard bitboard_second_rank = (uo_bitboard)0x000000000000FF00;
    uo_bitboard single_push = uo_bitboard_single_push_P(pawns & bitboard_second_rank, empty);
    return uo_bitboard_single_push_P(single_push, empty);
  }

  static inline uo_bitboard uo_bitboard_attacks_left_P(uo_bitboard pawns)
  {
    return (pawns & (uo_bitboard)0x00FEFEFEFEFEFE00) << 7;
  }

  static inline uo_bitboard uo_bitboard_attacks_right_P(uo_bitboard pawns)
  {
    return (pawns & (uo_bitboard)0x007F7F7F7F7F7F00) << 9;
  }

  static inline uo_bitboard uo_bitboard_attacks_left_enemy_P(uo_bitboard pawns)
  {
    return (pawns & (uo_bitboard)0x00FEFEFEFEFEFE00) >> 9;
  }

  static inline uo_bitboard uo_bitboard_attacks_right_enemy_P(uo_bitboard pawns)
  {
    return (pawns & (uo_bitboard)0x007F7F7F7F7F7F00) >> 7;
  }

  static inline uo_bitboard uo_bitboard_attacks_enemy_P(uo_bitboard pawns)
  {
    return uo_bitboard_attacks_left_enemy_P(pawns) | uo_bitboard_attacks_right_enemy_P(pawns);
  }

  static inline uo_bitboard uo_bitboard_captures_left_P(uo_bitboard pawns, uo_bitboard enemy)
  {
    return uo_bitboard_attacks_left_P(pawns) & enemy;
  }

  static inline uo_bitboard uo_bitboard_captures_right_P(uo_bitboard pawns, uo_bitboard enemy)
  {
    return uo_bitboard_attacks_right_P(pawns) & enemy;
  }

  static inline uo_bitboard uo_bitboard_moves_N(uo_square square, uo_bitboard own, uo_bitboard enemy)
  {
    return uo_andn(own, uo_bitboard_attacks_N(square));
  }

  static inline uo_bitboard uo_bitboard_moves_B(uo_square square, uo_bitboard own, uo_bitboard enemy)
  {
    return uo_andn(own, uo_bitboard_attacks_B(square, own | enemy));
  }

  static inline uo_bitboard uo_bitboard_moves_R(uo_square square, uo_bitboard own, uo_bitboard enemy)
  {
    return uo_andn(own, uo_bitboard_attacks_R(square, own | enemy));
  }

  static inline uo_bitboard uo_bitboard_moves_Q(uo_square square, uo_bitboard own, uo_bitboard enemy)
  {
    return uo_andn(own, uo_bitboard_attacks_Q(square, own | enemy));
  }

  static inline uo_bitboard uo_bitboard_moves_K(uo_square square, uo_bitboard own, uo_bitboard enemy)
  {
    return uo_andn(own, uo_bitboard_attacks_K(square));
  }

  // pins and discoveries
  uo_bitboard uo_bitboard_pins(uo_square square, uo_bitboard blockers, uo_bitboard diagonal_attackers, uo_bitboard line_attackers);
  uo_bitboard uo_bitboard_pins_B(uo_square square, uo_bitboard blockers, uo_bitboard diagonal_attackers);
  uo_bitboard uo_bitboard_pins_R(uo_square square, uo_bitboard blockers, uo_bitboard line_attackers);

  void uo_bitboard_init();

#ifdef __cplusplus
}
#endif

#endif
