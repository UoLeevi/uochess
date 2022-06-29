#ifndef UO_DEF_H
#define UO_DEF_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <inttypes.h>

#define UO_MAX_PLY 128
#define UO_BRANCING_FACTOR 35

#define UO_SCORE_CHECKMATE ((int16_t)0x7FFF)
#define UO_SCORE_MATE_IN_THRESHOLD ((int16_t)(0x7FFF - UO_MAX_PLY))

#define uo_color(color) ((color) & 1)
#define uo_white 0
#define uo_black 1

#define uo_direction_forward(color) (8 - (uo_color(color) << 4))
#define uo_direction_backwards(color) ((uo_color(color) << 4) - 8)
#define uo_rank_fourth(color) (3 + uo_color(color))

#ifdef __cplusplus
}
#endif

#endif
