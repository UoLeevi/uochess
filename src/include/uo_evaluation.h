#ifndef UO_EVALUATION_H
#define UO_EVALUATION_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_position.h"
#include "uo_piece.h"
#include "uo_util.h"
#include "uo_def.h"

  static inline bool uo_score_is_checkmate(int16_t score)
  {
    return score > uo_score_mate_in_threshold || score < -uo_score_mate_in_threshold;
  }

  static inline int16_t uo_score_adjust_for_mate_to_ttable(const uo_position *position, int16_t score)
  {
    if (score > uo_score_mate_in_threshold)
    {
      return score + position->ply;
    }
    else if (score < -uo_score_mate_in_threshold)
    {
      return score - position->ply;
    }

    return score;
  }

  static inline int16_t uo_score_adjust_for_mate_from_ttable(const uo_position *position, int16_t score)
  {
    if (score > uo_score_mate_in_threshold)
    {
      return score - position->ply;
    }
    else if (score < -uo_score_mate_in_threshold)
    {
      return score + position->ply;
    }

    return score;
  }


  // side to move
#define uo_score_tempo 25

  // mobility
#define uo_score_square_access 5

#define uo_score_mobility_P 5
#define uo_score_mobility_N 1
#define uo_score_mobility_B 1
#define uo_score_mobility_R 1
#define uo_score_mobility_Q 1
#define uo_score_mobility_K 1

#define uo_score_N_square_attackable_by_P -10
#define uo_score_B_square_attackable_by_P -5
#define uo_score_R_square_attackable_by_P -15
#define uo_score_Q_square_attackable_by_P -30
#define uo_score_K_square_attackable_by_P -70

  // space
#define uo_score_space_P 5
#define uo_score_space_N 2
#define uo_score_space_B 2
#define uo_score_space_R 4
#define uo_score_space_Q 1

#define uo_score_extra_piece 80

  // pawns
#define uo_score_doubled_P -15
#define uo_score_blocked_P -20

#define uo_score_passed_pawn 30

  // piece development
#define uo_score_rook_on_semiopen_file 15
#define uo_score_rook_on_open_file 20
#define uo_score_connected_rooks 20

#define uo_score_defended_by_pawn 10
#define uo_score_attacked_by_pawn 5

#define uo_score_knight_on_outpost 30
#define uo_score_unattackable_by_pawn 20
#define uo_score_knight_in_the_corner -80

  // attacks near king
#define uo_score_attacks_near_K 15
#define uo_score_attacker_to_K 20

  // king safety and castling
#define uo_score_casting_right 10
#define uo_score_king_in_the_center -40
#define uo_score_castled_king 50
#define uo_score_king_cover_pawn 50
#define uo_score_king_next_to_open_file -90

  extern const int16_t mg_table_P[64];
  extern const int16_t eg_table_P[64];
  extern const int16_t mg_table_N[64];
  extern const int16_t eg_table_N[64];
  extern const int16_t mg_table_B[64];
  extern const int16_t eg_table_B[64];
  extern const int16_t mg_table_R[64];
  extern const int16_t eg_table_R[64];
  extern const int16_t mg_table_Q[64];
  extern const int16_t eg_table_Q[64];
  extern const int16_t mg_table_K[64];
  extern const int16_t eg_table_K[64];

  static inline int16_t uo_position_evaluate(const uo_position *position)
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
    uo_bitboard attacks_own_K = uo_bitboard_attacks_K(square_own_K);
    uo_bitboard next_to_own_K = attacks_own_K | own_K;
    uo_square square_enemy_K = uo_tzcnt(enemy_K);
    uo_bitboard attacks_enemy_K = uo_bitboard_attacks_K(square_enemy_K);
    uo_bitboard next_to_enemy_K = attacks_enemy_K | enemy_K;

    uo_bitboard pushes_own_P = uo_bitboard_single_push_P(own_P, empty) | uo_bitboard_double_push_P(own_P, empty);
    uo_bitboard pushes_enemy_P = uo_bitboard_single_push_enemy_P(enemy_P, empty) | uo_bitboard_double_push_enemy_P(enemy_P, empty);

    uo_bitboard attacks_left_own_P = uo_bitboard_attacks_left_P(own_P);
    uo_bitboard attacks_right_own_P = uo_bitboard_attacks_right_P(own_P);
    uo_bitboard attacks_own_P = attacks_left_own_P | attacks_right_own_P;
    uo_bitboard attacks_left_enemy_P = uo_bitboard_attacks_left_enemy_P(enemy_P);
    uo_bitboard attacks_right_enemy_P = uo_bitboard_attacks_right_enemy_P(enemy_P);
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

      score_mg += mg_table_N[square_own_N];
      score_eg += eg_table_N[square_own_N];

      if (attacks_own_N & next_to_enemy_K) score_eg += uo_score_attacker_to_K;
    }
    temp = enemy_N;
    score -= uo_score_N_square_attackable_by_P * (int16_t)uo_popcnt(enemy_N & potential_attacks_own_P);
    while (temp)
    {
      uo_square square_enemy_N = uo_bitboard_next_square(&temp);
      uo_bitboard attacks_enemy_N = uo_bitboard_attacks_N(square_enemy_N);
      attacks_lower_value_enemy |= attacks_enemy_N;
      uo_bitboard mobility_enemy_N = uo_andn(mask_enemy | attacks_lower_value_own, attacks_enemy_N);
      score -= uo_score_N + uo_score_extra_piece;
      score -= uo_score_mobility_N * (int16_t)uo_popcnt(mobility_enemy_N);
      score -= uo_score_space_N * (int16_t)uo_popcnt(attacks_enemy_N & uo_bitboard_half_own);

      score_mg -= mg_table_N[square_enemy_N ^ 56];
      score_eg -= eg_table_N[square_enemy_N ^ 56];

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

      score_mg += mg_table_B[square_own_B];
      score_eg += eg_table_B[square_own_B];

      if (attacks_own_B & next_to_enemy_K) score_eg += uo_score_attacker_to_K;
    }
    temp = enemy_B;
    score -= uo_score_B_square_attackable_by_P * (int16_t)uo_popcnt(enemy_B & potential_attacks_own_P);
    while (temp)
    {
      uo_square square_enemy_B = uo_bitboard_next_square(&temp);
      uo_bitboard attacks_enemy_B = uo_bitboard_attacks_B(square_enemy_B, occupied);
      attacks_lower_value_enemy |= attacks_enemy_B;
      uo_bitboard mobility_enemy_B = uo_andn(mask_enemy | attacks_lower_value_own, attacks_enemy_B);
      score -= uo_score_B + uo_score_extra_piece;
      score -= uo_score_mobility_B * (int16_t)uo_popcnt(mobility_enemy_B);
      score -= uo_score_space_B * (int16_t)uo_popcnt(attacks_enemy_B & uo_bitboard_half_own);

      score_mg -= mg_table_B[square_enemy_B ^ 56];
      score_eg -= eg_table_B[square_enemy_B ^ 56];

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

      score_mg += mg_table_R[square_own_R];
      score_eg += eg_table_R[square_own_R];

      // connected rooks
      if (attacks_own_R & own_R) score += uo_score_connected_rooks / 2;

      if (attacks_own_R & next_to_enemy_K) score_eg += uo_score_attacker_to_K;
    }
    temp = enemy_R;
    score -= uo_score_R_square_attackable_by_P * (int16_t)uo_popcnt(enemy_R & potential_attacks_own_P);
    while (temp)
    {
      uo_square square_enemy_R = uo_bitboard_next_square(&temp);
      uo_bitboard attacks_enemy_R = uo_bitboard_attacks_R(square_enemy_R, occupied);
      attacks_lower_value_enemy |= attacks_enemy_R;
      uo_bitboard mobility_enemy_R = uo_andn(mask_enemy | attacks_lower_value_own, attacks_enemy_R);
      score -= uo_score_R + uo_score_extra_piece;
      score -= uo_score_mobility_R * (int16_t)uo_popcnt(mobility_enemy_R);
      score -= uo_score_space_R * (int16_t)uo_popcnt(attacks_enemy_R & uo_bitboard_half_own);

      score_mg -= mg_table_R[square_enemy_R ^ 56];
      score_eg -= eg_table_R[square_enemy_R ^ 56];

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

      score_mg += mg_table_Q[square_own_Q];
      score_eg += eg_table_Q[square_own_Q];

      if (attacks_own_Q & next_to_enemy_K) score_eg += uo_score_attacker_to_K;
    }
    temp = enemy_Q;
    score -= uo_score_Q_square_attackable_by_P * (int16_t)uo_popcnt(enemy_Q & potential_attacks_own_P);
    while (temp)
    {
      uo_square square_enemy_Q = uo_bitboard_next_square(&temp);
      uo_bitboard attacks_enemy_Q = uo_bitboard_attacks_Q(square_enemy_Q, occupied);
      attacks_lower_value_enemy |= attacks_enemy_Q;
      uo_bitboard mobility_enemy_Q = uo_andn(mask_enemy | attacks_lower_value_own, attacks_enemy_Q);
      score -= uo_score_Q + uo_score_extra_piece;
      score -= uo_score_mobility_Q * (int16_t)uo_popcnt(mobility_enemy_Q);
      score -= uo_score_space_Q * (int16_t)uo_popcnt(attacks_enemy_Q & uo_bitboard_half_own);

      score_mg -= mg_table_Q[square_enemy_Q ^ 56];
      score_eg -= eg_table_Q[square_enemy_Q ^ 56];

      if (attacks_enemy_Q & next_to_own_K) score_eg -= uo_score_attacker_to_K;
    }

    // king

    score_mg += mg_table_K[square_own_K];
    score_eg += eg_table_K[square_own_K];

    score_mg -= mg_table_K[square_enemy_K ^ 56];
    score_eg -= eg_table_K[square_enemy_K ^ 56];

    score += uo_score_mobility_K * (int16_t)uo_popcnt(uo_andn(attacks_enemy_K | attacks_lower_value_enemy, attacks_own_K));
    score += uo_score_K_square_attackable_by_P * (int16_t)uo_popcnt(own_K & potential_attacks_enemy_P);
    score -= uo_score_mobility_K * (int16_t)uo_popcnt(uo_andn(attacks_own_K | attacks_lower_value_own, attacks_enemy_K));
    score -= uo_score_K_square_attackable_by_P * (int16_t)uo_popcnt(enemy_K & potential_attacks_own_P);

    score += uo_score_king_cover_pawn * (int32_t)uo_popcnt(attacks_own_K & own_P);
    score -= uo_score_king_cover_pawn * (int32_t)uo_popcnt(attacks_enemy_K & enemy_P);

    // doupled pawns
    score += uo_score_doubled_P * (((int16_t)uo_popcnt(own_P) - (int16_t)uo_popcnt(uo_bitboard_files(own_P) & uo_bitboard_rank_first))
      - ((int16_t)uo_popcnt(enemy_P) - (int16_t)uo_popcnt(uo_bitboard_files(enemy_P) & uo_bitboard_rank_first)));

    // passed pawns
    uo_bitboard passed_own_P = uo_bitboard_passed_P(own_P, enemy_P);
    if (passed_own_P)
    {
      score_eg += uo_score_passed_pawn * (int32_t)uo_popcnt(passed_own_P);
    }

    // passed pawns
    uo_bitboard passed_enemy_P = uo_bitboard_passed_enemy_P(enemy_P, own_P);
    if (passed_enemy_P)
    {
      score_eg -= uo_score_passed_pawn * (int32_t)uo_popcnt(passed_enemy_P);
    }

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

#ifdef __cplusplus
}
#endif

#endif
