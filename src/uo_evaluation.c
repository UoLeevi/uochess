#include "uo_evaluation.h"
#include "uo_util.h"

#include <stdint.h>

#define uo_bitboard_mask_developed_N ((uo_bitboard)0x00007E7E7E7E0000)
#define uo_bitboard_mask_dangerous_own_N ((uo_bitboard)0x00007E7E7E000000)
#define uo_bitboard_mask_dangerous_enemy_N ((uo_bitboard)0x0000007E7E7E0000)
#define uo_bitboard_mask_developed_own_B ((uo_bitboard)0x0000183C3C7E4200)
#define uo_bitboard_mask_developed_enemy_B ((uo_bitboard)0x00427E3C3C180000)

// side to move
const int16_t uo_score_tempo = 20;

// mobility
const int16_t uo_score_square_access = 10;

const int16_t uo_score_mobility_P = 5;
const int16_t uo_score_mobility_N = 2;
const int16_t uo_score_mobility_B = 2;
const int16_t uo_score_mobility_R = 2;
const int16_t uo_score_mobility_Q = 1;
const int16_t uo_score_mobility_K = 2;

const int16_t uo_score_N_square_attackable_by_P = -20;
const int16_t uo_score_B_square_attackable_by_P = -10;
const int16_t uo_score_R_square_attackable_by_P = -30;
const int16_t uo_score_Q_square_attackable_by_P = -50;
const int16_t uo_score_K_square_attackable_by_P = -70;

// space
const int16_t uo_score_space_P = 10;
const int16_t uo_score_space_N = 2;
const int16_t uo_score_space_B = 2;
const int16_t uo_score_space_R = 8;
const int16_t uo_score_space_Q = 1;

// material
const int16_t uo_score_P = 100;
const int16_t uo_score_N = 450;
const int16_t uo_score_B = 460;
const int16_t uo_score_R = 800;
const int16_t uo_score_Q = 1750;

const int16_t uo_score_material_max = (
  16 * 100 +
  4 * 450 +
  4 * 460 +
  4 * 800 +
  2 * 1750);

const int16_t uo_score_extra_piece = 300;

// pawns
const int16_t uo_score_center_P = 20;
const int16_t uo_score_doubled_P = -20;
const int16_t uo_score_blocked_P = -20;

const int16_t uo_score_passed_pawn = 20;
const int16_t uo_score_passed_pawn_on_fifth = 30;
const int16_t uo_score_passed_pawn_on_sixth = 70;
const int16_t uo_score_passed_pawn_on_seventh = 120;

// piece development
const int16_t uo_score_undeveloped_piece = -15;

const int16_t uo_score_rook_on_semiopen_file = 15;
const int16_t uo_score_rook_on_open_file = 20;
const int16_t uo_score_connected_rooks = 20;

const int16_t uo_score_defended_by_pawn = 20;
const int16_t uo_score_attacked_by_pawn = 10;

const int16_t uo_score_knight_on_outpost = 30;
const int16_t uo_score_unattackable_by_pawn = 20;
const int16_t uo_score_knight_in_the_corner = -150;

// attacks near king
const int16_t uo_score_attacks_near_K = 30;
const int16_t uo_score_attacker_to_K = 30;

// king safety and castling
const int16_t uo_score_casting_right = 10;
const int16_t uo_score_king_in_the_center = -40;
const int16_t uo_score_castled_king = 40;
const int16_t uo_score_king_cover_pawn = 30;
const int16_t uo_score_king_next_to_open_file = -90;

int16_t uo_position_evaluate(const uo_position *position)
{
  int16_t score = uo_score_tempo;

  // opening & middle game
  int32_t score_mg = 0;

  // endgame
  int32_t score_eg = 0;

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
  uo_square attacks_own_K = uo_bitboard_attacks_K(square_own_K);
  uo_bitboard next_to_own_K = attacks_own_K | own_K;
  uo_square square_enemy_K = uo_tzcnt(enemy_K);
  uo_bitboard attacks_enemy_K = uo_bitboard_attacks_K(square_enemy_K);
  uo_bitboard next_to_enemy_K = attacks_enemy_K | enemy_K;

  uo_bitboard pushes_own_P = uo_bitboard_single_push_P(own_P, empty) | uo_bitboard_double_push_P(own_P, empty);
  uo_bitboard pushes_enemy_P = uo_bitboard_single_push_enemy_P(enemy_P, empty) | uo_bitboard_double_push_enemy_P(enemy_P, empty);

  uo_bitboard attacks_left_own_P = uo_bitboard_attacks_left_P(own_P);
  uo_bitboard attacks_right_own_P = uo_bitboard_attacks_right_P(own_P);
  uo_bitboard attacks_own_P = attacks_left_own_P | attacks_right_own_P;
  uo_bitboard attacks_left_enemy_P = uo_bitboard_attacks_left_P(enemy_P);
  uo_bitboard attacks_right_enemy_P = uo_bitboard_attacks_right_P(enemy_P);
  uo_bitboard attacks_enemy_P = attacks_left_enemy_P | attacks_right_enemy_P;

  uo_bitboard potential_attacks_own_P = attacks_own_P | uo_bitboard_attacks_left_P(pushes_own_P) | uo_bitboard_attacks_right_P(pushes_own_P);
  uo_bitboard potential_attacks_enemy_P = attacks_enemy_P | uo_bitboard_attacks_left_enemy_P(pushes_enemy_P) | uo_bitboard_attacks_right_enemy_P(pushes_enemy_P);

  // pawns

  score += uo_score_P * (int16_t)uo_popcnt(own_P);
  score += uo_score_mobility_P * (int16_t)uo_popcnt(pushes_own_P);
  score += uo_score_mobility_P * (int16_t)uo_popcnt(attacks_left_own_P & mask_enemy);
  score += uo_score_mobility_P * (int16_t)uo_popcnt(attacks_right_own_P & mask_enemy);
  score -= uo_score_P * (int16_t)uo_popcnt(enemy_P);
  score -= uo_score_mobility_P * (int16_t)uo_popcnt(pushes_enemy_P);
  score -= uo_score_mobility_P * (int16_t)uo_popcnt(attacks_left_enemy_P & mask_own);
  score -= uo_score_mobility_P * (int16_t)uo_popcnt(attacks_right_enemy_P & mask_own);

  score += uo_score_space_P * (int16_t)uo_popcnt(attacks_left_own_P & uo_bitboard_half_enemy);
  score += uo_score_space_P * (int16_t)uo_popcnt(attacks_right_own_P & uo_bitboard_half_enemy);
  score -= uo_score_space_P * (int16_t)uo_popcnt(attacks_left_enemy_P & uo_bitboard_half_own);
  score -= uo_score_space_P * (int16_t)uo_popcnt(attacks_right_enemy_P & uo_bitboard_half_own);

  uo_bitboard attacks_lower_value_own = attacks_own_P;
  uo_bitboard attacks_lower_value_enemy = attacks_enemy_P;

  uo_bitboard temp;

  // knights
  temp = own_N;
  score += uo_score_N_square_attackable_by_P * (int16_t)uo_popcnt(own_N & potential_attacks_enemy_P);
  while (temp)
  {
    uo_square square_own_N = uo_bitboard_next_square(&temp);
    uo_bitboard attacks_own_N = uo_bitboard_attacks_N(square_own_N);
    attacks_lower_value_own |= attacks_own_N;
    uo_bitboard mobility_own_N = uo_andn(mask_own | attacks_lower_value_enemy, attacks_own_N);
    score += uo_score_N + uo_score_extra_piece;
    score += uo_score_mobility_N * (int16_t)uo_popcnt(mobility_own_N);
    score += uo_score_space_N * (int16_t)uo_popcnt(attacks_own_N & uo_bitboard_half_enemy);

    if (attacks_own_N & next_to_enemy_K) score_eg += uo_score_attacker_to_K;
  }
  temp = enemy_N;
  score -= uo_score_N_square_attackable_by_P * (int16_t)uo_popcnt(enemy_N & potential_attacks_enemy_P);
  while (temp)
  {
    uo_square square_enemy_N = uo_bitboard_next_square(&temp);
    uo_bitboard attacks_enemy_N = uo_bitboard_attacks_N(square_enemy_N);
    attacks_lower_value_enemy |= attacks_enemy_N;
    uo_bitboard mobility_enemy_N = uo_andn(mask_enemy | attacks_lower_value_own, attacks_enemy_N);
    score -= uo_score_N + uo_score_extra_piece;
    score -= uo_score_mobility_N * (int16_t)uo_popcnt(mobility_enemy_N);
    score -= uo_score_space_N * (int16_t)uo_popcnt(attacks_enemy_N & uo_bitboard_half_own);

    if (attacks_enemy_N & next_to_own_K) score_eg -= uo_score_attacker_to_K;
  }

  // bishops
  temp = own_B;
  score += uo_score_B_square_attackable_by_P * (int16_t)uo_popcnt(own_B & potential_attacks_enemy_P);
  while (temp)
  {
    uo_square square_own_B = uo_bitboard_next_square(&temp);
    uo_bitboard attacks_own_B = uo_bitboard_attacks_B(square_own_B, occupied);
    attacks_lower_value_own |= attacks_own_B;
    uo_bitboard mobility_own_B = uo_andn(mask_own | attacks_lower_value_enemy, attacks_own_B);
    score += uo_score_B + uo_score_extra_piece;
    score += uo_score_mobility_B * (int16_t)uo_popcnt(mobility_own_B);
    score += uo_score_space_B * (int16_t)uo_popcnt(attacks_own_B & uo_bitboard_half_enemy);

    if (attacks_own_B & next_to_enemy_K) score_eg += uo_score_attacker_to_K;
  }
  temp = enemy_B;
  score -= uo_score_B_square_attackable_by_P * (int16_t)uo_popcnt(enemy_B & potential_attacks_enemy_P);
  while (temp)
  {
    uo_square square_enemy_B = uo_bitboard_next_square(&temp);
    uo_bitboard attacks_enemy_B = uo_bitboard_attacks_B(square_enemy_B, occupied);
    attacks_lower_value_enemy |= attacks_enemy_B;
    uo_bitboard mobility_enemy_B = uo_andn(mask_enemy | attacks_lower_value_own, attacks_enemy_B);
    score -= uo_score_B + uo_score_extra_piece;
    score -= uo_score_mobility_B * (int16_t)uo_popcnt(mobility_enemy_B);
    score -= uo_score_space_B * (int16_t)uo_popcnt(attacks_enemy_B & uo_bitboard_half_own);

    if (attacks_enemy_B & next_to_own_K) score_eg -= uo_score_attacker_to_K;
  }

  // rooks
  temp = own_R;
  score += uo_score_R_square_attackable_by_P * (int16_t)uo_popcnt(own_R & potential_attacks_enemy_P);
  while (temp)
  {
    uo_square square_own_R = uo_bitboard_next_square(&temp);
    uo_bitboard attacks_own_R = uo_bitboard_attacks_R(square_own_R, occupied);
    attacks_lower_value_own |= attacks_own_R;
    uo_bitboard mobility_own_R = uo_andn(mask_own | attacks_lower_value_enemy, attacks_own_R);
    score += uo_score_R + uo_score_extra_piece;
    score += uo_score_mobility_R * (int16_t)uo_popcnt(mobility_own_R);
    score += uo_score_space_R * (int16_t)uo_popcnt(attacks_own_R & uo_bitboard_half_enemy);

    // connected rooks
    if (attacks_own_R & own_R) score += uo_score_connected_rooks / 2;

    if (attacks_own_R & next_to_enemy_K) score_eg += uo_score_attacker_to_K;
  }
  temp = enemy_R;
  score -= uo_score_R_square_attackable_by_P * (int16_t)uo_popcnt(enemy_R & potential_attacks_enemy_P);
  while (temp)
  {
    uo_square square_enemy_R = uo_bitboard_next_square(&temp);
    uo_bitboard attacks_enemy_R = uo_bitboard_attacks_R(square_enemy_R, occupied);
    attacks_lower_value_enemy |= attacks_enemy_R;
    uo_bitboard mobility_enemy_R = uo_andn(mask_enemy | attacks_lower_value_own, attacks_enemy_R);
    score -= uo_score_R + uo_score_extra_piece;
    score -= uo_score_mobility_R * (int16_t)uo_popcnt(mobility_enemy_R);
    score -= uo_score_space_R * (int16_t)uo_popcnt(attacks_enemy_R & uo_bitboard_half_own);

    // connected rooks
    if (attacks_enemy_R & enemy_R) score -= uo_score_connected_rooks / 2;

    if (attacks_enemy_R & next_to_own_K) score_eg -= uo_score_attacker_to_K;
  }

  // queens
  temp = own_Q;
  score += uo_score_Q_square_attackable_by_P * (int16_t)uo_popcnt(own_Q & potential_attacks_enemy_P);
  while (temp)
  {
    uo_square square_own_Q = uo_bitboard_next_square(&temp);
    uo_bitboard attacks_own_Q = uo_bitboard_attacks_Q(square_own_Q, occupied);
    attacks_lower_value_own |= attacks_own_Q;
    uo_bitboard mobility_own_Q = uo_andn(mask_own | attacks_lower_value_enemy, attacks_own_Q);
    score += uo_score_Q + uo_score_extra_piece;
    score += uo_score_mobility_Q * (int16_t)uo_popcnt(mobility_own_Q);
    score += uo_score_space_Q * (int16_t)uo_popcnt(attacks_own_Q & uo_bitboard_half_enemy);

    if (attacks_own_Q & next_to_enemy_K) score_eg += uo_score_attacker_to_K;
  }
  temp = enemy_Q;
  score -= uo_score_Q_square_attackable_by_P * (int16_t)uo_popcnt(enemy_Q & potential_attacks_enemy_P);
  while (temp)
  {
    uo_square square_enemy_Q = uo_bitboard_next_square(&temp);
    uo_bitboard attacks_enemy_Q = uo_bitboard_attacks_Q(square_enemy_Q, occupied);
    attacks_lower_value_enemy |= attacks_enemy_Q;
    uo_bitboard mobility_enemy_Q = uo_andn(mask_enemy | attacks_lower_value_own, attacks_enemy_Q);
    score -= uo_score_Q + uo_score_extra_piece;
    score -= uo_score_mobility_Q * (int16_t)uo_popcnt(mobility_enemy_Q);
    score -= uo_score_space_Q * (int16_t)uo_popcnt(attacks_enemy_Q & uo_bitboard_half_own);

    if (attacks_enemy_Q & next_to_own_K) score_eg -= uo_score_attacker_to_K;
  }

  // king safety

  score += uo_score_mobility_K * (int16_t)uo_popcnt(uo_andn(attacks_enemy_K | attacks_lower_value_enemy, attacks_own_K));
  score += uo_score_K_square_attackable_by_P * (int16_t)uo_popcnt(own_K & potential_attacks_enemy_P);
  score -= uo_score_mobility_K * (int16_t)uo_popcnt(uo_andn(attacks_own_K | attacks_lower_value_own, attacks_enemy_K));
  score -= uo_score_K_square_attackable_by_P * (int16_t)uo_popcnt(enemy_K & potential_attacks_own_P);

  score += uo_score_king_cover_pawn * (int32_t)uo_popcnt(attacks_own_K & own_P);
  score -= uo_score_king_cover_pawn * (int32_t)uo_popcnt(attacks_enemy_K & enemy_P);

  // center pawns
  score_mg += uo_score_center_P * ((int32_t)uo_popcnt(uo_bitboard_center & own_P) - (int32_t)uo_popcnt(uo_bitboard_center & enemy_P));

  // doupled pawns
  score += uo_score_doubled_P * (((int16_t)uo_popcnt(own_P) - (int16_t)uo_popcnt(uo_bitboard_files(own_P) & uo_bitboard_rank_first))
    - ((int16_t)uo_popcnt(enemy_P) - (int16_t)uo_popcnt(uo_bitboard_files(enemy_P) & uo_bitboard_rank_first)));

  // passed pawns
  uo_bitboard passed_own_P = uo_bitboard_passed_P(own_P, enemy_P);
  if (passed_own_P)
  {
    score_eg += uo_score_passed_pawn * (int32_t)uo_popcnt(passed_own_P);
    score_eg += uo_score_passed_pawn_on_fifth * (int32_t)uo_popcnt(passed_own_P & uo_bitboard_rank_fifth);
    score_eg += uo_score_passed_pawn_on_sixth * (int32_t)uo_popcnt(passed_own_P & uo_bitboard_rank_sixth);
    score_eg += uo_score_passed_pawn_on_seventh * (int32_t)uo_popcnt(passed_own_P & uo_bitboard_rank_seventh);
  }

  // passed pawns
  uo_bitboard passed_enemy_P = uo_bitboard_passed_enemy_P(enemy_P, own_P);
  if (passed_enemy_P)
  {
    score_eg -= uo_score_passed_pawn * (int32_t)uo_popcnt(passed_enemy_P);
    score_eg -= uo_score_passed_pawn_on_fifth * (int32_t)uo_popcnt(passed_enemy_P & uo_bitboard_rank_fourth);
    score_eg -= uo_score_passed_pawn_on_sixth * (int32_t)uo_popcnt(passed_enemy_P & uo_bitboard_rank_third);
    score_eg -= uo_score_passed_pawn_on_seventh * (int32_t)uo_popcnt(passed_enemy_P & uo_bitboard_rank_second);
  }

  // first rank piece count (development)
  score_mg += uo_score_undeveloped_piece * ((int32_t)uo_popcnt((own_N | own_B) & uo_bitboard_rank_first) - (int32_t)uo_popcnt((enemy_N | enemy_B) & uo_bitboard_rank_last));

  // castling rights
  score_mg += uo_score_casting_right * (uo_position_flags_castling_OO(position->flags) + uo_position_flags_castling_OOO(position->flags)
    - uo_position_flags_castling_enemy_OO(position->flags) - uo_position_flags_castling_enemy_OOO(position->flags));

  // king safety
  score_mg += (uo_score_king_in_the_center * (0 != (own_K & (uo_bitboard_file[2] | uo_bitboard_file[3] | uo_bitboard_file[4] | uo_bitboard_file[5]))))
    - (uo_score_king_in_the_center * (0 != (enemy_K & (uo_bitboard_file[2] | uo_bitboard_file[3] | uo_bitboard_file[4] | uo_bitboard_file[5]))));

  score_mg += (uo_score_castled_king * (square_own_K == uo_square__g1))
    - (uo_score_castled_king * (square_enemy_K == uo_square__g8));

  score_mg += (uo_score_castled_king * (square_own_K == uo_square__b1))
    - (uo_score_castled_king * (square_enemy_K == uo_square__b8));

  int32_t material_percentage = uo_position_material_percentage(position);

  score += score_mg * material_percentage / 100;
  score += score_eg * (100 - material_percentage) / 100;

  return score;
}

int16_t uo_position_evaluate2(const uo_position *position)
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
  uo_square attacks_own_K = uo_bitboard_attacks_K(square_own_K);
  uo_bitboard next_to_own_K = attacks_own_K | own_K;
  uo_square square_enemy_K = uo_tzcnt(enemy_K);
  uo_bitboard attacks_enemy_K = uo_bitboard_attacks_K(square_enemy_K);
  uo_bitboard next_to_enemy_K = attacks_enemy_K | enemy_K;

  uo_bitboard pushes_own_P = uo_bitboard_single_push_P(own_P, empty) | uo_bitboard_double_push_P(own_P, empty);
  uo_bitboard pushes_enemy_P = uo_bitboard_single_push_enemy_P(enemy_P, empty) | uo_bitboard_double_push_enemy_P(enemy_P, empty);

  uo_bitboard attacks_own_NBRQ = uo_position_attacks_own_NBRQ(position);
  uo_bitboard attacks_enemy_NBRQ = uo_position_attacks_enemy_NBRQ(position);

  uo_bitboard attacks_own_P = uo_bitboard_attacks_left_P(own_P) | uo_bitboard_attacks_right_P(own_P);
  uo_bitboard attacks_enemy_P = uo_bitboard_attacks_left_enemy_P(enemy_P) | uo_bitboard_attacks_right_enemy_P(enemy_P);

  uo_bitboard attacks_own = attacks_own_K | attacks_own_NBRQ | attacks_own_P;
  uo_bitboard attacks_enemy = attacks_enemy_K | attacks_enemy_NBRQ | attacks_enemy_P;

  uo_bitboard mobility_own = uo_andn(mask_own, attacks_own_NBRQ | (attacks_own_P & mask_enemy) | pushes_own_P | uo_andn(attacks_enemy, attacks_own_K));
  uo_bitboard mobility_enemy = uo_andn(mask_enemy, attacks_enemy_NBRQ | (attacks_enemy_P & mask_enemy) | pushes_enemy_P | uo_andn(attacks_own, attacks_enemy_K));

  uo_bitboard potential_attacks_own_P = attacks_own_P | uo_bitboard_attacks_left_P(pushes_own_P) | uo_bitboard_attacks_right_P(pushes_own_P);
  uo_bitboard potential_attacks_enemy_P = attacks_enemy_P | uo_bitboard_attacks_left_enemy_P(pushes_enemy_P) | uo_bitboard_attacks_right_enemy_P(pushes_enemy_P);

  uo_bitboard developed_own_N = own_N & uo_bitboard_mask_developed_N;
  uo_bitboard developed_enemy_N = enemy_N & uo_bitboard_mask_developed_N;
  uo_bitboard dangerous_own_N = own_N & uo_bitboard_mask_dangerous_own_N;
  uo_bitboard dangerous_enemy_N = enemy_N & uo_bitboard_mask_dangerous_enemy_N;
  uo_bitboard developed_own_B = own_B & uo_bitboard_mask_developed_own_B;
  uo_bitboard developed_enemy_B = enemy_B & uo_bitboard_mask_developed_enemy_B;

  uo_bitboard temp;

  // material
  score += position->material;
  score -= 2 * uo_score_P * (int16_t)uo_popcnt(enemy_P);
  score -= 2 * uo_score_N * (int16_t)uo_popcnt(enemy_N);
  score -= 2 * uo_score_B * (int16_t)uo_popcnt(enemy_B);
  score -= 2 * uo_score_R * (int16_t)uo_popcnt(enemy_R);
  score -= 2 * uo_score_Q * (int16_t)uo_popcnt(enemy_Q);

  score += uo_score_extra_piece * ((int16_t)uo_popcnt(uo_andn(own_P, mask_own)) - (int16_t)uo_popcnt(uo_andn(enemy_P, mask_enemy)));

  // queen aiming opponent king
  temp = own_Q;
  while (temp)
  {
    uo_square square_own_Q = uo_bitboard_next_square(&temp);
    if (uo_bitboard_attacks_Q(square_own_Q, occupied) & next_to_enemy_K) score += uo_score_attacker_to_K;
  }
  temp = enemy_Q;
  while (temp)
  {
    uo_square square_enemy_Q = uo_bitboard_next_square(&temp);
    if (uo_bitboard_attacks_Q(square_enemy_Q, occupied) & next_to_own_K) score -= uo_score_attacker_to_K;
  }

  // rook aiming opponent king
  temp = own_R;
  while (temp)
  {
    uo_square square_own_R = uo_bitboard_next_square(&temp);
    if (uo_bitboard_attacks_R(square_own_R, occupied) & next_to_enemy_K) score += uo_score_attacker_to_K;
  }
  temp = enemy_R;
  while (temp)
  {
    uo_square square_enemy_R = uo_bitboard_next_square(&temp);
    if (uo_bitboard_attacks_R(square_enemy_R, occupied) & next_to_own_K) score -= uo_score_attacker_to_K;
  }

  // bishop aiming opponent king
  temp = own_B;
  while (temp)
  {
    uo_square square_own_B = uo_bitboard_next_square(&temp);
    if (uo_bitboard_attacks_B(square_own_B, occupied) & next_to_enemy_K) score += uo_score_attacker_to_K;
  }
  temp = enemy_B;
  while (temp)
  {
    uo_square square_enemy_B = uo_bitboard_next_square(&temp);
    if (uo_bitboard_attacks_B(square_enemy_B, occupied) & next_to_own_K) score -= uo_score_attacker_to_K;
  }

  // knight very close to opponent king
  temp = own_N;
  while (temp)
  {
    uo_square square_own_N = uo_bitboard_next_square(&temp);
    if (uo_bitboard_attacks_N(square_own_N) & next_to_enemy_K) score += uo_score_attacker_to_K;
  }
  temp = enemy_N;
  while (temp)
  {
    uo_square square_enemy_N = uo_bitboard_next_square(&temp);
    if (uo_bitboard_attacks_N(square_enemy_N) & next_to_own_K) score -= uo_score_attacker_to_K;
  }

  score += uo_score_attacks_near_K * (int16_t)uo_popcnt(attacks_own & next_to_enemy_K) - (int16_t)uo_popcnt(attacks_enemy & next_to_own_K);

  // doupled pawns
  score += uo_score_doubled_P * (((int16_t)uo_popcnt(own_P) - (int16_t)uo_popcnt(uo_bitboard_files(own_P) & uo_bitboard_rank_first))
    - ((int16_t)uo_popcnt(enemy_P) - (int16_t)uo_popcnt(uo_bitboard_files(enemy_P) & uo_bitboard_rank_first)));

  // blocked pawns 
  score += uo_score_blocked_P * ((int16_t)uo_popcnt((own_P << 8) & mask_own) - (int16_t)uo_popcnt((enemy_P >> 8) & mask_enemy));


  //// opening & middle game
  int32_t score_mg = 0;

  // mobility
  score_mg += uo_score_square_access * ((int32_t)uo_popcnt(mobility_own) - (int32_t)uo_popcnt(mobility_enemy));

  // center pawns
  score_mg += uo_score_center_P * ((int32_t)uo_popcnt(uo_bitboard_center & own_P) - (int32_t)uo_popcnt(uo_bitboard_center & enemy_P));

  // first rank piece count (development)
  score_mg += uo_score_undeveloped_piece * ((int32_t)uo_popcnt((own_N | own_B | own_Q) & uo_bitboard_rank_first) - (int32_t)uo_popcnt((enemy_N | enemy_B | enemy_Q) & uo_bitboard_rank_last));

  // castling rights
  score_mg += (uo_score_casting_right * (uo_position_flags_castling_OO(position->flags) + uo_position_flags_castling_OOO(position->flags)))
    - (uo_score_casting_right * (uo_position_flags_castling_enemy_OO(position->flags) + uo_position_flags_castling_enemy_OOO(position->flags)));

  // king safety
  score_mg += (uo_score_king_in_the_center * (0 != (own_K & (uo_bitboard_file[2] | uo_bitboard_file[3] | uo_bitboard_file[4] | uo_bitboard_file[5]))))
    - (uo_score_king_in_the_center * (0 != (enemy_K & (uo_bitboard_file[2] | uo_bitboard_file[3] | uo_bitboard_file[4] | uo_bitboard_file[5]))));

  score_mg += (uo_score_castled_king * (square_own_K == uo_square__g1))
    - (uo_score_castled_king * (square_enemy_K == uo_square__g8));

  score_mg += (uo_score_castled_king * (square_own_K == uo_square__b1))
    - (uo_score_castled_king * (square_enemy_K == uo_square__b8));

  score_mg += (uo_score_king_cover_pawn * (int32_t)uo_popcnt(uo_bitboard_moves_K(square_own_K, ~own_P, own_P)))
    - (uo_score_king_cover_pawn * (int32_t)uo_popcnt(uo_bitboard_moves_K(square_enemy_K, ~enemy_P, enemy_P)));

  //score_mg += (uo_score_king_next_to_open_file * (int32_t)uo_popcnt((uo_square_bitboard_adjecent_files[square_own_K] | uo_square_bitboard_file[square_own_K]) & uo_bitboard_files(own_P) & uo_bitboard_rank_first))
  //  - (uo_score_king_next_to_open_file * (int32_t)uo_popcnt((uo_square_bitboard_adjecent_files[square_enemy_K] | uo_square_bitboard_file[square_enemy_K]) & uo_bitboard_files(enemy_P) & uo_bitboard_rank_first));

  // rooks on semi-open files
  score_mg += uo_score_rook_on_semiopen_file * ((int32_t)uo_popcnt(uo_andn(uo_bitboard_files(own_P), own_R)) - (int32_t)uo_popcnt(uo_andn(uo_bitboard_files(enemy_P), enemy_R)));

  // rooks on open files
  score_mg += uo_score_rook_on_open_file * ((int32_t)uo_popcnt(uo_andn(uo_bitboard_files(position->P), own_R)) - (int32_t)uo_popcnt(uo_andn(uo_bitboard_files(position->P), enemy_R)));

  // pieces defended by pawns
  score_mg += uo_score_defended_by_pawn * ((int32_t)uo_popcnt(attacks_own_P & mask_own) - (int32_t)uo_popcnt(attacks_enemy_P & mask_enemy));

  // pieces attacked by pawns
  score_mg += uo_score_attacked_by_pawn * ((int32_t)uo_popcnt(attacks_own_P & mask_enemy) - (int32_t)uo_popcnt(attacks_enemy_P & mask_own));

  // developed pieces which cannot be attacked by pushing pawns
  score_mg += uo_score_unattackable_by_pawn * ((int32_t)uo_popcnt(uo_andn(potential_attacks_enemy_P, developed_own_N | developed_own_B)) - (int32_t)uo_popcnt(uo_andn(potential_attacks_own_P, developed_enemy_N | developed_enemy_B)));

  // knights on outpost
  score_mg += uo_score_knight_on_outpost * ((int32_t)uo_popcnt(uo_andn(potential_attacks_enemy_P, dangerous_own_N)) - (int32_t)uo_popcnt(uo_andn(potential_attacks_own_P, dangerous_enemy_N)));

  // knight in the corner
  score_mg += uo_score_knight_in_the_corner * ((int32_t)uo_popcnt(uo_bitboard_corners & own_N) - (int32_t)uo_popcnt(uo_bitboard_corners & enemy_N));


  //// endgame
  int32_t score_eg = 0;

  // passed pawns
  uo_bitboard passed_own_P = uo_bitboard_passed_P(own_P, enemy_P);
  if (passed_own_P)
  {
    score_eg += uo_score_passed_pawn * (int32_t)uo_popcnt(passed_own_P);
    score_eg += uo_score_passed_pawn_on_fifth * (int32_t)uo_popcnt(passed_own_P & uo_bitboard_rank_fifth);
    score_eg += uo_score_passed_pawn_on_sixth * (int32_t)uo_popcnt(passed_own_P & uo_bitboard_rank_sixth);
    score_eg += uo_score_passed_pawn_on_seventh * (int32_t)uo_popcnt(passed_own_P & uo_bitboard_rank_seventh);
  }

  // passed pawns
  uo_bitboard passed_enemy_P = uo_bitboard_passed_enemy_P(enemy_P, own_P);
  if (passed_enemy_P)
  {
    score_eg -= uo_score_passed_pawn * (int32_t)uo_popcnt(passed_enemy_P);
    score_eg -= uo_score_passed_pawn_on_fifth * (int32_t)uo_popcnt(passed_enemy_P & uo_bitboard_rank_fourth);
    score_eg -= uo_score_passed_pawn_on_sixth * (int32_t)uo_popcnt(passed_enemy_P & uo_bitboard_rank_third);
    score_eg -= uo_score_passed_pawn_on_seventh * (int32_t)uo_popcnt(passed_enemy_P & uo_bitboard_rank_second);
  }

  int32_t material_percentage = uo_position_material_percentage(position);

  score += score_mg * material_percentage / 100;
  score += score_eg * (100 - material_percentage) / 100;

  return score;
}
