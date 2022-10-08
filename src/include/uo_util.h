#ifndef UO_UTIL_H
#define UO_UTIL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_macro.h"

#include <inttypes.h>
#include <stdlib.h>

#if defined(_MSC_VER)

#include <Windows.h>

#define uo_alloca _alloca

#define uo_strtok strtok_s

#else

# ifndef uo_alloca
# include <alloca.h>
#define uo_alloca alloca
#endif // !uo_alloca

#include <string.h>
#define uo_strtok strtok_r


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

#pragma region uo_min

  static inline double _uo_min_lf(double x, double y)
  {
    return x > y ? y : x;
  }
  static inline float _uo_min_f(float x, float y)
  {
    return x > y ? y : x;
  }
  static inline char _uo_min_c(char x, char y)
  {
    return x > y ? y : x;
  }
  static inline unsigned char _uo_min_uc(unsigned char x, unsigned char y)
  {
    return x > y ? y : x;
  }
  static inline short _uo_min_s(short x, short y)
  {
    return x > y ? y : x;
  }
  static inline unsigned short _uo_min_us(unsigned short x, unsigned short y)
  {
    return x > y ? y : x;
  }
  static inline int _uo_min_i(int x, int y)
  {
    return x > y ? y : x;
  }
  static inline unsigned int _uo_min_u(unsigned int x, unsigned int y)
  {
    return x > y ? y : x;
  }
  static inline long _uo_min_l(long x, long y)
  {
    return x > y ? y : x;
  }
  static inline unsigned long _uo_min_ul(unsigned long x, unsigned long y)
  {
    return x > y ? y : x;
  }
  static inline long long _uo_min_ll(long long x, long long y)
  {
    return x > y ? y : x;
  }
  static inline unsigned long long _uo_min_ull(unsigned long long x, unsigned long long y)
  {
    return x > y ? y : x;
  }


#define uo_min(x, y) \
  _Generic(((x + y)), \
                double: _uo_min_lf,  \
                 float: _uo_min_f,   \
                  char: _uo_min_c,   \
         unsigned char: _uo_min_uc,  \
                 short: _uo_min_s,   \
        unsigned short: _uo_min_us,  \
                   int: _uo_min_i,   \
          unsigned int: _uo_min_u,   \
                  long: _uo_min_l,   \
         unsigned long: _uo_min_ul,  \
             long long: _uo_min_ll,  \
    unsigned long long: _uo_min_ull  \
  )(x, y)

#pragma endregion

#pragma region uo_max

  static inline double _uo_max_lf(double x, double y)
  {
    return x < y ? y : x;
  }
  static inline float _uo_max_f(float x, float y)
  {
    return x < y ? y : x;
  }
  static inline char _uo_max_c(char x, char y)
  {
    return x < y ? y : x;
  }
  static inline unsigned char _uo_max_uc(unsigned char x, unsigned char y)
  {
    return x < y ? y : x;
  }
  static inline short _uo_max_s(short x, short y)
  {
    return x < y ? y : x;
  }
  static inline unsigned short _uo_max_us(unsigned short x, unsigned short y)
  {
    return x < y ? y : x;
  }
  static inline int _uo_max_i(int x, int y)
  {
    return x < y ? y : x;
  }
  static inline unsigned int _uo_max_u(unsigned int x, unsigned int y)
  {
    return x < y ? y : x;
  }
  static inline long _uo_max_l(long x, long y)
  {
    return x < y ? y : x;
  }
  static inline unsigned long _uo_max_ul(unsigned long x, unsigned long y)
  {
    return x < y ? y : x;
  }
  static inline long long _uo_max_ll(long long x, long long y)
  {
    return x < y ? y : x;
  }
  static inline unsigned long long _uo_max_ull(unsigned long long x, unsigned long long y)
  {
    return x < y ? y : x;
  }


#define uo_max(x, y) \
  _Generic(((x + y)), \
                double: _uo_max_lf,  \
                 float: _uo_max_f,   \
                  char: _uo_max_c,   \
         unsigned char: _uo_max_uc,  \
                 short: _uo_max_s,   \
        unsigned short: _uo_max_us,  \
                   int: _uo_max_i,   \
          unsigned int: _uo_max_u,   \
                  long: _uo_max_l,   \
         unsigned long: _uo_max_ul,  \
             long long: _uo_max_ll,  \
    unsigned long long: _uo_max_ull  \
  )(x, y)

#pragma endregion

  static inline uint64_t uo_rand_u64()
  {
    uint64_t r = 0;

    for (int i = 0; i < 64; i += 15) {
      r <<= 15;
      r ^= (unsigned)rand();
    }

    return r;
  }

  static inline float uo_rand_percent()
  {
    return (float)rand() / (float)RAND_MAX;
  }

  static inline float uo_rand_percent_excl()
  {
    return (float)rand() / (float)(RAND_MAX + 1);
  }

  static inline float uo_rand_between(float min, float max)
  {
    float range = max - min;
    return uo_rand_percent() * range + min;
  }

  static inline float uo_rand_between_excl(float min, float max_ex)
  {
    float range = max_ex - min;
    return uo_rand_percent_excl() * range + min;
  }

  static inline void uo_rand_init(uint64_t seed)
  {
    srand(seed);
  }

  static inline char *_uo_strcat(size_t count, ...)
  {
    size_t size = 0;
    size_t *lens = uo_alloca(count * sizeof(size_t));
    va_list args;

    va_start(args, count);
    for (size_t i = 0; i < count; ++i)
    {
      char *str = va_arg(args, char *);
      size_t len = strlen(str);
      size += len;
      lens[i] = len;
    }
    va_end(args);

    char *buf = malloc(size + 1);
    char *ptr = buf;

    va_start(args, count);
    for (size_t i = 0; i < count; ++i)
    {
      char *str = va_arg(args, char *);
      size_t len = lens[i];
      memcpy(ptr, str, len);
      ptr += len;
    }
    va_end(args);

    *ptr = '\0';
    return buf;
  }

#define uo_strcat(...) _uo_strcat(uo_narg(__VA_ARGS__), __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
