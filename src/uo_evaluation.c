#include "uo_evaluation.h"
#include "uo_util.h"
#include "uo_engine.h" // DEBUG
#include "uo_global.h" // DEBUG

#include <stdint.h>
#include <stdio.h> // DEBUG

#define uo_bitboard_mask_developed_N ((uo_bitboard)0x00007E7E7E7E0000)
#define uo_bitboard_mask_dangerous_own_N ((uo_bitboard)0x00007E7E7E000000)
#define uo_bitboard_mask_dangerous_enemy_N ((uo_bitboard)0x0000007E7E7E0000)
#define uo_bitboard_mask_developed_own_B ((uo_bitboard)0x0000183C3C7E4200)
#define uo_bitboard_mask_developed_enemy_B ((uo_bitboard)0x00427E3C3C180000)

// side to move
const int16_t uo_score_tempo = 20;

// material
const int16_t uo_score_P = 100;
const int16_t uo_score_N = 300;
const int16_t uo_score_B = 301;
const int16_t uo_score_R = 500;
const int16_t uo_score_Q = 900;

// pawns
const int16_t uo_score_center_P = 60;
const int16_t uo_score_doubled_P = -50;

const int16_t uo_score_passed_pawn = 20;
const int16_t uo_score_passed_pawn_on_fifth = 30;
const int16_t uo_score_passed_pawn_on_sixth = 70;
const int16_t uo_score_passed_pawn_on_seventh = 120;

// piece development
const int16_t uo_score_undeveloped_piece = -30;
const int16_t uo_score_rook_on_open_file = 30;

const int16_t uo_score_defended_by_pawn = 30;
const int16_t uo_score_attacked_by_pawn = 20;

const int16_t uo_score_knight_on_outpost = 40;
const int16_t uo_score_unattackable_by_pawn = 20;

// pieces relative to aiming at opponent king
const int16_t uo_score_Q_aiming_K = 25;
const int16_t uo_score_R_aiming_K = 40;
const int16_t uo_score_B_aiming_K = 50;
const int16_t uo_score_N_aiming_K = 50;

// king safety and castling
const int16_t uo_score_casting_right = 10;
const int16_t uo_score_king_in_the_center = -40;
const int16_t uo_score_castled_king = 50;
const int16_t uo_score_king_cover_pawn = 40;
const int16_t uo_score_king_next_to_open_file = -50;

int16_t uo_position_evaluate(uo_position *const position)
{
  int16_t score = uo_score_tempo;

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
  score += uo_score_P * ((int16_t)uo_popcnt(own_P) - (int16_t)uo_popcnt(enemy_P));
  score += uo_score_N * ((int16_t)uo_popcnt(own_N) - (int16_t)uo_popcnt(enemy_N));
  score += uo_score_B * ((int16_t)uo_popcnt(own_B) - (int16_t)uo_popcnt(enemy_B));
  score += uo_score_R * ((int16_t)uo_popcnt(own_R) - (int16_t)uo_popcnt(enemy_R));
  score += uo_score_Q * ((int16_t)uo_popcnt(own_Q) - (int16_t)uo_popcnt(enemy_Q));

  // DEBUG
  int material_score = score;

  // queen pointing towards opponent king
  score += uo_score_Q_aiming_K * ((int16_t)uo_popcnt(own_Q & (uo_square_bitboard_rays[square_enemy_K] | uo_square_bitboard_adjecent_rays[square_enemy_K]))
    - (int16_t)uo_popcnt(enemy_Q & (uo_square_bitboard_rays[square_own_K] | uo_square_bitboard_adjecent_rays[square_own_K])));

  // rook pointing towards opponent king
  score += uo_score_R_aiming_K * ((int16_t)uo_popcnt(own_R & (uo_square_bitboard_file[square_enemy_K] | uo_square_bitboard_adjecent_files[square_enemy_K]))
    - (int16_t)uo_popcnt(enemy_R & (uo_square_bitboard_file[square_own_K] | uo_square_bitboard_adjecent_files[square_own_K])));

  // bishop pointing towards opponent king
  score += uo_score_B_aiming_K * ((int16_t)uo_popcnt(own_B & (uo_square_bitboard_diagonals[square_enemy_K] | uo_square_bitboard_adjecent_diagonals[square_enemy_K]))
    - (int16_t)uo_popcnt(enemy_B & (uo_square_bitboard_diagonals[square_own_K] | uo_square_bitboard_adjecent_diagonals[square_own_K])));

  // knight very close to opponent king
  score += uo_score_N_aiming_K * ((int16_t)uo_popcnt(own_N & uo_square_bitboard_radius_two[square_enemy_K])
    - (int16_t)uo_popcnt(enemy_N & uo_square_bitboard_radius_two[square_own_K]));

  //// doupled pawns
  //score += uo_score_doubled_P * (((int16_t)uo_popcnt(own_P) - (int16_t)uo_popcnt(uo_bitboard_files(own_P) & uo_bitboard_rank_first))
  //  - ((int16_t)uo_popcnt(enemy_P) - (int16_t)uo_popcnt(uo_bitboard_files(enemy_P) & uo_bitboard_rank_first)));

  if (enemy_Q) // opening & middle game
  {
    uo_square square_enemy_Q = uo_tzcnt(enemy_Q);

    // center pawns
    score += uo_score_center_P * (int16_t)uo_popcnt(uo_bitboard_center & own_P);

    // first rank piece count (development)
    score += uo_score_undeveloped_piece * (int16_t)uo_popcnt((own_N | own_B | own_Q) & uo_bitboard_rank_first);

    // castling rights
    score += uo_score_casting_right * (uo_position_flags_castling_OO(position->flags) + uo_position_flags_castling_OOO(position->flags));

    // king safety
    score += uo_score_king_in_the_center * (0 != (own_K & (uo_bitboard_file[2] | uo_bitboard_file[3] | uo_bitboard_file[4] | uo_bitboard_file[5])));
    score += uo_score_castled_king * (square_own_K == uo_square__g1);
    score += uo_score_castled_king * (square_own_K == uo_square__b1);
    score += uo_score_king_cover_pawn * (int16_t)uo_popcnt(uo_bitboard_moves_K(square_own_K, ~own_P, own_P));
    //score += uo_score_king_next_to_open_file * (int16_t)uo_popcnt((uo_square_bitboard_adjecent_files[square_own_K] | uo_square_bitboard_file[square_own_K]) & uo_bitboard_files(own_P) & uo_bitboard_rank_first);

    //// rooks on open files
    //score += uo_score_rook_on_open_file * (int16_t)uo_popcnt(uo_andn(uo_bitboard_files(own_P), own_R));

    // pieces defended by pawns
    score += uo_score_defended_by_pawn * (int16_t)uo_popcnt(attacks_own_P & mask_own);

    // pieces attacked by pawns
    score += uo_score_attacked_by_pawn * (int16_t)uo_popcnt(attacks_own_P & mask_enemy);

    // developed pieces which cannot be attacked by pushing pawns
    score += uo_score_unattackable_by_pawn * (int16_t)uo_popcnt(uo_andn(potential_attacks_enemy_P, developed_own_N | developed_own_B));

    // knights on outpost
    score += uo_score_knight_on_outpost * (int16_t)uo_popcnt(uo_andn(potential_attacks_enemy_P, dangerous_own_N));
  }
  else // endgame
  {
    // passed pawns
    uo_bitboard passed_own_P = uo_bitboard_passed_P(own_P, enemy_P);

    if (passed_own_P)
    {
      score += uo_score_passed_pawn * (int16_t)uo_popcnt(passed_own_P);
      score += uo_score_passed_pawn_on_fifth * (int16_t)uo_popcnt(passed_own_P & uo_bitboard_rank_fifth);
      score += uo_score_passed_pawn_on_sixth * (int16_t)uo_popcnt(passed_own_P & uo_bitboard_rank_sixth);
      score += uo_score_passed_pawn_on_seventh * (int16_t)uo_popcnt(passed_own_P & uo_bitboard_rank_seventh);
    }
  }

  if (own_Q) // opening & middle game
  {
    uo_square square_own_Q = uo_tzcnt(own_Q);

    // center pawns
    score -= uo_score_center_P * (int16_t)uo_popcnt(uo_bitboard_center & enemy_P);

    // first rank piece count (development)
    score -= uo_score_undeveloped_piece * (int16_t)uo_popcnt((enemy_N | enemy_B | enemy_Q) & uo_bitboard_rank_last);

    // castling rights
    score -= uo_score_casting_right * (uo_position_flags_castling_enemy_OO(position->flags) + uo_position_flags_castling_enemy_OOO(position->flags));

    // king safety
    score -= uo_score_king_in_the_center * (0 != (enemy_K & (uo_bitboard_file[2] | uo_bitboard_file[3] | uo_bitboard_file[4] | uo_bitboard_file[5])));
    score -= uo_score_castled_king * (square_enemy_K == uo_square__g8);
    score -= uo_score_castled_king * (square_enemy_K == uo_square__b8);
    score -= uo_score_king_cover_pawn * (int16_t)uo_popcnt(uo_bitboard_moves_K(square_enemy_K, ~enemy_P, enemy_P));
    //score -= uo_score_king_next_to_open_file * (int16_t)uo_popcnt((uo_square_bitboard_adjecent_files[square_enemy_K] | uo_square_bitboard_file[square_enemy_K]) & uo_bitboard_files(enemy_P) & uo_bitboard_rank_first);

    //// rooks on open files
    //score -= uo_score_rook_on_open_file * (int16_t)uo_popcnt(uo_andn(uo_bitboard_files(enemy_P), enemy_R));

    // pieces defended by pawns
    score -= uo_score_defended_by_pawn * (int16_t)uo_popcnt(attacks_enemy_P & mask_enemy);

    // pieces attacked by pawns
    score -= uo_score_attacked_by_pawn * (int16_t)uo_popcnt(attacks_enemy_P & mask_own);

    // developed pieces which cannot be attacked by pushing pawns
    score -= uo_score_unattackable_by_pawn * (int16_t)uo_popcnt(uo_andn(potential_attacks_own_P, developed_enemy_N | developed_enemy_B));

    // knights on outpost
    score -= uo_score_knight_on_outpost * (int16_t)uo_popcnt(uo_andn(potential_attacks_own_P, dangerous_enemy_N));
  }
  else // endgame
  {
    // passed pawns
    uo_bitboard passed_enemy_P = uo_bitboard_passed_enemy_P(enemy_P, own_P);

    if (passed_enemy_P)
    {
      score -= uo_score_passed_pawn * (int16_t)uo_popcnt(passed_enemy_P);
      score -= uo_score_passed_pawn_on_fifth * (int16_t)uo_popcnt(passed_enemy_P & uo_bitboard_rank_fourth);
      score -= uo_score_passed_pawn_on_sixth * (int16_t)uo_popcnt(passed_enemy_P & uo_bitboard_rank_third);
      score -= uo_score_passed_pawn_on_seventh * (int16_t)uo_popcnt(passed_enemy_P & uo_bitboard_rank_second);
    }
  }

  // DEBUG
  if (material_score + 700 < score)
  {
    uo_engine_lock_stdout();
    printf("INVALID EVALUATION\n");
    uo_position_print_diagram(position, buf);
    printf("\n%s", buf);
    uo_position_print_fen(position, buf);
    printf("\n");
    printf("Fen: %s\n", buf);
    printf("Key: %" PRIu64 "\n", position->key);
    uo_engine_unlock_stdout();
  }

  return score;
}
