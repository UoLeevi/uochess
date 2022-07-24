#ifndef UO_SEARCH_H
#define UO_SEARCH_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_thread.h"
#include "uo_move.h"
#include "uo_misc.h"

#include <stdbool.h>
#include <stdint.h>

#define uo_seach_type__principal_variation 0
#define uo_seach_type__quiescence 1

  typedef struct uo_engine_thread uo_engine_thread;

  typedef struct uo_search_params
  {
    uint8_t seach_type;
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
    int16_t alpha;
    int16_t beta;
  } uo_search_params;

  typedef struct uo_search_info
  {
    uo_engine_thread *thread;
    uint64_t nodes;
    uo_time time_start;
    uint16_t multipv;
    uint8_t depth;
    uint8_t seldepth;
    uint8_t tbhits;
    bool completed;
    int16_t value;
    union
    {
      uo_move bestmove;
      uo_move pv[UO_MAX_PLY];
    };
    union
    {
      uo_move bestmove;
      uo_move pv[UO_MAX_PLY];
    } *secondary_pvs;
  } uo_search_info;

  void *uo_engine_thread_start_timer(void *arg);

  void *uo_engine_thread_run_principal_variation_search(void *arg);

  void *uo_engine_thread_run_quiescence_search(void *arg);

#ifdef __cplusplus
}
#endif

#endif
