#ifndef UO_SEARCH_H
#define UO_SEARCH_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_position.h"

#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>

#define UO_MAX_PLY 128
#define UO_BRANCING_FACTOR 35

  typedef struct uo_search_pv
  {
    uo_move *moves;
    uint8_t depth;
    struct {
      uint16_t cp;
      uint8_t mate;
    } score;
  } uo_search_pv;

  typedef struct uo_search
  {
    uo_position position;
    uo_move moves[UO_MAX_PLY * UO_BRANCING_FACTOR];
    uo_move *head;
    uo_search_pv *pv;
  } uo_search;

  typedef struct uo_search_info
  {
    bool completed;
    uint8_t depth;
    uint8_t seldepth;
    uint16_t multipv;
    uint64_t nodes;
    uint64_t nps;
    uint8_t tbhits;
    uint64_t time;
    uo_search_pv *pv;
  } uo_search_info;

  typedef struct uo_search_params
  {
    uint8_t depth;
    uint64_t movetime;
    uint16_t multipv;
    void (*report)(uo_search_info);
  } uo_search_params;

  size_t uo_search_perft(uo_search *search, size_t depth);

  int uo_search_start(uo_search *search, uo_search_params params);

  void uo_search_end(int search_id);

#ifdef __cplusplus
}
#endif

#endif
