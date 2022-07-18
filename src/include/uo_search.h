#ifndef UO_SEARCH_H
#define UO_SEARCH_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_position.h"
#include "uo_ttable.h"
#include "uo_thread.h"
#include "uo_def.h"

#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>

  typedef struct uo_search_params
  {
    uint8_t depth;
    uint64_t movetime;
    uint32_t time_own;
    uint32_t time_enemy;
    uint32_t time_inc_own;
    uint32_t time_inc_enemy;
    uint64_t nodes;
    uint8_t mate;
    uint16_t movestogo;
    bool ponder;
    uo_move *searchmoves;
  } uo_search_params;

  void *uo_engine_thread_run_negamax_search(void *arg);

#ifdef __cplusplus
}
#endif

#endif
