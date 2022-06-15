#ifndef UO_UTIL_H
#define UO_UTIL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <inttypes.h>

#ifdef __has_builtin

  // uo_popcount
#if __has_builtin(__builtin_popcountll)
#define uo_popcount __builtin_popcountll
#else
  int uo_popcount(uint64_t u64);
#endif
  // END - uo_popcount

  // uo_ffs
#if __has_builtin(__builtin_ffsll)
#define uo_ffs __builtin_ffsll
#elif __has_builtin(__builtin_clzll)
#define uo_ffs(u64) ((u64) ? (64 - __builtin_clzll((uint64_t)(u64))) : 0)
#else
  int uo_ffs(uint64_t u64);
#endif
  // END - uo_ffs

#endif

  uint64_t uo_rand();
  void uo_rand_init(uint64_t seed);

#ifdef __cplusplus
}
#endif

#endif
