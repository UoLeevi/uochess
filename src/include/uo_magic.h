#ifndef UO_MAGIC_H
#define UO_MAGIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "uo_bitboard.h"

typedef struct uo_magic
{
  uint32_t number;
  uint32_t shift;
  uo_bitboard *blockers; 
} uo_magic;

void uo_magic_init();

#ifdef __cplusplus
}
#endif

#endif