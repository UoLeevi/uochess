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
#define UO_LAZY_SMP_MIN_DEPTH 6
#define UO_LAZY_SMP_FREE_THREAD_COUNT 1

// Number of moves to look at when checking for mc-prune.
#define UO_MULTICUT_MOVE_COUNT 12

// Number of cutoffs to cause an mc-prune, C < M.
#define UO_MULTICUT_CUTOFF_COUNT 3

// Search depth reduction for mc-prune searches.
#define UO_MULTICUT_DEPTH_REDUCTION 2

#define UO_TTABLE_LOCK_BITS 10 

#define uo_score_draw 0
#define uo_score_unknown INT16_MIN
#define uo_score_checkmate INT16_MAX
#define uo_score_mate_in_threshold (uo_score_checkmate - UO_MAX_PLY)
#define uo_score_tb_win 10000
#define uo_score_tb_win_threshold (uo_score_tb_win - UO_MAX_PLY)

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
