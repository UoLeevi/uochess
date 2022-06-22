#ifndef UO_UTIL_H
#define UO_UTIL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <inttypes.h>

#if defined(_MSC_VER)

# include <intrin.h>
# pragma intrinsic(_BitScanForward64, _BitScanReverse64)

  static unsigned long temp;

# define uo_lsb(u64) ((int8_t)(_BitScanForward64(&temp, u64) ? temp : -1))
# define uo_msb(u64) ((int8_t)(_BitScanReverse64(&temp, u64) ? temp : -1))

# define uo_popcount __popcnt64

#elif defined(__has_builtin)

  // uo_popcount
# if __has_builtin(__builtin_popcountll)
#   define uo_popcount __builtin_popcountll
# else
  int uo_popcount(uint64_t u64);
# endif
  // END - uo_popcount

  // uo_lsb && uo_lsb
# if __has_builtin(__builtin_ffsll) && __has_builtin(__builtin_clzll)
#   define uo_lsb(u64) ((int8_t)((int8_t)__builtin_ffsll(u64) - 1))
#   define uo_msb(u64) ((int8_t)((u64) ? (63 - __builtin_clzll((uint64_t)(u64))) : -1))
# else

# endif

#else

  //uint8_t uo_popcount(uint64_t u64);
  //int8_t uo_lsb(uint64_t u64);
  //int8_t uo_msb(uint64_t u64);

#endif


#if defined(__AVX__)

#include <immintrin.h>

# define uo_bzhi _bzhi_u64

// ~a & b)
# define uo_andn _andn_u64

// (a - 1) & a
# define uo_blsr _blsr_u64


#endif


#if !defined(uo_bzhi) && !defined(__uo_bzhi__defined)

# define uo_bzhi(u64, n) (((u64) << (n)) >> (n))

#endif

#if !defined(uo_bzlo) && !defined(__uo_bzlo__defined)

# define uo_bzlo(u64, n) (((u64) >> (n)) << (n))

#endif

  uint64_t uo_rand();
  void uo_rand_init(uint64_t seed);

#ifdef __cplusplus
}
#endif

#endif
