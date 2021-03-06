#ifndef UO_UTIL_H
#define UO_UTIL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <inttypes.h>
#include <stdlib.h>

#if defined(_MSC_VER)

# include <intrin.h>
# pragma intrinsic(_BitScanForward64, _BitScanReverse64)

  static unsigned long temp;

  //# define uo_lsb(u64) ((int8_t)(_BitScanForward64(&temp, u64) ? temp : -1))
  //# define uo_msb(u64) ((int8_t)(_BitScanReverse64(&temp, u64) ? temp : -1))

#elif defined(__has_builtin)

  // uo_popcnt
# if __has_builtin(__builtin_popcountll)
#   define uo_popcnt __builtin_popcountll
# else
  int uo_popcnt(uint64_t u64);
# endif
  // END - uo_popcnt

  // uo_lsb && uo_lsb
# if __has_builtin(__builtin_ffsll) && __has_builtin(__builtin_clzll)
#   define uo_lsb(u64) ((int8_t)((int8_t)__builtin_ffsll(u64) - 1))
#   define uo_msb(u64) ((int8_t)((u64) ? (63 - __builtin_clzll((uint64_t)(u64))) : -1))
# else

# endif

#else

  //uint8_t uo_popcnt(uint64_t u64);
  //int8_t uo_lsb(uint64_t u64);
  //int8_t uo_msb(uint64_t u64);

#endif


#if defined(__AVX2__)

#include <immintrin.h>

// ~a & b
# define uo_andn _andn_u64

// (a - 1) & a
# define uo_blsr _blsr_u64

// a & -a
# define uo_blsi _blsi_u64

# define uo_tzcnt _tzcnt_u64

# define uo_lzcnt _lzcnt_u64

# define uo_lsb(u64) ((int8_t)(u64 ? (int8_t)uo_tzcnt(u64) : -1))

# define uo_msb(u64) ((int8_t)(63 - (int8_t)uo_lzcnt(u64)))

# define uo_pext _pext_u64

# define uo_pdep _pdep_u64

# define uo_bzhi _bzhi_u64

# define uo_bzlo(u64, i) (uo_andn(_bzhi_u64(-1, i), u64))

# if defined(__INTEL_LLVM_COMPILER)

#  define uo_popcnt _popcnt64

#  define uo_bswap _bswap64

# else

#  define uo_popcnt __popcnt64

#  define uo_bswap _byteswap_uint64

# endif

#endif


#if !defined(uo_bzhi) && !defined(__uo_bzhi__defined)

# define uo_bzhi(u64, n) (((u64) << (n)) >> (n))

#endif

#if !defined(uo_bzlo) && !defined(__uo_bzlo__defined)

# define uo_bzlo(u64, n) (((u64) >> (n)) << (n))

#endif

  static inline uint64_t uo_rand()
  {
    uint64_t r = 0;

    for (int i = 0; i < 64; i += 15) {
      r <<= 15;
      r ^= (unsigned)rand();
    }

    return r;
  }

  static inline void uo_rand_init(uint64_t seed)
  {
    srand(seed);
  }

#ifdef __cplusplus
  }
#endif

#endif
