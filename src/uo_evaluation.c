#include <stdint.h>

int16_t score_attacks_to_K[100] = {
   0,   0,   1,   2,   3,   5,   7,   9,  12,  15,
  18,  22,  26,  30,  35,  39,  44,  50,  56,  62,
  68,  75,  82,  85,  89,  97, 105, 113, 122, 131,
 140, 150, 169, 180, 191, 202, 213, 225, 237, 248,
 260, 272, 283, 295, 307, 319, 330, 342, 354, 366,
 377, 389, 401, 412, 424, 436, 448, 459, 471, 483,
 494, 500, 500, 500, 500, 500, 500, 500, 500, 500,
 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
 500, 500, 500, 500, 500, 500, 500, 500, 500, 500
};

int16_t mg_table_P[64] = {
      0,   0,   0,   0,   0,   0,  0,   0,
     49,  67,  30,  47,  34,  63, 17,  -5,
     -3,   3,  13,  15,  32,  28, 12, -10,
     -7,   6,   3,  10,  11,   6,  8, -12,
    -14,  -1,  -2,   6,   8,   3,  5, -12,
    -13,  -2,  -2,  -5,   1,   1, 16,  -6,
    -18,  -1, -10, -12,  -7,  12, 19, -11,
      0,   0,   0,   0,   0,   0,  0,   0
};

int16_t eg_table_P[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
     89,  87,  79,  67,  73,  66,  82,  93,
     47,  50,  42,  33,  28,  26,  41,  42,
     16,  12,   6,   2,  -1,   2,   8,   8,
      6,   4,  -3,  -4,  -4,  -4,   1,   0,
      6,   4,   4,   5,   6,   0,   1,  -4,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0
};

int16_t mg_table_N[64] = {
    -84, -45, -17, -24,  30, -49,  -8, -54,
    -37, -21,  36,  18,  11,  31,   3,  -8,
    -24,  30,  18,  32,  42,  64,  36,  22,
     -5,   8,   9,  26,  18,  34,   9,  11,
     -7,   2,   8,   6,  14,   9,  10,  -4,
    -12,  -5,   6,   5,   9,   8,  12,  -8,
    -14, -26,  -6,  -2,   0,   9,  -7,  -9,
    -52, -10, -29, -16,  -9, -14,  -9, -11
};

int16_t eg_table_N[64] = {
    -29, -19,  -6, -14, -15, -14, -32, -50,
    -12,  -4, -12,  -1,  -5, -12, -12, -26,
    -12, -10,   5,   4,  -1,  -5,  -9, -20,
     -9,   1,  11,  11,  11,   5,   4,  -9,
     -9,  -3,   8,  12,   8,   8,   2,  -9,
    -11,  -2,   0,   7,   5,  -2, -10, -11,
    -21, -10,  -5,  -2,  -1, -10, -12, -22,
    -15, -26, -11,  -7, -11,  -9, -25, -32
};

int16_t mg_table_B[64] = {
    -14,   2, -41, -19, -12, -21,   4,  -4,
    -13,   8,  -9,  -7,  15,  29,   9, -23,
     -8,  18,  21,  20,  17,  25,  18,  -1,
     -2,   2,   9,  25,  18,  18,   3,  -1,
     -3,   7,   6,  13,  17,   6,   5,   2,
      0,   7,   7,   7,   7,  13,   9,   5,
      2,   7,   8,   0,   3,  10,  16,   0,
    -16,  -1,  -7, -10,  -6,  -6, -19, -10
};

int16_t eg_table_B[64] = {
     -7, -11,  -6,  -4, -4,  -4,  -8, -12,
     -4,  -2,   3,  -6, -2,  -7,  -2,  -7,
      1,  -4,   0,   0, -1,   3,   0,   2,
     -1,   4,   6,   4,  7,   5,   1,   1,
     -3,   1,   6,   9,  3,   5,  -1,  -4,
     -6,  -2,   4,   5,  6,   1,  -4,  -7,
     -7,  -9,  -3,   0,  2,  -4,  -7, -13,
    -11,  -4, -11,  -2, -4,  -8,  -2,  -8
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
    -14,   0,  14,   6,  29,  22,  21,  22,
    -12, -19,  -2,   0,  -8,  28,  14,  27,
     -6,  -8,   3,   4,  14,  28,  23,  28,
    -13, -13,  -8,  -8,   0,   8,  -1,   0,
     -4, -13,  -4,  -5,  -1,  -2,   1,  -1,
     -7,   1,  -6,  -1,  -3,   1,   7,   2,
    -17,  -4,   5,   1,   4,   7,  -1,   0,
      0,  -9,  -4,   5,  -7, -12, -15, -25
};

int16_t eg_table_Q[64] = {
     -4,  11,  11,  13,  13,   9,   5,  10,
     -8,  10,  16,  20,  29,  12,  15,   0,
    -10,   3,   4,  24,  23,  17,   9,   4,
      1,  11,  12,  22,  28,  20,  28,  18,
     -9,  14,   9,  23,  15,  17,  19,  11,
     -8, -13,   7,   3,   4,   8,   5,   2,
    -11, -11, -15,  -8,  -8, -11, -18, -16,
    -16, -14, -11, -21,  -2, -16, -10, -20
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
