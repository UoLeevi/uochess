#include "uo_evaluation.h"
#include "uo_util.h"

#include <inttypes.h>

int16_t uo_position_evaluate(uo_position *const position)
{
  int16_t score = 0;

  uo_bitboard mask_own = position->own;
  uo_bitboard mask_enemy = position->enemy;
  uo_bitboard occupied = mask_own | mask_enemy;
  uo_bitboard empty = ~occupied;

  uo_bitboard own_P = mask_own & position->P;
  uo_bitboard own_N = mask_own & position->N;
  uo_bitboard own_B = mask_own & position->B;
  uo_bitboard own_R = mask_own & position->R;
  uo_bitboard own_Q = mask_own & position->Q;
  uo_bitboard own_K = mask_own & position->K;
  uo_bitboard enemy_P = mask_enemy & position->P;
  uo_bitboard enemy_N = mask_enemy & position->N;
  uo_bitboard enemy_B = mask_enemy & position->B;
  uo_bitboard enemy_R = mask_enemy & position->R;
  uo_bitboard enemy_Q = mask_enemy & position->Q;
  uo_bitboard enemy_K = mask_enemy & position->K;

  uo_square square_own_K = uo_tzcnt(own_K);
  uo_square square_enemy_K = uo_tzcnt(enemy_K);

  uo_bitboard diagonals_own_K = uo_square_bitboard_diagonals[square_own_K];
  uo_bitboard diagonals_enemy_K = uo_square_bitboard_diagonals[square_enemy_K];

  uo_bitboard diagonals_own_Q = own_Q ? uo_square_bitboard_diagonals[uo_tzcnt(own_Q)] : 0;
  uo_bitboard diagonals_enemy_Q = enemy_Q ? uo_square_bitboard_diagonals[uo_tzcnt(enemy_Q)] : 0;

  uo_bitboard files_own_R = uo_bitboard_files(own_R);
  uo_bitboard files_enemy_R = uo_bitboard_files(enemy_R);

  // material
  score += 100 * ((int16_t)uo_popcnt(own_P) - (int16_t)uo_popcnt(enemy_P));
  score += 300 * ((int16_t)uo_popcnt(own_N) - (int16_t)uo_popcnt(enemy_N));
  score += 301 * ((int16_t)uo_popcnt(own_B) - (int16_t)uo_popcnt(enemy_B));
  score += 500 * ((int16_t)uo_popcnt(own_R) - (int16_t)uo_popcnt(enemy_R));
  score += 900 * ((int16_t)uo_popcnt(own_Q) - (int16_t)uo_popcnt(enemy_Q));

  // first rank piece count (development)
  score += 1 * ((int16_t)uo_popcnt(mask_enemy & uo_bitboard_rank_last) - (int16_t)uo_popcnt(mask_own & uo_bitboard_rank_first));

  // castling rights
  score += 1 * (uo_position_flags_castling_OO(position->flags) + uo_position_flags_castling_OOO(position->flags)
    - uo_position_flags_castling_enemy_OO(position->flags) - uo_position_flags_castling_enemy_OOO(position->flags));

  // king safety
  score += 5 * ((square_own_K == uo_square__g1) - (square_enemy_K == uo_square__g8));
  score += 4 * ((square_own_K == uo_square__b1) - (square_enemy_K == uo_square__b8));
  score += 2 * ((int16_t)uo_popcnt(uo_bitboard_moves_K(square_own_K, ~(own_P), own_P))
    - (int16_t)uo_popcnt(uo_bitboard_moves_K(square_enemy_K, ~(enemy_P), enemy_P)));

  // centralized knights
  score += 3 * ((int16_t)uo_popcnt(enemy_N & uo_bitboard_edge) - (int16_t)uo_popcnt(own_N & uo_bitboard_edge));

  // rooks on open files
  score += 4 * ((int16_t)uo_popcnt(files_enemy_R & enemy_P)
    - (int16_t)uo_popcnt(files_own_R & own_P));

  // rook on same file as opponent king
  score += 3 * ((int16_t)uo_popcnt(files_own_R & enemy_K) - (int16_t)uo_popcnt(files_enemy_R & own_K));

  // rook on same file as opponent queen
  score += 1 * ((int16_t)uo_popcnt(files_own_R & enemy_Q) - (int16_t)uo_popcnt(files_enemy_R & own_Q));

  // bishop on same diagonal as opponent king
  score += 3 * ((int16_t)uo_popcnt(own_B & diagonals_enemy_K) - (int16_t)uo_popcnt(enemy_B & diagonals_own_K));

  // bishop on same diagonal as opponent queen
  score += 1 * ((int16_t)uo_popcnt(own_B & diagonals_enemy_Q) - (int16_t)uo_popcnt(enemy_B & diagonals_own_Q));

  // pieces protected by pawns
  score += 3 * ((int16_t)uo_popcnt((uo_bitboard_attacks_left_P(own_P) | uo_bitboard_attacks_right_P(own_P)) & mask_own)
    - (int16_t)uo_popcnt((uo_bitboard_attacks_left_enemy_P(enemy_P) | uo_bitboard_attacks_right_enemy_P(enemy_P)) & mask_enemy));

  return score;
}
