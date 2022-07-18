#include "uo_evaluation.h"
#include "uo_util.h"

#include <inttypes.h>

int16_t uo_position_evaluate(uo_position *const position)
{
  int16_t score = 0;

  // material
  score += 100 * ((int16_t)uo_popcnt(position->own & position->P) - (int16_t)uo_popcnt(position->enemy & position->P));
  score += 300 * ((int16_t)uo_popcnt(position->own & position->N) - (int16_t)uo_popcnt(position->enemy & position->N));
  score += 301 * ((int16_t)uo_popcnt(position->own & position->B) - (int16_t)uo_popcnt(position->enemy & position->B));
  score += 500 * ((int16_t)uo_popcnt(position->own & position->R) - (int16_t)uo_popcnt(position->enemy & position->R));
  score += 900 * ((int16_t)uo_popcnt(position->own & position->Q) - (int16_t)uo_popcnt(position->enemy & position->Q));

  // first rank piece count (development)
  score += 1 * ((int16_t)uo_popcnt(position->enemy & uo_bitboard_rank_last) - (int16_t)uo_popcnt(position->own & uo_bitboard_rank_first));

  // castling rights
  score += 1 * (uo_position_flags_castling_OO(position->flags) + uo_position_flags_castling_OOO(position->flags)
    - uo_position_flags_castling_enemy_OO(position->flags) - uo_position_flags_castling_enemy_OOO(position->flags));

  // king safety
  uo_bitboard own_K = position->own & position->K;
  uo_square square_own_K = uo_tzcnt(own_K);
  uo_bitboard enemy_K = position->enemy & position->K;
  uo_square square_enemy_K = uo_tzcnt(enemy_K);
  score += 5 * ((square_own_K == uo_square__g1) - (square_enemy_K == uo_square__g8));
  score += 4 * ((square_own_K == uo_square__b1) - (square_enemy_K == uo_square__b8));
  score += 2 * ((int16_t)uo_popcnt(uo_bitboard_moves_K(square_own_K, ~(position->own & position->P), position->own & position->P))
    - (int16_t)uo_popcnt(uo_bitboard_moves_K(square_enemy_K, ~(position->enemy & position->P), position->enemy & position->P)));

  // centralized knights
  score += 3 * ((int16_t)uo_popcnt(position->enemy & position->N & uo_bitboard_edge) - (int16_t)uo_popcnt(position->own & position->N & uo_bitboard_edge));

  // rooks on open files
  score += 4 * ((int16_t)uo_popcnt(uo_bitboard_files(position->enemy & position->R) & (position->enemy & position->P))
    - (int16_t)uo_popcnt(uo_bitboard_files(position->own & position->R) & (position->own & position->P)));

  // pieces protected by pawns
  score += 3 * ((int16_t)uo_popcnt((uo_bitboard_attacks_left_P(position->own & position->P) | uo_bitboard_attacks_right_P(position->own & position->P)) & position->own)
    - (int16_t)uo_popcnt((uo_bitboard_attacks_left_enemy_P(position->enemy & position->P) | uo_bitboard_attacks_right_enemy_P(position->enemy & position->P)) & position->enemy));

  return score;
}
