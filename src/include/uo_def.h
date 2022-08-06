#ifndef UO_DEF_H
#define UO_DEF_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <inttypes.h>

#define UO_MAX_PLY 128
#define UO_BRANCING_FACTOR 35
#define UO_PV_MAX_LENGTH 32

#define UO_PARALLEL_MIN_DEPTH 9
#define UO_PARALLEL_MAX_COUNT 4
#define UO_PARALLEL_MIN_MOVE_COUNT 3
#define UO_LAZY_SMP_MIN_DEPTH 3
#define UO_LAZY_SMP_FREE_THREAD_COUNT 1

#define UO_TTABLE_LOCK_BITS 8 

#define UO_SCORE_CHECKMATE ((int16_t)0x7FFF)
#define UO_SCORE_MATE_IN_THRESHOLD ((int16_t)(0x7FFF - UO_MAX_PLY))

#define uo_color(color) ((color) & 1)
#define uo_color_own 0
#define uo_color_enemy 1
#define uo_white 0
#define uo_black 1

#define uo_fen_startpos "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"


#ifdef __cplusplus
}
#endif

#endif
