#ifndef UO_TB_H
#define UO_TB_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_position.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

  typedef struct uo_tb
  {
    bool enabled;
    size_t probe_limit;
    size_t probe_depth;
    bool rule50;
    int score_wdl_draw;
    char dir[0x100];
  } uo_tb;

  extern int TBlargest; // 5 if 5-piece tables, 6 if 6-piece tables were found.

  bool uo_tb_init(uo_tb *tb);

  int uo_tb_probe_wdl(uo_position *position, int *success);
  int uo_tb_probe_dtz(uo_position *position, int *success);
  int uo_tb_root_probe(uo_position *position, int *success);
  //int uo_tb_root_probe_wdl(uo_position *position, int16_t *value);

#ifdef __cplusplus
}
#endif

#endif
