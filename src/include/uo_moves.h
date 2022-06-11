#ifndef UO_MOVES_H
#define UO_MOVES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "uo_bitboard.h"
#include "uo_position.h"

#include <inttypes.h>
#include <stdbool.h>

void uo_moves_init();

uo_bitboard uo_position_moves(uo_position *pos, uo_square square, uo_position nodes[64]);

bool uo_test_moves();

#ifdef __cplusplus
}
#endif

#endif