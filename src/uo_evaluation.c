#include "uo_evaluation.h"
#include "uo_util.h"

#include <stdint.h>

#define uo_bitboard_mask_developed_N ((uo_bitboard)0x00007E7E7E7E0000)
#define uo_bitboard_mask_dangerous_own_N ((uo_bitboard)0x00007E7E7E000000)
#define uo_bitboard_mask_dangerous_enemy_N ((uo_bitboard)0x0000007E7E7E0000)
#define uo_bitboard_mask_developed_own_B ((uo_bitboard)0x0000183C3C7E4200)
#define uo_bitboard_mask_developed_enemy_B ((uo_bitboard)0x00427E3C3C180000)

// side to move
const int16_t uo_score_tempo = 25;

// mobility
const int16_t uo_score_square_access = 10;

const int16_t uo_score_mobility_P = 5;
const int16_t uo_score_mobility_N = 1;
const int16_t uo_score_mobility_B = 1;
const int16_t uo_score_mobility_R = 1;
const int16_t uo_score_mobility_Q = 1;
const int16_t uo_score_mobility_K = 1;

const int16_t uo_score_N_square_attackable_by_P = -20;
const int16_t uo_score_B_square_attackable_by_P = -10;
const int16_t uo_score_R_square_attackable_by_P = -30;
const int16_t uo_score_Q_square_attackable_by_P = -50;
const int16_t uo_score_K_square_attackable_by_P = -70;

// space
const int16_t uo_score_space_P = 15;
const int16_t uo_score_space_N = 3;
const int16_t uo_score_space_B = 3;
const int16_t uo_score_space_R = 8;
const int16_t uo_score_space_Q = 2;

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
const int16_t uo_score_castled_king = 50;
const int16_t uo_score_king_cover_pawn = 50;
const int16_t uo_score_king_next_to_open_file = -90;

int16_t mg_table_P[64] = {
      0,   0,   0,   0,   0,   0,  0,   0,
     98, 134,  61,  95,  68, 126, 34, -11,
     -6,   7,  26,  31,  65,  56, 25, -20,
    -14,  13,   6,  21,  23,  12, 17, -23,
    -27,  -2,  -5,  12,  17,   6, 10, -25,
    -26,  -4,  -4, -10,   3,   3, 33, -12,
    -35,  -1, -20, -23, -15,  24, 38, -22,
      0,   0,   0,   0,   0,   0,  0,   0
};

int16_t eg_table_P[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
    178, 173, 158, 134, 147, 132, 165, 187,
     94, 100,  85,  67,  56,  53,  82,  84,
     32,  24,  13,   5,  -2,   4,  17,  17,
     13,   9,  -3,  -7,  -7,  -8,   3,  -1,
      4,   7,  -6,   1,   0,  -5,  -1,  -8,
     13,   8,   8,  10,  13,   0,   2,  -7,
      0,   0,   0,   0,   0,   0,   0,   0
};

int16_t mg_table_N[64] = {
    -167, -89, -34, -49,  61, -97, -15, -107,
     -73, -41,  72,  36,  23,  62,   7,  -17,
     -47,  60,  37,  65,  84, 129,  73,   44,
      -9,  17,  19,  53,  37,  69,  18,   22,
     -13,   4,  16,  13,  28,  19,  21,   -8,
     -23,  -9,  12,  10,  19,  17,  25,  -16,
     -29, -53, -12,  -3,  -1,  18, -14,  -19,
    -105, -21, -58, -33, -17, -28, -19,  -23
};

int16_t eg_table_N[64] = {
    -58, -38, -13, -28, -31, -27, -63, -99,
    -25,  -8, -25,  -2,  -9, -25, -24, -52,
    -24, -20,  10,   9,  -1,  -9, -19, -41,
    -17,   3,  22,  22,  22,  11,   8, -18,
    -18,  -6,  16,  25,  16,  17,   4, -18,
    -23,  -3,  -1,  15,  10,  -3, -20, -22,
    -42, -20, -10,  -5,  -2, -20, -23, -44,
    -29, -51, -23, -15, -22, -18, -50, -64
};

int16_t mg_table_B[64] = {
    -29,   4, -82, -37, -25, -42,   7,  -8,
    -26,  16, -18, -13,  30,  59,  18, -47,
    -16,  37,  43,  40,  35,  50,  37,  -2,
     -4,   5,  19,  50,  37,  37,   7,  -2,
     -6,  13,  13,  26,  34,  12,  10,   4,
      0,  15,  15,  15,  14,  27,  18,  10,
      4,  15,  16,   0,   7,  21,  33,   1,
    -33,  -3, -14, -21, -13, -12, -39, -21
};

int16_t eg_table_B[64] = {
    -14, -21, -11,  -8, -7,  -9, -17, -24,
     -8,  -4,   7, -12, -3, -13,  -4, -14,
      2,  -8,   0,  -1, -2,   6,   0,   4,
     -3,   9,  12,   9, 14,  10,   3,   2,
     -6,   3,  13,  19,  7,  10,  -3,  -9,
    -12,  -3,   8,  10, 13,   3,  -7, -15,
    -14, -18,  -7,  -1,  4,  -9, -15, -27,
    -23,  -9, -23,  -5, -9, -16,  -5, -17
};

int16_t mg_table_R[64] = {
     32,  42,  32,  51, 63,  9,  31,  43,
     27,  32,  58,  62, 80, 67,  26,  44,
     -5,  19,  26,  36, 17, 45,  61,  16,
    -24, -11,   7,  26, 24, 35,  -8, -20,
    -36, -26, -12,  -1,  9, -7,   6, -23,
    -45, -25, -16, -17,  3,  0,  -5, -33,
    -44, -16, -20,  -9, -1, 11,  -6, -71,
    -19, -13,   1,  17, 16,  7, -37, -26
};

int16_t eg_table_R[64] = {
    13, 10, 18, 15, 12,  12,   8,   5,
    11, 13, 13, 11, -3,   3,   8,   3,
     7,  7,  7,  5,  4,  -3,  -5,  -3,
     4,  3, 13,  1,  2,   1,  -1,   2,
     3,  5,  8,  4, -5,  -6,  -8, -11,
    -4,  0, -5, -1, -7, -12,  -8, -16,
    -6, -6,  0,  2, -9,  -9, -11,  -3,
    -9,  2,  3, -1, -5, -13,   4, -20
};

int16_t mg_table_Q[64] = {
    -28,   0,  29,  12,  59,  44,  43,  45,
    -24, -39,  -5,   1, -16,  57,  28,  54,
    -13, -17,   7,   8,  29,  56,  47,  57,
    -27, -27, -16, -16,  -1,  17,  -2,   1,
     -9, -26,  -9, -10,  -2,  -4,   3,  -3,
    -14,   2, -11,  -2,  -5,   2,  14,   5,
    -35,  -8,  11,   2,   8,  15,  -3,   1,
     -1, -18,  -9,  10, -15, -25, -31, -50
};

int16_t eg_table_Q[64] = {
     -9,  22,  22,  27,  27,  19,  10,  20,
    -17,  20,  32,  41,  58,  25,  30,   0,
    -20,   6,   9,  49,  47,  35,  19,   9,
      3,  22,  24,  45,  57,  40,  57,  36,
    -18,  28,  19,  47,  31,  34,  39,  23,
    -16, -27,  15,   6,   9,  17,  10,   5,
    -22, -23, -30, -16, -16, -23, -36, -32,
    -33, -28, -22, -43,  -5, -32, -20, -41
};

int16_t mg_table_K[64] = {
    -65,  23,  16, -15, -56, -34,   2,  13,
     29,  -1, -20,  -7,  -8,  -4, -38, -29,
     -9,  24,   2, -16, -20,   6,  22, -22,
    -17, -20, -12, -27, -30, -25, -14, -36,
    -49,  -1, -27, -39, -46, -44, -33, -51,
    -14, -14, -22, -46, -44, -30, -15, -27,
      1,   7,  -8, -64, -43, -16,   9,   8,
    -15,  36,  12, -54,   8, -28,  24,  14
};

int16_t eg_table_K[64] = {
    -74, -35, -18, -18, -11,  15,   4, -17,
    -12,  17,  14,  17,  17,  38,  23,  11,
     10,  17,  23,  15,  20,  45,  44,  13,
     -8,  22,  24,  27,  26,  33,  26,   3,
    -18,  -4,  21,  24,  27,  23,   9, -11,
    -19,  -3,  11,  21,  23,  16,   7,  -9,
    -27, -11,   4,  13,  14,   4,  -5, -17,
    -53, -34, -21, -11, -28, -14, -24, -43
};

int16_t *mg_pesto_table[6] =
{
    mg_table_P,
    mg_table_N,
    mg_table_B,
    mg_table_R,
    mg_table_Q,
    mg_table_Q
};

int16_t *eg_pesto_table[6] =
{
    eg_table_P,
    eg_table_N,
    eg_table_B,
    eg_table_R,
    eg_table_Q,
    eg_table_Q
};

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

    score_mg += mg_table_N[square_own_N];
    score_eg += eg_table_N[square_own_N];

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

    score_mg -= mg_table_Q[square_enemy_Q ^ 56];
    score_eg -= eg_table_Q[square_enemy_Q ^ 56];

    if (attacks_enemy_Q & next_to_own_K) score_eg -= uo_score_attacker_to_K;
  }

  // king safety

  score_mg += mg_table_K[square_enemy_K];
  score_eg += eg_table_K[square_enemy_K];

  score_mg -= mg_table_K[square_enemy_K ^ 56];
  score_eg -= eg_table_K[square_enemy_K ^ 56];

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
