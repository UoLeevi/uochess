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
#define uo_bitboard_center ((uo_bitboard)0x0000001818000000ull)
#define uo_bitboard_big_center ((uo_bitboard)0x00003C3C3C3C0000ull)

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

  extern uo_bitboard uo_square_bitboard_file[64];         //  |
  extern uo_bitboard uo_square_bitboard_rank[64];         //  -
  extern uo_bitboard uo_square_bitboard_lines[64];        //  +
  extern uo_bitboard uo_square_bitboard_diagonal[64];     //  /
  extern uo_bitboard uo_square_bitboard_antidiagonal[64]; //  
  extern uo_bitboard uo_square_bitboard_diagonals[64];    //  X
  extern uo_bitboard uo_square_bitboard_rays[64];         //  *

  extern uo_bitboard uo_square_bitboard_adjecent_files[64];
  extern uo_bitboard uo_square_bitboard_adjecent_ranks[64];
  extern uo_bitboard uo_square_bitboard_adjecent_lines[64];
  extern uo_bitboard uo_square_bitboard_adjecent_diagonals[64];
  extern uo_bitboard uo_square_bitboard_adjecent_rays[64];

  extern uo_bitboard uo_square_bitboard_above[64];
  extern uo_bitboard uo_square_bitboard_below[64];
  extern uo_bitboard uo_square_bitboard_left[64];
  extern uo_bitboard uo_square_bitboard_right[64];
  extern uo_bitboard uo_square_bitboard_radius_two[64];

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

  static inline uo_bitboard uo_bitboard_files(uo_bitboard mask)
  {
    mask |= mask << 8;
    mask |= mask << 16;
    mask |= mask << 32;
    mask |= mask >> 8;
    mask |= mask >> 16;
    mask |= mask >> 32;
    return mask;
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
    uo_bitboard single_push = uo_bitboard_single_push_P(pawns & uo_bitboard_rank_second, empty);
    return uo_bitboard_single_push_P(single_push, empty);
  }

  static inline uo_bitboard uo_bitboard_single_push_enemy_P(uo_bitboard pawns, uo_bitboard empty)
  {
    return (pawns >> 8) & empty;
  }

  static inline uo_bitboard uo_bitboard_double_push_enemy_P(uo_bitboard pawns, uo_bitboard empty)
  {
    uo_bitboard single_push = uo_bitboard_single_push_enemy_P(pawns & uo_bitboard_rank_seventh, empty);
    return uo_bitboard_single_push_enemy_P(single_push, empty);
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

  static inline uo_bitboard uo_bitboard_passed_P(uo_bitboard own_P, uo_bitboard enemy_P)
  {
    uo_bitboard passed_P = 0;

    while (own_P)
    {
      uo_square square = uo_bitboard_next_square(&own_P);
      uo_bitboard pawn = uo_square_bitboard(square);
      uo_bitboard enemy_P_above = uo_bzlo(enemy_P, square + 2);
      uint8_t file = uo_square_file(square);
      uo_bitboard mask = uo_square_bitboard_file[square] | uo_square_bitboard_adjecent_files[square];

      if (!(enemy_P_above & mask))
      {
        passed_P |= pawn;
      }
    }

    return passed_P;
  }

  static inline uo_bitboard uo_bitboard_passed_enemy_P(uo_bitboard enemy_P, uo_bitboard own_P)
  {
    uo_bitboard passed_P = 0;

    while (enemy_P)
    {
      uo_square square = uo_bitboard_next_square(&enemy_P);
      uo_bitboard pawn = uo_square_bitboard(square);
      uo_bitboard own_P_below = uo_bzhi(own_P, square - 2);
      uint8_t file = uo_square_file(square);
      uo_bitboard mask = uo_square_bitboard_file[square] | uo_square_bitboard_adjecent_files[square];

      if (!(own_P_below & mask))
      {
        passed_P |= pawn;
      }
    }

    return passed_P;
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

  static inline uo_bitboard uo_bitboard_pins_B(uo_square square, uo_bitboard blockers, uo_bitboard diagonal_attackers)
  {
    uo_bitboard mask = uo_square_bitboard(square);
    uo_bitboard bitboard_diagonal = uo_andn(mask, uo_square_bitboard_diagonal[square]);
    uo_bitboard bitboard_antidiagonal = uo_andn(mask, uo_square_bitboard_antidiagonal[square]);
    uo_bitboard bitboard_both_diagonals = bitboard_diagonal | bitboard_antidiagonal;

    diagonal_attackers &= bitboard_both_diagonals;

    if (!diagonal_attackers) return 0;

    blockers &= bitboard_both_diagonals;

    uo_bitboard pins = 0;

    uo_bitboard antidiagonal_attackers = diagonal_attackers & bitboard_antidiagonal;
    uo_bitboard antidiagonal_blockers = blockers & bitboard_antidiagonal;

    if (antidiagonal_attackers && antidiagonal_blockers)
    {
      uo_bitboard antidiagonal_below = uo_bzhi(bitboard_antidiagonal, square);
      uo_bitboard antidiagonal_attackers_below = antidiagonal_attackers & antidiagonal_below;

      if (antidiagonal_attackers_below)
      {
        uo_square square_antidiagonal_attacker_below = uo_msb(antidiagonal_attackers_below);
        antidiagonal_below = uo_bzlo(antidiagonal_below, square_antidiagonal_attacker_below);
        uo_bitboard antidiagonal_blockers_below = antidiagonal_below & antidiagonal_blockers;

        if (uo_popcnt(antidiagonal_blockers_below) == 2) pins |= antidiagonal_below;
      }

      uo_bitboard antidiagonal_above = uo_bzlo(bitboard_antidiagonal, square);
      uo_bitboard antidiagonal_attackers_above = antidiagonal_attackers & antidiagonal_above;

      if (antidiagonal_attackers_above)
      {
        uo_square square_antidiagonal_attacker_above = uo_tzcnt(antidiagonal_attackers_above);
        antidiagonal_above = uo_bzhi(antidiagonal_above, square_antidiagonal_attacker_above + 1);
        uo_bitboard antidiagonal_blockers_above = antidiagonal_above & antidiagonal_blockers;

        if (uo_popcnt(antidiagonal_blockers_above) == 2) pins |= antidiagonal_above;
      }
    }

    diagonal_attackers &= bitboard_diagonal;
    uo_bitboard diagonal_blockers = blockers & bitboard_diagonal;

    if (diagonal_attackers && diagonal_blockers)
    {
      uo_bitboard diagonal_below = uo_bzhi(bitboard_diagonal, square);
      uo_bitboard diagonal_attackers_below = diagonal_attackers & diagonal_below;

      if (diagonal_attackers_below)
      {
        uo_square square_diagonal_attacker_below = uo_msb(diagonal_attackers_below);
        diagonal_below = uo_bzlo(diagonal_below, square_diagonal_attacker_below);
        uo_bitboard diagonal_blockers_below = diagonal_below & diagonal_blockers;

        if (uo_popcnt(diagonal_blockers_below) == 2) pins |= diagonal_below;
      }

      uo_bitboard diagonal_above = uo_bzlo(bitboard_diagonal, square);
      uo_bitboard diagonal_attackers_above = diagonal_attackers & diagonal_above;

      if (diagonal_attackers_above)
      {
        uo_square square_diagonal_attacker_above = uo_tzcnt(diagonal_attackers_above);
        diagonal_above = uo_bzhi(diagonal_above, square_diagonal_attacker_above + 1);
        uo_bitboard diagonal_blockers_above = diagonal_above & diagonal_blockers;

        if (uo_popcnt(diagonal_blockers_above) == 2) pins |= diagonal_above;
      }
    }

    return pins;
  }

  static inline uo_bitboard uo_bitboard_pins_R(uo_square square, uo_bitboard blockers, uo_bitboard line_attackers)
  {
    uo_bitboard mask = uo_square_bitboard(square);
    uo_bitboard bitboard_file = uo_andn(mask, uo_square_bitboard_file[square]);
    uo_bitboard bitboard_rank = uo_andn(mask, uo_square_bitboard_rank[square]);
    uo_bitboard bitboard_lines = bitboard_file | bitboard_rank;

    line_attackers &= bitboard_lines;

    if (!line_attackers) return 0;

    blockers &= bitboard_lines;

    uo_bitboard pins = 0;

    uo_bitboard rank_attackers = line_attackers & bitboard_rank;
    uo_bitboard rank_blockers = blockers & bitboard_rank;

    if (rank_attackers & rank_blockers)
    {
      uo_bitboard rank_left = uo_bzhi(bitboard_rank, square);
      uo_bitboard rank_attackers_left = rank_attackers & rank_left;

      if (rank_attackers_left)
      {
        uo_square square_rank_attacker_left = uo_msb(rank_attackers_left);
        rank_left = uo_bzlo(rank_left, square_rank_attacker_left);
        uo_bitboard rank_blockers_left = rank_left & rank_blockers;

        if (uo_popcnt(rank_blockers_left) == 2) pins |= rank_left;
      }

      uo_bitboard rank_right = uo_bzlo(bitboard_rank, square);
      uo_bitboard rank_attackers_right = rank_attackers & rank_right;

      if (rank_attackers_right)
      {
        uo_square square_rank_attacker_right = uo_tzcnt(rank_attackers_right);
        rank_right = uo_bzhi(rank_right, square_rank_attacker_right + 1);
        uo_bitboard rank_blockers_right = rank_right & rank_blockers;

        if (uo_popcnt(rank_blockers_right) == 2) pins |= rank_right;
      }
    }

    uo_bitboard file_attackers = line_attackers & bitboard_file;
    uo_bitboard file_blockers = blockers & bitboard_file;

    if (file_attackers && file_blockers)
    {
      uo_bitboard file_below = uo_bzhi(bitboard_file, square);
      uo_bitboard file_attackers_below = file_attackers & file_below;

      if (file_attackers_below)
      {
        uo_square square_file_attacker_below = uo_msb(file_attackers_below);
        file_below = uo_bzlo(file_below, square_file_attacker_below);
        uo_bitboard file_blockers_below = file_below & file_blockers;

        if (uo_popcnt(file_blockers_below) == 2) pins |= file_below;
      }

      uo_bitboard file_above = uo_bzlo(bitboard_file, square);
      uo_bitboard file_attackers_above = file_attackers & file_above;

      if (file_attackers_above)
      {
        uo_square square_file_attacker_above = uo_tzcnt(file_attackers_above);
        file_above = uo_bzhi(file_above, square_file_attacker_above + 1);
        uo_bitboard file_blockers_above = file_above & file_blockers;

        if (uo_popcnt(file_blockers_above) == 2) pins |= file_above;
      }
    }

    return pins;
  }

  void uo_bitboard_init();

#ifdef __cplusplus
}
#endif

#endif
