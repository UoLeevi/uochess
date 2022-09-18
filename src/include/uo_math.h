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
  typedef uo_avx_float uo_mm256_ps_function2(uo_avx_float avx_float0, uo_avx_float avx_float1);

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
    size_t nb = n / uo_floats_per_avx_float;
    size_t reminder = n % uo_floats_per_avx_float;
    uo_avx_float sum = _mm256_setzero_ps();

    for (size_t i = 0; i < nb; ++i)
    {
      uo_avx_float _a = _mm256_loadu_ps(a + i * uo_floats_per_avx_float);
      uo_avx_float _b = _mm256_loadu_ps(b + i * uo_floats_per_avx_float);
      uo_avx_float mul = _mm256_mul_ps(_a, _b);
      sum = _mm256_add_ps(sum, mul);
    }

    __m256i mask = _mm256_cmpgt_epi32(_mm256_set1_epi32(reminder), _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0));
    uo_avx_float _a = _mm256_maskload_ps(a + nb * uo_floats_per_avx_float, mask);
    uo_avx_float _b = _mm256_maskload_ps(b + nb * uo_floats_per_avx_float, mask);
    uo_avx_float mul = _mm256_mul_ps(_a, _b);
    sum = _mm256_add_ps(sum, mul);

    return uo_mm256_hsum_ps(sum);
  }

  static inline float uo_vec_mean_ps(float *a, size_t n)
  {
    size_t nb = n / uo_floats_per_avx_float;
    size_t reminder = n % uo_floats_per_avx_float;
    uo_avx_float reci = _mm256_set1_ps(1.0f / (float)n);
    uo_avx_float avg = _mm256_setzero_ps();

    for (size_t i = 0; i < nb; ++i)
    {
      uo_avx_float _a = _mm256_loadu_ps(a + i * uo_floats_per_avx_float);
      uo_avx_float mul = _mm256_mul_ps(_a, reci);
      avg = _mm256_add_ps(avg, mul);
    }

    __m256i mask = _mm256_cmpgt_epi32(_mm256_set1_epi32(reminder), _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0));
    uo_avx_float _a = _mm256_maskload_ps(a + nb * uo_floats_per_avx_float, mask);
    uo_avx_float mul = _mm256_mul_ps(_a, reci);
    avg = _mm256_add_ps(avg, mul);

    return uo_mm256_hsum_ps(avg);
  }

  static inline void uo_vec_set1_ps(float *a, float value, size_t n)
  {
    size_t nb = n / uo_floats_per_avx_float;
    size_t reminder = n % uo_floats_per_avx_float;
    uo_avx_float _value = _mm256_set1_ps(value);

    for (size_t i = 0; i < nb; ++i)
    {
      _mm256_storeu_ps(a + i * uo_floats_per_avx_float, _value);
    }

    __m256i mask = _mm256_cmpgt_epi32(_mm256_set1_epi32(reminder), _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0));
    _mm256_maskstore_ps(a + nb * uo_floats_per_avx_float, mask, _value);
  }

  static inline void uo_vec_add_ps(float *a, float *b, float *c, size_t n)
  {
    size_t nb = n / uo_floats_per_avx_float;
    size_t reminder = n % uo_floats_per_avx_float;

    for (size_t i = 0; i < nb; ++i)
    {
      uo_avx_float _a = _mm256_loadu_ps(a + i * uo_floats_per_avx_float);
      uo_avx_float _b = _mm256_loadu_ps(b + i * uo_floats_per_avx_float);
      uo_avx_float add = _mm256_add_ps(_a, _b);
      _mm256_storeu_ps(c + i * uo_floats_per_avx_float, add);
    }

    __m256i mask = _mm256_cmpgt_epi32(_mm256_set1_epi32(reminder), _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0));
    uo_avx_float _a = _mm256_maskload_ps(a + nb * uo_floats_per_avx_float, mask);
    uo_avx_float _b = _mm256_maskload_ps(b + nb * uo_floats_per_avx_float, mask);
    uo_avx_float add = _mm256_add_ps(_a, _b);
    _mm256_maskstore_ps(c + nb * uo_floats_per_avx_float, mask, add);
  }

  static inline void uo_vec_sub_ps(float *a, float *b, float *c, size_t n)
  {
    size_t nb = n / uo_floats_per_avx_float;
    size_t reminder = n % uo_floats_per_avx_float;

    for (size_t i = 0; i < nb; ++i)
    {
      uo_avx_float _a = _mm256_loadu_ps(a + i * uo_floats_per_avx_float);
      uo_avx_float _b = _mm256_loadu_ps(b + i * uo_floats_per_avx_float);
      uo_avx_float sub = _mm256_sub_ps(_a, _b);
      _mm256_storeu_ps(c + i * uo_floats_per_avx_float, sub);
    }

    __m256i mask = _mm256_cmpgt_epi32(_mm256_set1_epi32(reminder), _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0));
    uo_avx_float _a = _mm256_maskload_ps(a + nb * uo_floats_per_avx_float, mask);
    uo_avx_float _b = _mm256_maskload_ps(b + nb * uo_floats_per_avx_float, mask);
    uo_avx_float sub = _mm256_sub_ps(_a, _b);
    _mm256_maskstore_ps(c + nb * uo_floats_per_avx_float, mask, sub);
  }

  static inline void uo_vec_mul_ps(float *a, float *b, float *c, size_t n)
  {
    size_t nb = n / uo_floats_per_avx_float;
    size_t reminder = n % uo_floats_per_avx_float;

    for (size_t i = 0; i < nb; ++i)
    {
      uo_avx_float _a = _mm256_loadu_ps(a + i * uo_floats_per_avx_float);
      uo_avx_float _b = _mm256_loadu_ps(b + i * uo_floats_per_avx_float);
      uo_avx_float mul = _mm256_mul_ps(_a, _b);
      _mm256_storeu_ps(c + i * uo_floats_per_avx_float, mul);
    }

    __m256i mask = _mm256_cmpgt_epi32(_mm256_set1_epi32(reminder), _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0));
    uo_avx_float _a = _mm256_maskload_ps(a + nb * uo_floats_per_avx_float, mask);
    uo_avx_float _b = _mm256_maskload_ps(b + nb * uo_floats_per_avx_float, mask);
    uo_avx_float mul = _mm256_mul_ps(_a, _b);
    _mm256_maskstore_ps(c + nb * uo_floats_per_avx_float, mask, mul);
  }

  static inline void uo_vec_mul1_ps(float *a, float scalar, float *c, size_t n)
  {
    size_t nb = n / uo_floats_per_avx_float;
    size_t reminder = n % uo_floats_per_avx_float;
    uo_avx_float _scalar = _mm256_set1_ps(scalar);

    for (size_t i = 0; i < nb; ++i)
    {
      uo_avx_float _a = _mm256_loadu_ps(a + i * uo_floats_per_avx_float);
      uo_avx_float mul = _mm256_mul_ps(_a, _scalar);
      _mm256_storeu_ps(c + i * uo_floats_per_avx_float, mul);
    }

    __m256i mask = _mm256_cmpgt_epi32(_mm256_set1_epi32(reminder), _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0));
    uo_avx_float _a = _mm256_maskload_ps(a + nb * uo_floats_per_avx_float, mask);
    uo_avx_float mul = _mm256_mul_ps(_a, _scalar);
    _mm256_maskstore_ps(c + nb * uo_floats_per_avx_float, mask, mul);
  }

  static inline void uo_vec_mapfunc_ps(float *v, float *dst, size_t n, uo_mm256_ps_function *function)
  {
    size_t nb = n / uo_floats_per_avx_float;
    size_t reminder = n % uo_floats_per_avx_float;

    for (size_t i = 0; i < nb; ++i)
    {
      uo_avx_float _v = _mm256_loadu_ps(v + i * uo_floats_per_avx_float);
      uo_avx_float res = function(_v);
      _mm256_storeu_ps(dst + i * uo_floats_per_avx_float, res);
    }

    __m256i mask = _mm256_cmpgt_epi32(_mm256_set1_epi32(reminder), _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0));
    uo_avx_float _v = _mm256_maskload_ps(v + nb * uo_floats_per_avx_float, mask);
    uo_avx_float res = function(_v);
    _mm256_maskstore_ps(dst + nb * uo_floats_per_avx_float, mask, res);
  }

  static inline void uo_vec_map2func_ps(float *v0, float *v1, float *dst, size_t n, uo_mm256_ps_function2 *function)
  {
    size_t nb = n / uo_floats_per_avx_float;
    size_t reminder = n % uo_floats_per_avx_float;

    for (size_t i = 0; i < nb; ++i)
    {
      uo_avx_float _v0 = _mm256_loadu_ps(v0 + i * uo_floats_per_avx_float);
      uo_avx_float _v1 = _mm256_loadu_ps(v1 + i * uo_floats_per_avx_float);
      uo_avx_float res = function(_v0, _v1);
      _mm256_storeu_ps(dst + i * uo_floats_per_avx_float, res);
    }

    __m256i mask = _mm256_cmpgt_epi32(_mm256_set1_epi32(reminder), _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0));
    uo_avx_float _v0 = _mm256_maskload_ps(v0 + nb * uo_floats_per_avx_float, mask);
    uo_avx_float _v1 = _mm256_maskload_ps(v1 + nb * uo_floats_per_avx_float, mask);
    uo_avx_float res = function(_v0, _v1);
    _mm256_maskstore_ps(dst + nb * uo_floats_per_avx_float, mask, res);
  }

  static inline void uo_transpose_ps(const float *A, float *A_t, size_t m, size_t n)
  {
    for (size_t i = 0; i < m; ++i)
    {
      for (size_t j = 0; j < n; ++j)
      {
        A_t[j * m + i] = A[i * n + j];
      }
    }
  }

  // C = AB, C is n x m matrix, k is "other" dimension
  static inline void uo_matmul_ps(const float *A, const float *B_t, float *C, size_t m, size_t n, size_t k, int offset_n_C, int offset_n_A, int offset_n_B)
  {
    for (size_t i = 0; i < m; ++i)
    {
      for (size_t j = 0; j < n; ++j)
      {
        C[i * (n + offset_n_C) + j] = uo_dotproduct_ps(A + i * (k + offset_n_A), B_t + j * (k + offset_n_B), k);
      }
    }
  }

  bool uo_test_matmul(char *test_data_dir);

  void uo_print_matrix(float *A, size_t m, size_t n);

#ifdef __cplusplus
    }
#endif

#endif
