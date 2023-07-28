#ifndef UO_DEF_H
#define UO_DEF_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <inttypes.h>

#define UO_MAX_PLY 128
#define UO_BRANCING_FACTOR 60
#define UO_PV_MAX_LENGTH 32

#define UO_PARALLEL_MIN_DEPTH 9
#define UO_PARALLEL_MAX_COUNT 4
#define UO_PARALLEL_MIN_MOVE_COUNT 3
#define UO_LAZY_SMP_MIN_DEPTH 6
#define UO_LAZY_SMP_FREE_THREAD_COUNT 1

// Insertion sort move count threshold
#define UO_INSERTION_SORT_MOVE_COUNT 4

// Examine check moves in quiescence search until depth
#define UO_QS_CHECKS_DEPTH 2

// Principal variation minimum allocated length
#define UO_PV_ALLOC_MIN_LENGTH 12

#define UO_TTABLE_LOCK_BITS 10 

#define uo_score_draw 0
#define uo_score_unknown INT16_MIN
#define uo_score_checkmate INT16_MAX
#define uo_score_mate_in_threshold (uo_score_checkmate - UO_MAX_PLY)
#define uo_score_tb_win 10000
#define uo_score_tb_win_threshold (uo_score_tb_win - UO_MAX_PLY)

#define uo_score_type__exact 3
#define uo_score_type__upper_bound 8
#define uo_score_type__lower_bound 4

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
