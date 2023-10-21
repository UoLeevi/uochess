#ifndef UO_EVALUATION_H
#define UO_EVALUATION_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_bitboard.h"
#include "uo_piece.h"
#include "uo_util.h"
#include "uo_def.h"

#include <math.h>
#include <stdbool.h>

#define uo_score_centipawn_to_win_prob(cp) (atanf(cp / 290.680623072f) / 3.096181612f + 0.5f)
#define uo_score_centipawn_to_q_score(cp) (atanf(cp / 111.714640912f) / 1.5620688421f)
#define uo_score_win_prob_to_centipawn(winprob) (int16_t)(290.680623072f * tanf(3.096181612f * (win_prob - 0.5f)))
#define uo_score_q_score_to_centipawn(q_score) (int16_t)(111.714640912f * tanf(1.5620688421f * q_score))

  static inline bool uo_score_is_checkmate(int16_t score)
  {
    return score > uo_score_mate_in_threshold || score < -uo_score_mate_in_threshold;
  }

  static inline int16_t uo_score_adjust_to_ttable(int16_t ply, int16_t score)
  {
    return score > uo_score_tb_win_threshold ? score + ply
      : score < -uo_score_tb_win_threshold ? score - ply
      : score;
  }

  static inline int16_t uo_score_adjust_from_ttable(int16_t ply, int16_t score)
  {
    return score > uo_score_tb_win_threshold ? score - ply
      : score < -uo_score_tb_win_threshold ? score + ply
      : score;
  }

  // factor:          0,    1,    2,    3,    4,    5,    6,    7,    8,    9,   10
  // log(1 + factor): 0, 0.69, 1.10, 1.39, 1.61, 1.79, 1.96, 2.08, 2.20, 2.30, 2.40
  static inline int16_t uo_score_mul_ln(int16_t score, int factor)
  {
    float ln;

    switch (factor)
    {
      case 0: ln = 0; break;
      case 1: ln = 0.693; break;
      case 2: ln = 1.099; break;
      case 3: ln = 1.386; break;
      case 4: ln = 1.609; break;
      case 5: ln = 1.792; break;
      case 6: ln = 1.946; break;
      case 7: ln = 2.079; break;
      case 8: ln = 2.197; break;
      case 9: ln = 2.303; break;
      case 10: ln = 2.398; break;
      case 11: ln = 2.485; break;
      case 12: ln = 2.565; break;
      case 13: ln = 2.639; break;
      case 14: ln = 2.708; break;
      case 15: ln = 2.773; break;
      case 16: ln = 2.833; break;
      case 17: ln = 2.89; break;
      case 18: ln = 2.944; break;
      case 19: ln = 2.996; break;
      case 20: ln = 3.045; break;
      case 21: ln = 3.091; break;
      case 22: ln = 3.135; break;
      case 23: ln = 3.178; break;
      case 24: ln = 3.219; break;
      case 25: ln = 3.258; break;
      case 26: ln = 3.296; break;
      case 27: ln = 3.332; break;
      case 28: ln = 3.367; break;
      case 29: ln = 3.401; break;
      case 30: ln = 3.434; break;
      case 31: ln = 3.466; break;
      case 32: ln = 3.497; break;
      case 33: ln = 3.526; break;
      case 34: ln = 3.555; break;
      case 35: ln = 3.584; break;
      case 36: ln = 3.611; break;
      case 37: ln = 3.638; break;
      case 38: ln = 3.664; break;
      case 39: ln = 3.689; break;
      case 40: ln = 3.714; break;
      case 41: ln = 3.738; break;
      case 42: ln = 3.761; break;
      case 43: ln = 3.784; break;
      case 44: ln = 3.807; break;
      case 45: ln = 3.829; break;
      case 46: ln = 3.85; break;
      case 47: ln = 3.871; break;
      case 48: ln = 3.892; break;
      case 49: ln = 3.912; break;
      case 50: ln = 3.932; break;
      case 51: ln = 3.951; break;
      case 52: ln = 3.97; break;
      case 53: ln = 3.989; break;
      case 54: ln = 4.007; break;
      case 55: ln = 4.025; break;
      case 56: ln = 4.043; break;
      case 57: ln = 4.06; break;
      case 58: ln = 4.078; break;
      case 59: ln = 4.094; break;
      case 60: ln = 4.111; break;
      case 61: ln = 4.127; break;
      case 62: ln = 4.143; break;
      case 63: ln = 4.159; break;
      default: ln = logf(1.0f + (float)factor); break;
    }

    return (float)score * ln;
  }

  // side to move
#define uo_score_tempo 23

  // mobility

#define uo_score_mobility_P 18
#define uo_score_zero_mobility_P -5

#define uo_score_zero_mobility_K -54

  extern const int16_t score_mobility_N[9];
#define score_mobility_N_0 -44
#define score_mobility_N_1 -25
#define score_mobility_N_2 -12
#define score_mobility_N_3  -2
#define score_mobility_N_4   7
#define score_mobility_N_5  14
#define score_mobility_N_6  21
#define score_mobility_N_7  27
#define score_mobility_N_8  29

  extern const int16_t score_mobility_B[14];
#define score_mobility_B_0 -31
#define score_mobility_B_1 -24
#define score_mobility_B_2 -14
#define score_mobility_B_3  -8
#define score_mobility_B_4  -3
#define score_mobility_B_5   1
#define score_mobility_B_6   5
#define score_mobility_B_7   5
#define score_mobility_B_8   7
#define score_mobility_B_9   7
#define score_mobility_B_10 10
#define score_mobility_B_11 12
#define score_mobility_B_12 15
#define score_mobility_B_13 15

  extern const int16_t score_mobility_R[15];
#define score_mobility_R_0 -19
#define score_mobility_R_1  -2
#define score_mobility_R_2  -1
#define score_mobility_R_3   5
#define score_mobility_R_4   7
#define score_mobility_R_5  15
#define score_mobility_R_6  19
#define score_mobility_R_7  25
#define score_mobility_R_8  30
#define score_mobility_R_9  35
#define score_mobility_R_10 38
#define score_mobility_R_11 40
#define score_mobility_R_12 42
#define score_mobility_R_13 42
#define score_mobility_R_14 42

  extern const int16_t score_mobility_Q[28];
#define score_mobility_Q_0   9
#define score_mobility_Q_1  14
#define score_mobility_Q_2  14
#define score_mobility_Q_3  14
#define score_mobility_Q_4  16
#define score_mobility_Q_5  18
#define score_mobility_Q_6  21
#define score_mobility_Q_7  23
#define score_mobility_Q_8  25
#define score_mobility_Q_9  26
#define score_mobility_Q_10 28
#define score_mobility_Q_11 29
#define score_mobility_Q_12 30
#define score_mobility_Q_13 31
#define score_mobility_Q_14 32
#define score_mobility_Q_15 33
#define score_mobility_Q_16 36
#define score_mobility_Q_17 36
#define score_mobility_Q_18 37
#define score_mobility_Q_19 39
#define score_mobility_Q_20 43
#define score_mobility_Q_21 43
#define score_mobility_Q_22 43
#define score_mobility_Q_23 43
#define score_mobility_Q_24 43
#define score_mobility_Q_25 43
#define score_mobility_Q_26 43
#define score_mobility_Q_27 43

  // pawns
#define uo_score_supported_P 5
#define uo_score_isolated_P -12

#define uo_score_passed_pawn_on_fifth 20
#define uo_score_passed_pawn_on_sixth 67

  // piece development
#define uo_score_rook_stuck_in_corner -34

  // attacked and defended pieces

  extern const int16_t score_piece_attacked_by_enemy[];
#define uo_score_P_attacked_by_enemy_P -5
#define uo_score_P_attacked_by_enemy_N -5
#define uo_score_P_attacked_by_enemy_B -5
#define uo_score_P_attacked_by_enemy_R -5
#define uo_score_P_attacked_by_enemy_Q -5
#define uo_score_P_attacked_by_enemy_K -5

#define uo_score_N_attacked_by_enemy_P -5
#define uo_score_N_attacked_by_enemy_N -5
#define uo_score_N_attacked_by_enemy_B -5
#define uo_score_N_attacked_by_enemy_R -5
#define uo_score_N_attacked_by_enemy_Q -5
#define uo_score_N_attacked_by_enemy_K -5

#define uo_score_B_attacked_by_enemy_P -5
#define uo_score_B_attacked_by_enemy_N -5
#define uo_score_B_attacked_by_enemy_B -5
#define uo_score_B_attacked_by_enemy_R -5
#define uo_score_B_attacked_by_enemy_Q -5
#define uo_score_B_attacked_by_enemy_K -5

#define uo_score_R_attacked_by_enemy_P -5
#define uo_score_R_attacked_by_enemy_N -5
#define uo_score_R_attacked_by_enemy_B -5
#define uo_score_R_attacked_by_enemy_R -5
#define uo_score_R_attacked_by_enemy_Q -5
#define uo_score_R_attacked_by_enemy_K -5

#define uo_score_Q_attacked_by_enemy_P -5
#define uo_score_Q_attacked_by_enemy_N -5
#define uo_score_Q_attacked_by_enemy_B -5
#define uo_score_Q_attacked_by_enemy_R -5
#define uo_score_Q_attacked_by_enemy_Q -5
#define uo_score_Q_attacked_by_enemy_K -5



  extern const int16_t score_piece_defended_by[];
#define uo_score_P_defended_by_P 5
#define uo_score_P_defended_by_N 5
#define uo_score_P_defended_by_B 5
#define uo_score_P_defended_by_R 5
#define uo_score_P_defended_by_Q 5
#define uo_score_P_defended_by_K 5

#define uo_score_N_defended_by_P 5
#define uo_score_N_defended_by_N 5
#define uo_score_N_defended_by_B 5
#define uo_score_N_defended_by_R 5
#define uo_score_N_defended_by_Q 5
#define uo_score_N_defended_by_K 5

#define uo_score_B_defended_by_P 5
#define uo_score_B_defended_by_N 5
#define uo_score_B_defended_by_B 5
#define uo_score_B_defended_by_R 5
#define uo_score_B_defended_by_Q 5
#define uo_score_B_defended_by_K 5

#define uo_score_R_defended_by_P 5
#define uo_score_R_defended_by_N 5
#define uo_score_R_defended_by_B 5
#define uo_score_R_defended_by_R 5
#define uo_score_R_defended_by_Q 5
#define uo_score_R_defended_by_K 5

#define uo_score_Q_defended_by_P 5
#define uo_score_Q_defended_by_N 5
#define uo_score_Q_defended_by_B 5
#define uo_score_Q_defended_by_R 5
#define uo_score_Q_defended_by_Q 5
#define uo_score_Q_defended_by_K 5


  // attacks near king
  // see: https://www.chessprogramming.org/King_Safety#Attack_Units
#define uo_attack_unit_N 2
#define uo_attack_unit_B 2
#define uo_attack_unit_R 3
#define uo_attack_unit_Q 5
#define uo_attack_unit_supported_contact_R 2
#define uo_attack_unit_supported_contact_Q 6

  // king safety and castling
#define uo_score_casting_right 10
#define uo_score_king_in_the_center -5
#define uo_score_castled_king 13
#define uo_score_king_cover_pawn 8

  extern const int16_t score_attacks_to_K[100];

  extern const int16_t score_mg_P[56];
#define score_mg_P_a2 -16
#define score_mg_P_b2 -24
#define score_mg_P_c2 -17
#define score_mg_P_d2 -22
#define score_mg_P_e2 -16
#define score_mg_P_f2   6
#define score_mg_P_g2   2
#define score_mg_P_h2 -16

#define score_mg_P_a3 -26
#define score_mg_P_b3 -12
#define score_mg_P_c3 -15
#define score_mg_P_d3 -19
#define score_mg_P_e3 -10
#define score_mg_P_f3  -5
#define score_mg_P_g3  10
#define score_mg_P_h3  -9

#define score_mg_P_a4 -17
#define score_mg_P_b4 -14
#define score_mg_P_c4  -3
#define score_mg_P_d4   0
#define score_mg_P_e4   3
#define score_mg_P_f4  12
#define score_mg_P_g4  -3
#define score_mg_P_h4 -13

#define score_mg_P_a5 -28
#define score_mg_P_b5 -13
#define score_mg_P_c5  -4
#define score_mg_P_d5   2
#define score_mg_P_e5  17
#define score_mg_P_f5  16
#define score_mg_P_g5   6
#define score_mg_P_h5 -12

#define score_mg_P_a6 -41
#define score_mg_P_b6 -27
#define score_mg_P_c6 -12
#define score_mg_P_d6  -2
#define score_mg_P_e6  -7
#define score_mg_P_f6  58
#define score_mg_P_g6  35
#define score_mg_P_h6  -2

#define score_mg_P_a7  30
#define score_mg_P_b7  37
#define score_mg_P_c7  37
#define score_mg_P_d7  85
#define score_mg_P_e7 114
#define score_mg_P_f7  98
#define score_mg_P_g7 -13
#define score_mg_P_h7  41

  extern const int16_t score_eg_P[56];
#define score_eg_P_a2   2
#define score_eg_P_b2   7
#define score_eg_P_c2  -4
#define score_eg_P_d2   0
#define score_eg_P_e2  34
#define score_eg_P_f2  18
#define score_eg_P_g2   7
#define score_eg_P_h2   0

#define score_eg_P_a3   7
#define score_eg_P_b3   3
#define score_eg_P_c3   4
#define score_eg_P_d3  20
#define score_eg_P_e3  20
#define score_eg_P_f3  22
#define score_eg_P_g3  17
#define score_eg_P_h3   8

#define score_eg_P_a4  23
#define score_eg_P_b4  20
#define score_eg_P_c4  10
#define score_eg_P_d4   9
#define score_eg_P_e4  10
#define score_eg_P_f4  22
#define score_eg_P_g4  29
#define score_eg_P_h4  19

#define score_eg_P_a5  48
#define score_eg_P_b5  32
#define score_eg_P_c5  22
#define score_eg_P_d5   5
#define score_eg_P_e5  16
#define score_eg_P_f5  19
#define score_eg_P_g5  35
#define score_eg_P_h5  36

#define score_eg_P_a6  80
#define score_eg_P_b6  70
#define score_eg_P_c6  36
#define score_eg_P_d6  17
#define score_eg_P_e6   3
#define score_eg_P_f6   9
#define score_eg_P_g6  21
#define score_eg_P_h6  28

#define score_eg_P_a7 227
#define score_eg_P_b7 233
#define score_eg_P_c7 228
#define score_eg_P_d7 164
#define score_eg_P_e7 117
#define score_eg_P_f7 123
#define score_eg_P_g7 139
#define score_eg_P_h7 147


#ifdef __cplusplus
}
#endif

#endif
