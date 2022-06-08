#include "uo_square.h"

#include <stddef.h>

const int uo_square_diagonal[64] = {
     7,  8,  9, 10, 11, 12, 13, 14,
     6,  7,  8,  9, 10, 11, 12, 13,
     5,  6,  7,  8,  9, 10, 11, 12,
     4,  5,  6,  7,  8,  9, 10, 11,
     3,  4,  5,  6,  7,  8,  9, 10,
     2,  3,  4,  5,  6,  7,  8,  9,
     1,  2,  3,  4,  5,  6,  7,  8,
     0,  1,  2,  3,  4,  5,  6,  7};

const int uo_square_antidiagonal[64] = {
    14, 13, 12, 11, 10,  9,  8,  7,
    13, 12, 11, 10,  9,  8,  7,  6,
    12, 11, 10,  9,  8,  7,  6,  5,
    11, 10,  9,  8,  7,  6,  5,  4,
    10,  9,  8,  7,  6,  5,  4,  3,
     9,  8,  7,  6,  5,  4,  3,  2,
     8,  7,  6,  5,  4,  3,  2,  1,
     7,  6,  5,  4,  3,  2,  1,  0};
