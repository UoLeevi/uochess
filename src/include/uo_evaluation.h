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

#include <math.h>

  static inline bool uo_score_is_checkmate(int16_t score)
  {
    return score > uo_score_mate_in_threshold || score < -uo_score_mate_in_threshold;
  }

  static inline int16_t uo_score_adjust_to_ttable(const uo_position *position, int16_t score)
  {
    return score > uo_score_tb_win_threshold ? score + position->ply
      : score < -uo_score_tb_win_threshold ? score - position->ply
      : score;
  }

  static inline int16_t uo_score_adjust_from_ttable(const uo_position *position, int16_t score)
  {
    return score > uo_score_tb_win_threshold ? score - position->ply
      : score < -uo_score_tb_win_threshold ? score + position->ply
      : score;
  }

  // factor:          0,    1,    2,    3,    4,    5,    6,    7,    8,    9,   10
  // log(1 + factor): 0, 0.69, 1.10, 1.39, 1.61, 1.79, 1.96, 2.08, 2.20, 2.30, 2.40
  static inline int16_t uo_score_mul_ln(int16_t score, int factor)
  {
    return (float)score * logf(1.0f + (float)factor);
  }

  // side to move
#define uo_score_tempo 25

  // mobility
#define uo_score_mobility_P 5
#define uo_score_mobility_N 8
#define uo_score_mobility_B 6
#define uo_score_mobility_R 4
#define uo_score_mobility_Q 4
#define uo_score_mobility_K 3

#define uo_score_N_square_attackable_by_P -10
#define uo_score_B_square_attackable_by_P -5
#define uo_score_R_square_attackable_by_P -20
#define uo_score_Q_square_attackable_by_P -30
#define uo_score_K_square_attackable_by_P -70

#define uo_score_extra_piece 80

  // pawns
#define uo_score_doubled_P -15
#define uo_score_blocked_P -30

#define uo_score_passed_pawn 20
#define uo_score_passed_pawn_on_fifth 30
#define uo_score_passed_pawn_on_sixth 70
#define uo_score_passed_pawn_on_seventh 120

  // piece development
#define uo_score_rook_on_semiopen_file 15
#define uo_score_rook_on_open_file 20
#define uo_score_connected_rooks 20

#define uo_score_defended_by_pawn 10
#define uo_score_attacked_by_pawn 5

#define uo_score_knight_on_outpost 30
#define uo_score_unattackable_by_pawn 20

#define uo_score_rook_stuck_in_corner -40

  // attacks near king
#define uo_score_attacker_to_K 15
#define uo_score_defender_to_K 10

  // king safety and castling
#define uo_score_casting_right 20
#define uo_score_king_in_the_center -40
#define uo_score_castled_king 50
#define uo_score_king_cover_pawn 30
#define uo_score_king_next_to_open_file -90

  // piece square table weight
static inline int16_t uo_score_adjust_piece_square_table_score(int16_t score)
{
  return score / 2;
}

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

    int16_t castling_right_own_OO = uo_position_flags_castling_OO(position->flags);
    int16_t castling_right_own_OOO = uo_position_flags_castling_OOO(position->flags);
    int16_t castling_right_enemy_OO = uo_position_flags_castling_enemy_OO(position->flags);
    int16_t castling_right_enemy_OOO = uo_position_flags_castling_enemy_OOO(position->flags);

    int count_attacker_to_own_K = 0;
    int count_defender_of_own_K = 0;
    int count_attacker_to_enemy_K = 0;
    int count_defender_of_enemy_K = 0;

    // pawns

    score += uo_score_P * (int16_t)uo_popcnt(own_P);
    score -= uo_score_P * (int16_t)uo_popcnt(enemy_P);

    int mobility_own_P = uo_popcnt(pushes_own_P) +
      uo_popcnt(attacks_left_own_P & mask_enemy) +
      uo_popcnt(attacks_right_own_P & mask_enemy);
    score += uo_score_mul_ln(uo_score_mobility_P, mobility_own_P * mobility_own_P);

    int mobility_enemy_P = uo_popcnt(pushes_enemy_P) +
      uo_popcnt(attacks_left_enemy_P & mask_own) +
      uo_popcnt(attacks_right_enemy_P & mask_own);
    score -= uo_score_mul_ln(uo_score_mobility_P, mobility_enemy_P * mobility_enemy_P);


    // bitboards marking attacked squares by pieces of the same type
    uo_bitboard attacks_total_own_N = 0;
    uo_bitboard attacks_total_enemy_N = 0;
    uo_bitboard attacks_total_own_B = 0;
    uo_bitboard attacks_total_enemy_B = 0;
    uo_bitboard attacks_total_own_R = 0;
    uo_bitboard attacks_total_enemy_R = 0;
    uo_bitboard attacks_total_own_Q = 0;
    uo_bitboard attacks_total_enemy_Q = 0;

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
      attacks_total_own_N |= attacks_own_N;

      int mobility_own_N = uo_popcnt(uo_andn(mask_own | attacks_lower_value_enemy, attacks_own_N));
      score += uo_score_mul_ln(uo_score_mobility_N, mobility_own_N * mobility_own_N);

      score += uo_score_N + uo_score_extra_piece;

      score_mg += uo_score_adjust_piece_square_table_score(mg_table_N[square_own_N]);
      score_eg += uo_score_adjust_piece_square_table_score(eg_table_N[square_own_N]);

      if (attacks_own_N & next_to_enemy_K) ++count_attacker_to_enemy_K;
      if (attacks_own_N & next_to_own_K) ++count_defender_of_own_K;
    }
    temp = enemy_N;
    score -= uo_score_N_square_attackable_by_P * (int16_t)uo_popcnt(enemy_N & potential_attacks_own_P);
    while (temp)
    {
      uo_square square_enemy_N = uo_bitboard_next_square(&temp);
      uo_bitboard attacks_enemy_N = uo_bitboard_attacks_N(square_enemy_N);
      attacks_total_enemy_N |= attacks_enemy_N;

      int mobility_enemy_N = uo_popcnt(uo_andn(mask_enemy | attacks_lower_value_own, attacks_enemy_N));
      score -= uo_score_mul_ln(uo_score_mobility_N, mobility_enemy_N * mobility_enemy_N);

      score -= uo_score_N + uo_score_extra_piece;

      score_mg -= uo_score_adjust_piece_square_table_score(mg_table_N[square_enemy_N ^ 56]);
      score_eg -= uo_score_adjust_piece_square_table_score(eg_table_N[square_enemy_N ^ 56]);

      if (attacks_enemy_N & next_to_own_K) ++count_attacker_to_own_K;
      if (attacks_enemy_N & next_to_enemy_K) ++count_defender_of_enemy_K;
    }
    attacks_lower_value_own |= attacks_total_own_N;
    attacks_lower_value_enemy |= attacks_total_enemy_N;


    // bishops
    temp = own_B;
    score += uo_score_B_square_attackable_by_P * (int16_t)uo_popcnt(own_B & potential_attacks_enemy_P);
    while (temp)
    {
      uo_square square_own_B = uo_bitboard_next_square(&temp);
      uo_bitboard attacks_own_B = uo_bitboard_attacks_B(square_own_B, occupied);
      attacks_total_own_B |= attacks_own_B;

      int mobility_own_B = uo_popcnt(uo_andn(mask_own | attacks_lower_value_enemy, attacks_own_B));
      score += uo_score_mul_ln(uo_score_mobility_B, mobility_own_B * mobility_own_B);

      score += uo_score_B + uo_score_extra_piece;

      score_mg += uo_score_adjust_piece_square_table_score(mg_table_B[square_own_B]);
      score_eg += uo_score_adjust_piece_square_table_score(eg_table_B[square_own_B]);

      if (attacks_own_B & next_to_enemy_K) ++count_attacker_to_enemy_K;
      if (attacks_own_B & next_to_own_K) ++count_defender_of_own_K;
    }
    temp = enemy_B;
    score -= uo_score_B_square_attackable_by_P * (int16_t)uo_popcnt(enemy_B & potential_attacks_own_P);
    while (temp)
    {
      uo_square square_enemy_B = uo_bitboard_next_square(&temp);
      uo_bitboard attacks_enemy_B = uo_bitboard_attacks_B(square_enemy_B, occupied);
      attacks_total_enemy_B |= attacks_enemy_B;

      int mobility_enemy_B = uo_popcnt(uo_andn(mask_enemy | attacks_lower_value_own, attacks_enemy_B));
      score -= uo_score_mul_ln(uo_score_mobility_B, mobility_enemy_B * mobility_enemy_B);

      score -= uo_score_B + uo_score_extra_piece;

      score_mg -= uo_score_adjust_piece_square_table_score(mg_table_B[square_enemy_B ^ 56]);
      score_eg -= uo_score_adjust_piece_square_table_score(eg_table_B[square_enemy_B ^ 56]);

      if (attacks_enemy_B & next_to_own_K) ++count_attacker_to_own_K;
      if (attacks_enemy_B & next_to_enemy_K) ++count_defender_of_enemy_K;
    }
    attacks_lower_value_own |= attacks_total_own_B;
    attacks_lower_value_enemy |= attacks_total_enemy_B;

    // rooks
    temp = own_R;
    score += uo_score_R_square_attackable_by_P * (int16_t)uo_popcnt(own_R & potential_attacks_enemy_P);
    while (temp)
    {
      uo_square square_own_R = uo_bitboard_next_square(&temp);
      uo_bitboard attacks_own_R = uo_bitboard_attacks_R(square_own_R, occupied);
      attacks_total_own_R |= attacks_own_R;

      int mobility_own_R = uo_popcnt(uo_andn(mask_own | attacks_lower_value_enemy, attacks_own_R));
      score += uo_score_mul_ln(uo_score_mobility_R, mobility_own_R * mobility_own_R);

      score += uo_score_R + uo_score_extra_piece;

      score_mg += uo_score_adjust_piece_square_table_score(mg_table_R[square_own_R]);
      score_eg += uo_score_adjust_piece_square_table_score(eg_table_R[square_own_R]);

      // connected rooks
      if (attacks_own_R & own_R) score += uo_score_connected_rooks / 2;

      if (attacks_own_R & next_to_enemy_K) ++count_attacker_to_enemy_K;
      if (attacks_own_R & next_to_own_K) ++count_defender_of_own_K;
    }
    temp = enemy_R;
    score -= uo_score_R_square_attackable_by_P * (int16_t)uo_popcnt(enemy_R & potential_attacks_own_P);
    while (temp)
    {
      uo_square square_enemy_R = uo_bitboard_next_square(&temp);
      uo_bitboard attacks_enemy_R = uo_bitboard_attacks_R(square_enemy_R, occupied);
      attacks_total_enemy_R |= attacks_enemy_R;

      int mobility_enemy_R = uo_popcnt(uo_andn(mask_enemy | attacks_lower_value_own, attacks_enemy_R));
      score -= uo_score_mul_ln(uo_score_mobility_R, mobility_enemy_R * mobility_enemy_R);

      score -= uo_score_R + uo_score_extra_piece;

      score_mg -= uo_score_adjust_piece_square_table_score(mg_table_R[square_enemy_R ^ 56]);
      score_eg -= uo_score_adjust_piece_square_table_score(eg_table_R[square_enemy_R ^ 56]);

      // connected rooks
      if (attacks_enemy_R & enemy_R) score -= uo_score_connected_rooks / 2;

      if (attacks_enemy_R & next_to_own_K) ++count_attacker_to_own_K;
      if (attacks_enemy_R & next_to_enemy_K) ++count_defender_of_enemy_K;
    }
    attacks_lower_value_own |= attacks_total_own_R;
    attacks_lower_value_enemy |= attacks_total_enemy_R;

    // queens
    temp = own_Q;
    score += uo_score_Q_square_attackable_by_P * (int16_t)uo_popcnt(own_Q & potential_attacks_enemy_P);
    while (temp)
    {
      uo_square square_own_Q = uo_bitboard_next_square(&temp);
      uo_bitboard attacks_own_Q = uo_bitboard_attacks_Q(square_own_Q, occupied);
      attacks_total_own_Q |= attacks_own_Q;

      int mobility_own_Q = uo_popcnt(uo_andn(mask_own | attacks_lower_value_enemy, attacks_own_Q));
      score += uo_score_mul_ln(uo_score_mobility_Q, mobility_own_Q * mobility_own_Q);

      score += uo_score_Q + uo_score_extra_piece;

      score_mg += uo_score_adjust_piece_square_table_score(mg_table_Q[square_own_Q]);
      score_eg += uo_score_adjust_piece_square_table_score(eg_table_Q[square_own_Q]);

      if (attacks_own_Q & next_to_enemy_K) ++count_attacker_to_enemy_K;
      if (attacks_own_Q & next_to_own_K) ++count_defender_of_own_K;
    }
    temp = enemy_Q;
    score -= uo_score_Q_square_attackable_by_P * (int16_t)uo_popcnt(enemy_Q & potential_attacks_own_P);
    while (temp)
    {
      uo_square square_enemy_Q = uo_bitboard_next_square(&temp);
      uo_bitboard attacks_enemy_Q = uo_bitboard_attacks_Q(square_enemy_Q, occupied);
      attacks_total_enemy_Q |= attacks_enemy_Q;

      int mobility_enemy_Q = uo_popcnt(uo_andn(mask_enemy | attacks_lower_value_own, attacks_enemy_Q));
      score -= uo_score_mul_ln(uo_score_mobility_Q, mobility_enemy_Q * mobility_enemy_Q);

      score -= uo_score_Q + uo_score_extra_piece;

      score_mg -= uo_score_adjust_piece_square_table_score(mg_table_Q[square_enemy_Q ^ 56]);
      score_eg -= uo_score_adjust_piece_square_table_score(eg_table_Q[square_enemy_Q ^ 56]);

      if (attacks_enemy_Q & next_to_own_K) ++count_attacker_to_own_K;
      if (attacks_enemy_Q & next_to_enemy_K) ++count_defender_of_enemy_K;
    }
    attacks_lower_value_own |= attacks_total_own_Q;
    attacks_lower_value_enemy |= attacks_total_enemy_Q;

    // king

    score_mg += uo_score_adjust_piece_square_table_score(mg_table_K[square_own_K]);
    score_eg += uo_score_adjust_piece_square_table_score(eg_table_K[square_own_K]);

    score_mg -= uo_score_adjust_piece_square_table_score(mg_table_K[square_enemy_K ^ 56]);
    score_eg -= uo_score_adjust_piece_square_table_score(eg_table_K[square_enemy_K ^ 56]);

    score += uo_score_mobility_K * (int16_t)uo_popcnt(uo_andn(attacks_enemy_K | attacks_lower_value_enemy, attacks_own_K));
    score += uo_score_K_square_attackable_by_P * (int16_t)uo_popcnt(own_K & potential_attacks_enemy_P);
    score -= uo_score_mobility_K * (int16_t)uo_popcnt(uo_andn(attacks_own_K | attacks_lower_value_own, attacks_enemy_K));
    score -= uo_score_K_square_attackable_by_P * (int16_t)uo_popcnt(enemy_K & potential_attacks_own_P);

    score_mg += uo_score_king_cover_pawn * (int32_t)uo_popcnt(attacks_own_K & own_P);
    score_mg -= uo_score_king_cover_pawn * (int32_t)uo_popcnt(attacks_enemy_K & enemy_P);

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

    // castling rights
    score_mg += uo_score_casting_right * (castling_right_own_OO + castling_right_own_OOO - castling_right_enemy_OO - castling_right_enemy_OOO);

    bool both_rooks_undeveloped_own = uo_popcnt(own_R & (uo_square_bitboard(uo_square__a1) | uo_square_bitboard(uo_square__h1))) == 2;
    bool both_rooks_undeveloped_enemy = uo_popcnt(enemy_R & (uo_square_bitboard(uo_square__a8) | uo_square_bitboard(uo_square__h8))) == 2;

    score_mg += uo_score_rook_stuck_in_corner * both_rooks_undeveloped_own * !(castling_right_own_OO + castling_right_own_OOO);
    score_mg -= uo_score_rook_stuck_in_corner * both_rooks_undeveloped_enemy * !(castling_right_enemy_OO + castling_right_enemy_OOO);

    // king safety
    score_mg += (uo_score_king_in_the_center * (0 != (own_K & (uo_bitboard_file[2] | uo_bitboard_file[3] | uo_bitboard_file[4] | uo_bitboard_file[5]))))
      - (uo_score_king_in_the_center * (0 != (enemy_K & (uo_bitboard_file[2] | uo_bitboard_file[3] | uo_bitboard_file[4] | uo_bitboard_file[5]))));

    score_mg += (uo_score_castled_king * (square_own_K == uo_square__g1))
      - (uo_score_castled_king * (square_enemy_K == uo_square__g8));

    score_mg += (uo_score_castled_king * (square_own_K == uo_square__b1))
      - (uo_score_castled_king * (square_enemy_K == uo_square__b8));

    score_mg += (uo_bitboard_files(attacks_own_K) & uo_bitboard_rank_first) ? uo_score_king_next_to_open_file : 0;
    score_mg -= (uo_bitboard_files(attacks_enemy_K) & uo_bitboard_rank_first) ? uo_score_king_next_to_open_file : 0;

    score += (count_attacker_to_enemy_K * uo_score_attacker_to_K - count_defender_of_enemy_K * uo_score_defender_to_K) * uo_max(0, 1 + count_attacker_to_enemy_K - count_defender_of_enemy_K);
    score -= (count_attacker_to_own_K * uo_score_attacker_to_K - count_defender_of_own_K * uo_score_defender_to_K) * uo_max(0, 1 + count_attacker_to_own_K - count_defender_of_own_K);

    int32_t material_percentage = uo_position_material_percentage(position);

    score += score_mg * material_percentage / 100;
    score += score_eg * (100 - material_percentage) / 100;

    return score;
  }

#ifdef __cplusplus
  }
#endif

#endif
