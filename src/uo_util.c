#include "uo_util.h"

#ifndef uo_popcount_64

#include <limits.h>

int uo_popcount_64(uint64_t u64)
{
  uint64_t v = u64;
  v = v - ((v >> 1) & (uint64_t) ~(uint64_t)0 / 3);                                            // temp
  v = (v & (uint64_t) ~(uint64_t)0 / 15 * 3) + ((v >> 2) & (uint64_t) ~(uint64_t)0 / 15 * 3);  // temp
  v = (v + (v >> 4)) & (uint64_t) ~(uint64_t)0 / 255 * 15;                                     // temp
  return (uint64_t)(v * ((uint64_t) ~(uint64_t)0 / 255)) >> (sizeof(uint64_t) - 1) * CHAR_BIT; // count
}

#endif

uint16_t *uo_bit_permutations(uint16_t *array, int index, int bits, uint16_t number)
{
  // TODO
}