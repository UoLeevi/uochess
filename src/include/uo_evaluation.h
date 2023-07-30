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
#define uo_score_tempo 45

  // mobility
#define uo_score_mobility_P 6
#define uo_score_mobility_N 4
#define uo_score_mobility_B 12
#define uo_score_mobility_R 14
#define uo_score_mobility_Q 14
#define uo_score_mobility_K 3

#define uo_score_zero_mobility_P -15
#define uo_score_zero_mobility_N -25
#define uo_score_zero_mobility_B -25
#define uo_score_zero_mobility_R -20
#define uo_score_zero_mobility_Q -40
#define uo_score_zero_mobility_K -55

#define uo_score_extra_piece 0
#define uo_score_extra_pawn 10

  // pawns
#define uo_score_isolated_P -20

#define uo_score_passed_pawn 10
#define uo_score_passed_pawn_on_fifth 25
#define uo_score_passed_pawn_on_sixth 90
#define uo_score_passed_pawn_on_seventh 170

  // piece development
#define uo_score_rook_stuck_in_corner -40

  // attacks near king
  // see: https://www.chessprogramming.org/King_Safety#Attack_Units
#define uo_attack_unit_N 1
#define uo_attack_unit_B 1
#define uo_attack_unit_R 2
#define uo_attack_unit_Q 4
#define uo_attack_unit_supported_contact_R 2
#define uo_attack_unit_supported_contact_Q 6

  // king safety and castling
#define uo_score_casting_right 30
#define uo_score_king_in_the_center -10
#define uo_score_castled_king 40
#define uo_score_king_cover_pawn 20
#define uo_score_king_next_to_open_file -20


  // piece square table weight
  static inline int16_t uo_score_adjust_piece_square_table_score(int16_t score)
  {
    return score * 0;
  }

  extern const int16_t score_attacks_to_K[100];

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


  //static inline int16_t uo_position_evaluate(const uo_position *position)
  //{
  //  int16_t score = uo_score_tempo;

  //  // opening & middle game
  //  int32_t score_mg = 0;

  //  // endgame
  //  int32_t score_eg = 0;

  //  const uo_piece *board = position->board;

  //  for (uo_square square = 0; square < 64; ++square)
  //  {
  //    uo_piece piece = board[square];

  //    switch (piece)
  //    {
  //      case uo_piece__P:
  //        score += uo_score_P;
  //        score_mg += mg_table_P[square];
  //        score_eg += eg_table_P[square];
  //        break;

  //      case uo_piece__p:
  //        score -= uo_score_P;
  //        score_mg -= mg_table_P[square ^ 56];
  //        score_eg -= eg_table_P[square ^ 56];
  //        break;

  //      case uo_piece__N:
  //        score += uo_score_N;
  //        score_mg += mg_table_N[square];
  //        score_eg += eg_table_N[square];
  //        break;

  //      case uo_piece__n:
  //        score -= uo_score_N;
  //        score_mg -= mg_table_N[square ^ 56];
  //        score_eg -= eg_table_N[square ^ 56];
  //        break;

  //      case uo_piece__B:
  //        score += uo_score_B;
  //        score_mg += mg_table_B[square];
  //        score_eg += eg_table_B[square];
  //        break;

  //      case uo_piece__b:
  //        score -= uo_score_B;
  //        score_mg -= mg_table_B[square ^ 56];
  //        score_eg -= eg_table_B[square ^ 56];
  //        break;

  //      case uo_piece__R:
  //        score += uo_score_R;
  //        score_mg += mg_table_R[square];
  //        score_eg += eg_table_R[square];
  //        break;

  //      case uo_piece__r:
  //        score -= uo_score_R;
  //        score_mg -= mg_table_R[square ^ 56];
  //        score_eg -= eg_table_R[square ^ 56];
  //        break;

  //      case uo_piece__Q:
  //        score += uo_score_Q;
  //        score_mg += mg_table_Q[square];
  //        score_eg += eg_table_Q[square];
  //        break;

  //      case uo_piece__q:
  //        score -= uo_score_Q;
  //        score_mg -= mg_table_Q[square ^ 56];
  //        score_eg -= eg_table_Q[square ^ 56];
  //        break;

  //      case uo_piece__K:
  //        score_mg += mg_table_K[square];
  //        score_eg += eg_table_K[square];
  //        break;

  //      case uo_piece__k:
  //        score_mg -= mg_table_K[square ^ 56];
  //        score_eg -= eg_table_K[square ^ 56];
  //        break;
  //    }
  //  }

  //  int32_t material_percentage = uo_position_material_percentage(position);

  //  score += score_mg * material_percentage / 100;
  //  score += score_eg * (100 - material_percentage) / 100;

  //  return score;
  //}

#ifdef __cplusplus
}
#endif

#endif
