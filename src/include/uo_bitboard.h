#ifndef UO_BITBOARD_H
#define UO_BITBOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

typedef uint64_t uo_bitboard;

#define uo_bitboard_file(i) ((uo_bitboard)(i) & (uo_bitboard)7)
#define uo_bitboard_rank(i) ((uo_bitboard)(i) >> 3)

int uo_bitboard_print(uo_bitboard bitboard);

#ifdef __cplusplus
}
#endif

#endif