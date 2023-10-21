#include "uo_evaluation.h"

#include <stdint.h>

const int16_t score_attacks_to_K[100] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 3, 6, 10, 13, 16, 19, 22, 26, 
  29, 32, 35, 38, 42, 45, 48, 51, 55, 58, 
  61, 64, 67, 71, 74, 77, 80, 83, 87, 90, 
  93, 131, 168, 206, 243, 281, 318, 356, 393, 431, 
  468, 506, 543, 581, 618, 618, 618, 618, 618, 618, 
  618, 618, 618, 618, 618, 618, 618, 618, 618, 618, 
  618, 618, 618, 618, 618, 618, 618, 618, 618, 618, 
  618, 618, 618, 618, 618, 618, 618, 618, 618, 618, 
  618, 618, 618, 618, 618, 618, 618, 618, 618, 618
};

const int16_t score_piece_attacked_by_enemy[] = {
  uo_score_P_attacked_by_enemy_P, uo_score_P_attacked_by_enemy_N, uo_score_P_attacked_by_enemy_B, uo_score_P_attacked_by_enemy_R, uo_score_P_attacked_by_enemy_Q, uo_score_P_attacked_by_enemy_K,
  uo_score_N_attacked_by_enemy_P, uo_score_N_attacked_by_enemy_N, uo_score_N_attacked_by_enemy_B, uo_score_N_attacked_by_enemy_R, uo_score_N_attacked_by_enemy_Q, uo_score_N_attacked_by_enemy_K, 
  uo_score_B_attacked_by_enemy_P, uo_score_B_attacked_by_enemy_N, uo_score_B_attacked_by_enemy_B, uo_score_B_attacked_by_enemy_R, uo_score_B_attacked_by_enemy_Q, uo_score_B_attacked_by_enemy_K, 
  uo_score_R_attacked_by_enemy_P, uo_score_R_attacked_by_enemy_N, uo_score_R_attacked_by_enemy_B, uo_score_R_attacked_by_enemy_R, uo_score_R_attacked_by_enemy_Q, uo_score_R_attacked_by_enemy_K, 
  uo_score_Q_attacked_by_enemy_P, uo_score_Q_attacked_by_enemy_N, uo_score_Q_attacked_by_enemy_B, uo_score_Q_attacked_by_enemy_R, uo_score_Q_attacked_by_enemy_Q, uo_score_Q_attacked_by_enemy_K, 
};

const int16_t score_piece_defended_by[] = {
  uo_score_P_defended_by_P, uo_score_P_defended_by_N, uo_score_P_defended_by_B, uo_score_P_defended_by_R, uo_score_P_defended_by_Q, uo_score_P_defended_by_K,
  uo_score_N_defended_by_P, uo_score_N_defended_by_N, uo_score_N_defended_by_B, uo_score_N_defended_by_R, uo_score_N_defended_by_Q, uo_score_N_defended_by_K,
  uo_score_B_defended_by_P, uo_score_B_defended_by_N, uo_score_B_defended_by_B, uo_score_B_defended_by_R, uo_score_B_defended_by_Q, uo_score_B_defended_by_K,
  uo_score_R_defended_by_P, uo_score_R_defended_by_N, uo_score_R_defended_by_B, uo_score_R_defended_by_R, uo_score_R_defended_by_Q, uo_score_R_defended_by_K,
  uo_score_Q_defended_by_P, uo_score_Q_defended_by_N, uo_score_Q_defended_by_B, uo_score_Q_defended_by_R, uo_score_Q_defended_by_Q, uo_score_Q_defended_by_K,
};

const int16_t score_mg_P[56] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  score_mg_P_a2, score_mg_P_b2, score_mg_P_c2, score_mg_P_d2, score_mg_P_e2, score_mg_P_f2, score_mg_P_g2, score_mg_P_h2,
  score_mg_P_a3, score_mg_P_b3, score_mg_P_c3, score_mg_P_d3, score_mg_P_e3, score_mg_P_f3, score_mg_P_g3, score_mg_P_h3,
  score_mg_P_a4, score_mg_P_b4, score_mg_P_c4, score_mg_P_d4, score_mg_P_e4, score_mg_P_f4, score_mg_P_g4, score_mg_P_h4,
  score_mg_P_a5, score_mg_P_b5, score_mg_P_c5, score_mg_P_d5, score_mg_P_e5, score_mg_P_f5, score_mg_P_g5, score_mg_P_h5,
  score_mg_P_a6, score_mg_P_b6, score_mg_P_c6, score_mg_P_d6, score_mg_P_e6, score_mg_P_f6, score_mg_P_g6, score_mg_P_h6,
  score_mg_P_a7, score_mg_P_b7, score_mg_P_c7, score_mg_P_d7, score_mg_P_e7, score_mg_P_f7, score_mg_P_g7, score_mg_P_h7,
};

const int16_t score_eg_P[56] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  score_eg_P_a2, score_eg_P_b2, score_eg_P_c2, score_eg_P_d2, score_eg_P_e2, score_eg_P_f2, score_eg_P_g2, score_eg_P_h2,
  score_eg_P_a3, score_eg_P_b3, score_eg_P_c3, score_eg_P_d3, score_eg_P_e3, score_eg_P_f3, score_eg_P_g3, score_eg_P_h3,
  score_eg_P_a4, score_eg_P_b4, score_eg_P_c4, score_eg_P_d4, score_eg_P_e4, score_eg_P_f4, score_eg_P_g4, score_eg_P_h4,
  score_eg_P_a5, score_eg_P_b5, score_eg_P_c5, score_eg_P_d5, score_eg_P_e5, score_eg_P_f5, score_eg_P_g5, score_eg_P_h5,
  score_eg_P_a6, score_eg_P_b6, score_eg_P_c6, score_eg_P_d6, score_eg_P_e6, score_eg_P_f6, score_eg_P_g6, score_eg_P_h6,
  score_eg_P_a7, score_eg_P_b7, score_eg_P_c7, score_eg_P_d7, score_eg_P_e7, score_eg_P_f7, score_eg_P_g7, score_eg_P_h7,
};

const int16_t score_mobility_N[9] = {
  score_mobility_N_0,
  score_mobility_N_1,
  score_mobility_N_2,
  score_mobility_N_3,
  score_mobility_N_4,
  score_mobility_N_5,
  score_mobility_N_6,
  score_mobility_N_7,
  score_mobility_N_8
};

const int16_t score_mobility_B[14] = {
  score_mobility_B_0,
  score_mobility_B_1,
  score_mobility_B_2,
  score_mobility_B_3,
  score_mobility_B_4,
  score_mobility_B_5,
  score_mobility_B_6,
  score_mobility_B_7,
  score_mobility_B_8,
  score_mobility_B_9,
  score_mobility_B_10,
  score_mobility_B_11,
  score_mobility_B_12,
  score_mobility_B_13
};

const int16_t score_mobility_R[15] = {
  score_mobility_R_0,
  score_mobility_R_1,
  score_mobility_R_2,
  score_mobility_R_3,
  score_mobility_R_4,
  score_mobility_R_5,
  score_mobility_R_6,
  score_mobility_R_7,
  score_mobility_R_8,
  score_mobility_R_9,
  score_mobility_R_10,
  score_mobility_R_11,
  score_mobility_R_12,
  score_mobility_R_13,
  score_mobility_R_14
};

const int16_t score_mobility_Q[28] = {
  score_mobility_Q_0,
  score_mobility_Q_1,
  score_mobility_Q_2,
  score_mobility_Q_3,
  score_mobility_Q_4,
  score_mobility_Q_5,
  score_mobility_Q_6,
  score_mobility_Q_7,
  score_mobility_Q_8,
  score_mobility_Q_9,
  score_mobility_Q_10,
  score_mobility_Q_11,
  score_mobility_Q_12,
  score_mobility_Q_13,
  score_mobility_Q_14,
  score_mobility_Q_15,
  score_mobility_Q_16,
  score_mobility_Q_17,
  score_mobility_Q_18,
  score_mobility_Q_19,
  score_mobility_Q_20,
  score_mobility_Q_21,
  score_mobility_Q_22,
  score_mobility_Q_23,
  score_mobility_Q_24,
  score_mobility_Q_25,
  score_mobility_Q_26,
  score_mobility_Q_27
};
