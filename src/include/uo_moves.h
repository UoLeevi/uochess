#ifndef UO_MOVES_H
#define UO_MOVES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "uo_bitboard.h"

#include <inttypes.h>
#include <stdbool.h>

void uo_moves_init();

bool uo_test_moves();

#ifdef __cplusplus
}
#endif

#endif