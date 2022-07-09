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
#define uo_color_own 0
#define uo_color_enemy 1
#define uo_white 0
#define uo_black 1


#ifdef __cplusplus
}
#endif

#endif
