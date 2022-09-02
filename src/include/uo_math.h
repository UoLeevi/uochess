#ifndef UO_MATH_H
#define UO_MATH_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_util.h"

#include <immintrin.h>
#include <stdlib.h>
#include <stdbool.h>

#define uo_avx_float __m256
#define uo_floats_per_avx_float (sizeof(uo_avx_float) / sizeof(float))

  typedef uo_avx_float uo_mm256_ps_function(uo_avx_float avx_float);

  static inline size_t uo_mm256_roundup_size(size_t size)
  {
    return (size + uo_floats_per_avx_float - 1) & -uo_floats_per_avx_float;
  }

  static inline float uo_mm256_hsum_ps(__m256 v) {
    __m128 vlow = _mm256_castps256_ps128(v);
    __m128 vhigh = _mm256_extractf128_ps(v, 1);
    vlow = _mm_add_ps(vlow, vhigh);
    __m128 shuf = _mm_movehdup_ps(vlow);
    __m128 sums = _mm_add_ps(vlow, shuf);
    shuf = _mm_movehl_ps(shuf, sums);
    sums = _mm_add_ss(sums, shuf);
    return _mm_cvtss_f32(sums);
  }

  static inline float uo_dotproduct_ps(const float *a, const float *b, size_t n)
  {
    size_t block_count = n / uo_floats_per_avx_float;
    uo_avx_float sum = _mm256_setzero_ps();

    for (size_t i = 0; i < block_count; ++i)
    {
      uo_avx_float _a = _mm256_loadu_ps(a + i * uo_floats_per_avx_float);
      uo_avx_float _b = _mm256_loadu_ps(b + i * uo_floats_per_avx_float);
      uo_avx_float mul = _mm256_mul_ps(_a, _b);
      sum = _mm256_add_ps(sum, mul);
    }

    return uo_mm256_hsum_ps(sum);
  }

  static inline void uo_vecadd_ps(float *a, float *b, float *c, size_t n)
  {
    size_t block_count = n / uo_floats_per_avx_float;
    uo_avx_float sum = _mm256_setzero_ps();

    for (size_t i = 0; i < block_count; ++i)
    {
      uo_avx_float _a = _mm256_loadu_ps(a + i * uo_floats_per_avx_float);
      uo_avx_float _b = _mm256_loadu_ps(b + i * uo_floats_per_avx_float);
      uo_avx_float add = _mm256_add_ps(_a, _b);
      _mm256_store_ps(c + i * uo_floats_per_avx_float, add);
    }
  }

  static inline void uo_vecsub_ps(float *a, float *b, float *c, size_t n)
  {
    size_t block_count = n / uo_floats_per_avx_float;
    uo_avx_float sum = _mm256_setzero_ps();

    for (size_t i = 0; i < block_count; ++i)
    {
      uo_avx_float _a = _mm256_loadu_ps(a + i * uo_floats_per_avx_float);
      uo_avx_float _b = _mm256_loadu_ps(b + i * uo_floats_per_avx_float);
      uo_avx_float sub = _mm256_sub_ps(_a, _b);
      _mm256_store_ps(c + i * uo_floats_per_avx_float, sub);
    }
  }

  static inline void uo_vec_mapfunc_ps(float *v, size_t n, uo_mm256_ps_function *function)
  {
    size_t block_count = n / uo_floats_per_avx_float;

    for (size_t i = 0; i < block_count; ++i)
    {
      uo_avx_float _v = _mm256_loadu_ps(v + i * uo_floats_per_avx_float);
      uo_avx_float res = function(_v);
      _mm256_store_ps(v + i * uo_floats_per_avx_float, res);
    }
  }

  static inline void uo_transpose_ps(const float *A, float *At, size_t m, size_t n)
  {
    for (size_t i = 0; i < m; ++i)
    {
      for (size_t j = 0; j < n; ++j)
      {
        At[j * m + i] = A[i * n + j];
      }
    }
  }

  // C = AB, C is n x m matrix, k is "other" dimension
  static inline void uo_matmul_ps(const float *A, const float *Bt, float *C, size_t m, size_t n, size_t k)
  {
    for (size_t i = 0; i < m; ++i)
    {
      for (size_t j = 0; j < n; ++j)
      {
        C[i * n + j] = uo_dotproduct_ps(A + i * k, Bt + j * k, k);
      }
    }
  }

  bool uo_test_matmul();

#ifdef __cplusplus
}
#endif

#endif
