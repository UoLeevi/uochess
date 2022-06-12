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
#include <limits.h>

  static inline int uo_popcount(uint64_t u64)
  {
    uint64_t v = u64;
    v = v - ((v >> 1) & (uint64_t)~(uint64_t)0 / 3);                                            // temp
    v = (v & (uint64_t)~(uint64_t)0 / 15 * 3) + ((v >> 2) & (uint64_t)~(uint64_t)0 / 15 * 3);   // temp
    v = (v + (v >> 4)) & (uint64_t)~(uint64_t)0 / 255 * 15;                                     // temp
    return (uint64_t)(v * ((uint64_t)~(uint64_t)0 / 255)) >> (sizeof(uint64_t) - 1) * CHAR_BIT; // count
  }
#endif
  // END - uo_popcount

  // uo_ffs
#if __has_builtin(__builtin_ffsll)
#define uo_ffs __builtin_ffsll
#elif __has_builtin(__builtin_clzll)
#define uo_ffs(u64) ((u64) ? (64 - __builtin_clzll((uint64_t)(u64))) : 0)
#else
#include <intrin.h>
#pragma intrinsic(_BitScanForward64)

  static inline int uo_ffs(uint64_t u64)
  {
    int ffs = 0;
    return _BitScanForward64(&ffs, u64) ? (ffs + 1) : 0;
  }
#endif
  // END - uo_ffs

#endif

  uint64_t uo_rand();
  void uo_rand_init(uint64_t seed);

#ifdef __cplusplus
}
#endif

#endif
