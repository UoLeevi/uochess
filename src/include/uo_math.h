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

  typedef struct uo_matrix
  {
    struct
    {
      size_t rows;
      size_t cols;
    } size;
    uo_avx_float *data;
  } uo_matrix;

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

  static inline float uo_mm256_dotproduct_ps(uo_avx_float *a, uo_avx_float *b, size_t n)
  {
    size_t block_count = n / uo_floats_per_avx_float;
    uo_avx_float sum = _mm256_setzero_ps();

    for (size_t i = 0; i < block_count; ++i)
    {
      uo_avx_float mul = _mm256_mul_ps(a[i], b[i]);
      sum = _mm256_add_ps(sum, mul);
    }

    return uo_mm256_hsum_ps(sum);
  }

  static inline void uo_mm256_load_ps(uo_avx_float *dst, float *src, size_t n)
  {
    size_t block_count = n / uo_floats_per_avx_float;

    for (size_t i = 0; i < block_count; ++i)
    {
      dst[i] = _mm256_loadu_ps(src + i * uo_floats_per_avx_float);
    }
  }

  static inline void uo_mm256_load_transpose_ps(uo_avx_float *dst, float *src, size_t n, size_t size_cols)
  {
    size_t block_count = n / uo_floats_per_avx_float;
    __m256i vindex = _mm256_set_epi32(
      0 * size_cols,
      1 * size_cols,
      2 * size_cols,
      3 * size_cols,
      4 * size_cols,
      5 * size_cols,
      6 * size_cols,
      7 * size_cols
    );

    for (size_t i = 0; i < block_count; ++i)
    {
      dst[i] = _mm256_i32gather_ps(src + i * uo_floats_per_avx_float, vindex, sizeof(float));
    }
  }

  static inline void uo_mm256_store_ps(float *dst, uo_avx_float *src, size_t n)
  {
    size_t block_count = n / uo_floats_per_avx_float;

    for (size_t i = 0; i < block_count; ++i)
    {
      _mm256_storeu_ps(dst + i * uo_floats_per_avx_float, src[i]);
    }
  }

  static inline void uo_mm256_vecadd_ps(uo_avx_float *a, uo_avx_float *b, uo_avx_float *c, size_t n)
  {
    size_t block_count = n / uo_floats_per_avx_float;
    uo_avx_float sum = _mm256_setzero_ps();

    for (size_t i = 0; i < block_count; ++i)
    {
      c[i] = _mm256_add_ps(a[i], b[i]);
    }
  }

  static inline void uo_mm256_vecsub_ps(uo_avx_float *a, uo_avx_float *b, uo_avx_float *c, size_t n)
  {
    size_t block_count = n / uo_floats_per_avx_float;
    uo_avx_float sum = _mm256_setzero_ps();

    for (size_t i = 0; i < block_count; ++i)
    {
      c[i] = _mm256_sub_ps(a[i], b[i]);
    }
  }

  static inline void uo_mm256_vec_mapfunc_ps(uo_avx_float *v, size_t n, uo_mm256_ps_function *function)
  {
    size_t block_count = n / uo_floats_per_avx_float;

    for (size_t i = 0; i < block_count; ++i)
    {
      v[i] = function(v[i]);
    }
  }

  static inline void uo_mm256_matmul2_ps(uo_matrix A, uo_matrix Bt, uo_matrix *C)
  {
    size_t block_count = A.size.cols / uo_floats_per_avx_float;
    float *r = uo_alloca(Bt.size.rows);

    C->size.rows = A.size.rows;
    C->size.cols = Bt.size.rows;

    for (size_t i = 0; i < A.size.rows; ++i)
    {
      uo_avx_float *a = A.data + block_count * i;

      for (size_t j = 0; j < Bt.size.rows; ++j)
      {
        uo_avx_float *b = Bt.data + block_count * j;
        r[j] = uo_mm256_dotproduct_ps(a, b, A.size.cols);
      }

      for (size_t k = 0; k < block_count; ++k)
      {
        C->data[k] = _mm256_loadu_ps(r + k * uo_floats_per_avx_float);
      }
    }
  }

  static inline void uo_mm256_matmul_ps(uo_avx_float *At, size_t A_m, size_t A_n, uo_avx_float *B, size_t B_n, uo_avx_float *C)
  {
    size_t block_count = B_n / uo_floats_per_avx_float;
    float *r = uo_alloca(B_n);

    for (size_t i = 0; i < A_m; ++i)
    {
      for (size_t j = 0; j < B_n; ++j)
      {
        r[j] = uo_mm256_dotproduct_ps(At + A_n * i, B + A_n * j, A_n);
      }

      for (size_t k = 0; k < block_count; ++k)
      {
        C[k] = _mm256_loadu_ps(r + k * uo_floats_per_avx_float);
      }
    }
  }

  bool uo_test_matmul();

#ifdef __cplusplus
}
#endif

#endif
