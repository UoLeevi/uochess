#include "uo_evaluation.h"
#include "uo_util.h"

#include <inttypes.h>

#define uo_bitboard_mask_developed_N ((uo_bitboard)0x00007E7E7E7E0000)
#define uo_bitboard_mask_dangerous_own_N ((uo_bitboard)0x00007E7E7E000000)
#define uo_bitboard_mask_dangerous_enemy_N ((uo_bitboard)0x0000007E7E7E0000)
#define uo_bitboard_mask_developed_own_B ((uo_bitboard)0x0000183C3C7E4200)
#define uo_bitboard_mask_developed_enemy_B ((uo_bitboard)0x00427E3C3C180000)

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

  uo_bitboard pushes_own_P = uo_bitboard_single_push_P(own_P, empty) | uo_bitboard_double_push_P(own_P, empty);
  uo_bitboard pushes_enemy_P = uo_bitboard_single_push_enemy_P(enemy_P, empty) | uo_bitboard_double_push_enemy_P(enemy_P, empty);

  uo_bitboard attacks_own_P = uo_bitboard_attacks_left_P(own_P) | uo_bitboard_attacks_right_P(own_P);
  uo_bitboard attacks_enemy_P = uo_bitboard_attacks_left_enemy_P(enemy_P) | uo_bitboard_attacks_right_enemy_P(enemy_P);

  uo_bitboard potential_attacks_own_P = attacks_own_P | uo_bitboard_attacks_left_P(pushes_own_P) | uo_bitboard_attacks_right_P(pushes_own_P);
  uo_bitboard potential_attacks_enemy_P = attacks_enemy_P | uo_bitboard_attacks_left_enemy_P(pushes_enemy_P) | uo_bitboard_attacks_right_enemy_P(pushes_enemy_P);

  uo_bitboard developed_own_N = own_N & uo_bitboard_mask_developed_N;
  uo_bitboard developed_enemy_N = enemy_N & uo_bitboard_mask_developed_N;
  uo_bitboard dangerous_own_N = own_N & uo_bitboard_mask_dangerous_own_N;
  uo_bitboard dangerous_enemy_N = enemy_N & uo_bitboard_mask_dangerous_enemy_N;
  uo_bitboard developed_own_B = own_B & uo_bitboard_mask_developed_own_B;
  uo_bitboard developed_enemy_B = enemy_B & uo_bitboard_mask_developed_enemy_B;

  // material
  score += 100 * ((int16_t)uo_popcnt(own_P) - (int16_t)uo_popcnt(enemy_P));
  score += 300 * ((int16_t)uo_popcnt(own_N) - (int16_t)uo_popcnt(enemy_N));
  score += 301 * ((int16_t)uo_popcnt(own_B) - (int16_t)uo_popcnt(enemy_B));
  score += 500 * ((int16_t)uo_popcnt(own_R) - (int16_t)uo_popcnt(enemy_R));
  score += 900 * ((int16_t)uo_popcnt(own_Q) - (int16_t)uo_popcnt(enemy_Q));

    // queen opointing towards opponent king
  score += 25 * ((int16_t)uo_popcnt(own_Q & (uo_square_bitboard_rays[square_enemy_K] | uo_square_bitboard_adjecent_rays[square_enemy_K]))
    - (int16_t)uo_popcnt(enemy_Q & (uo_square_bitboard_rays[square_own_K] | uo_square_bitboard_adjecent_rays[square_own_K])));

  // rook pointing towards opponent king
  score += 40 * ((int16_t)uo_popcnt(own_R & (uo_square_bitboard_file[square_enemy_K] | uo_square_bitboard_adjecent_files[square_enemy_K]))
    - (int16_t)uo_popcnt(enemy_R & (uo_square_bitboard_file[square_own_K] | uo_square_bitboard_adjecent_files[square_own_K])));

  // bishop opointing towards opponent king
  score += 50 * ((int16_t)uo_popcnt(own_B & (uo_square_bitboard_diagonals[square_enemy_K] | uo_square_bitboard_adjecent_diagonals[square_enemy_K]))
    - (int16_t)uo_popcnt(enemy_B & (uo_square_bitboard_diagonals[square_own_K] | uo_square_bitboard_adjecent_diagonals[square_own_K])));

  if (enemy_Q) // opening & middle game
  {
    uo_square square_enemy_Q = uo_tzcnt(enemy_Q);

    // first rank piece count (development)
    score -= 25 * (int16_t)uo_popcnt((own_N | own_B | own_Q) & uo_bitboard_rank_first);

    // castling rights
    score += 10 * (int16_t)(uo_position_flags_castling_OO(position->flags) + uo_position_flags_castling_OOO(position->flags));

    // king safety
    score += 50 * (int16_t)(square_own_K == uo_square__g1);
    score += 40 * (int16_t)(square_own_K == uo_square__b1);
    score += 40 * (int16_t)uo_popcnt(uo_bitboard_moves_K(square_own_K, ~own_P, own_P));
    score -= 90 * (int16_t)uo_popcnt((uo_square_bitboard_adjecent_files[square_own_K] | uo_square_bitboard_file[square_own_K]) & uo_bitboard_files(own_P) & uo_bitboard_rank_first);

    // rooks on open files
    score -= 20 * (int16_t)uo_popcnt(uo_bitboard_files(own_R) & own_P);

    // rook on same file or rank as opponent queen
    score += 5 * (int16_t)uo_popcnt(uo_square_bitboard_lines[square_enemy_Q]);

    // bishop on same diagonal as opponent queen
    score += 5 * (int16_t)uo_popcnt(own_B & uo_square_bitboard_diagonals[square_enemy_Q]);

    // pieces protected by pawns
    score += 15 * (int16_t)uo_popcnt(attacks_own_P & mask_own);

    // pieces attacked by pawns
    score += 20 * (int16_t)uo_popcnt(attacks_own_P & uo_andn(enemy_P, mask_enemy));

    // developed pieces which cannot be attacked by pushing pawns
    score += 15 * (int16_t)uo_popcnt(uo_andn(potential_attacks_enemy_P, developed_own_N | developed_own_B));

    // knights on outpost
    score += 30 * (int16_t)uo_popcnt(uo_andn(potential_attacks_enemy_P, dangerous_own_N));
  }
  else // endgame
  {
    // passed pawns
    uo_bitboard passed_own_P = uo_bitboard_passed_P(own_P, enemy_P);

    if (passed_own_P)
    {
      score += 20 * (int16_t)uo_popcnt(passed_own_P);
      score += 30 * (int16_t)uo_popcnt(passed_own_P & uo_bitboard_rank_fifth);
      score += 70 * (int16_t)uo_popcnt(passed_own_P & uo_bitboard_rank_sixth);
      score += 120 * (int16_t)uo_popcnt(passed_own_P & uo_bitboard_rank_seventh);
    }
  }

  if (own_Q) // opening & middle game
  {
    uo_square square_own_Q = uo_tzcnt(own_Q);

    // first rank piece count (development)
    score += 10 * (int16_t)uo_popcnt((enemy_N | enemy_B | enemy_Q) & uo_bitboard_rank_last);

    // castling rights
    score -= 10 * (int16_t)(uo_position_flags_castling_enemy_OO(position->flags) + uo_position_flags_castling_enemy_OOO(position->flags));

    // king safety
    score -= 50 * (int16_t)(square_enemy_K == uo_square__g8);
    score -= 40 * (int16_t)(square_enemy_K == uo_square__b8);
    score -= 40 * (int16_t)uo_popcnt(uo_bitboard_moves_K(square_enemy_K, ~enemy_P, enemy_P));
    score += 90 * (int16_t)uo_popcnt((uo_square_bitboard_adjecent_files[square_enemy_K] | uo_square_bitboard_file[square_enemy_K]) & uo_bitboard_files(enemy_P) & uo_bitboard_rank_first);

    // rooks on open files
    score += 20 * (int16_t)uo_popcnt(uo_bitboard_files(enemy_R) & enemy_P);

    // rook on same file or rank as opponent queen
    score -= 5 * (int16_t)uo_popcnt(uo_square_bitboard_lines[square_own_Q]);

    // bishop on same diagonal as opponent queen
    score -= 5 * (int16_t)uo_popcnt(enemy_B & uo_square_bitboard_diagonals[square_own_Q]);

    // pieces protected by pawns
    score -= 15 * (int16_t)uo_popcnt(attacks_enemy_P & mask_enemy);

    // pieces attacked by pawns
    score -= 20 * (int16_t)uo_popcnt(attacks_enemy_P & uo_andn(own_P, mask_own));

    // developed pieces which cannot be attacked by pushing pawns
    score -= 15 * (int16_t)uo_popcnt(uo_andn(potential_attacks_own_P, developed_enemy_N | developed_enemy_B));

    // knights on outpost
    score -= 30 * (int16_t)uo_popcnt(uo_andn(potential_attacks_own_P, dangerous_enemy_N));
  }
  else // endgame
  {
    // passed pawns
    uo_bitboard passed_enemy_P = uo_bitboard_passed_enemy_P(enemy_P, own_P);

    if (passed_enemy_P)
    {
      score -= 20 * (int16_t)uo_popcnt(passed_enemy_P);
      score -= 30 * (int16_t)uo_popcnt(passed_enemy_P & uo_bitboard_rank_fourth);
      score -= 70 * (int16_t)uo_popcnt(passed_enemy_P & uo_bitboard_rank_third);
      score -= 120 * (int16_t)uo_popcnt(passed_enemy_P & uo_bitboard_rank_second);
    }
  }

  return score;
}
