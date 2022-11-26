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
#include <stdio.h>

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

  static inline float uo_dotproduct_mask_ps(const uint32_t *a_mask, const float *b, size_t n)
  {
    size_t nb = n / uo_floats_per_avx_float;
    size_t reminder = n % uo_floats_per_avx_float;
    uo_avx_float sum = _mm256_setzero_ps();

    for (size_t i = 0; i < nb; ++i)
    {
      __m256i _a_mask = _mm256_lddqu_si256((void *)(a_mask + i * uo_floats_per_avx_float));
      uo_avx_float _b = _mm256_loadu_ps(b + i * uo_floats_per_avx_float);
      uo_avx_float mul = _mm256_and_ps(_mm256_castsi256_ps(_a_mask), _b);
      sum = _mm256_add_ps(sum, mul);
    }

    __m256i mask = _mm256_cmpgt_epi32(_mm256_set1_epi32(reminder), _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0));
    __m256i _a_mask = _mm256_maskload_epi32((void *)(a_mask + nb * uo_floats_per_avx_float), mask);
    uo_avx_float _b = _mm256_maskload_ps(b + nb * uo_floats_per_avx_float, mask);
    uo_avx_float mul = _mm256_and_ps(_mm256_castsi256_ps(_a_mask), _b);
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

  static inline void uo_vec_mapfunc_mul_ps(float *a, float *b, float *dst, size_t n, uo_mm256_ps_function *function)
  {
    size_t nb = n / uo_floats_per_avx_float;
    size_t reminder = n % uo_floats_per_avx_float;

    for (size_t i = 0; i < nb; ++i)
    {
      uo_avx_float _a = _mm256_loadu_ps(a + i * uo_floats_per_avx_float);
      uo_avx_float res = function(_a);
      uo_avx_float _b = _mm256_loadu_ps(b + i * uo_floats_per_avx_float);
      uo_avx_float mul = _mm256_mul_ps(res, _b);
      _mm256_storeu_ps(dst + i * uo_floats_per_avx_float, mul);
    }

    __m256i mask = _mm256_cmpgt_epi32(_mm256_set1_epi32(reminder), _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0));
    uo_avx_float _a = _mm256_maskload_ps(a + nb * uo_floats_per_avx_float, mask);
    uo_avx_float res = function(_a);
    uo_avx_float _b = _mm256_maskload_ps(b + nb * uo_floats_per_avx_float, mask);
    uo_avx_float mul = _mm256_mul_ps(res, _b);
    _mm256_maskstore_ps(dst + nb * uo_floats_per_avx_float, mask, mul);
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

  // C = AB, C is m x n matrix, k is "other" dimension, A is split into six sub matrices:
  // - two equal sized mask matrices
  // - two equal sized float matrices
  // - one mask matrix
  // - one float matrix
  static inline void uo_matmul_position_ps(
    const uint32_t *own_mask, const uint32_t *enemy_mask, size_t k_half_mask,
    const float *own_floats, const float *enemy_floats, size_t k_half_floats,
    const uint32_t *shared_mask, size_t k_shared_mask,
    const float *shared_floats, size_t k_shared_floats,
    const float *B_t, float *C, size_t m, size_t n, int offset_C, int offset_B)
  {
    size_t k = k_half_mask * 2 + k_half_floats * 2 + k_shared_mask + k_shared_floats;

    for (size_t i = 0; i < m; ++i)
    {
      for (size_t j = 0; j < n; ++j)
      {
        const float *b = B_t + j * (k + offset_B);
        float dot_own_mask = uo_dotproduct_mask_ps(own_mask + i * k_half_mask, b, k_half_mask);

        b += k_half_mask;
        float dot_own_floats = uo_dotproduct_ps(own_floats + i * k_half_floats, b, k_half_floats);

        b += k_half_floats;
        float dot_enemy_mask = uo_dotproduct_mask_ps(enemy_mask + i * k_half_mask, b, k_half_mask);

        b += k_half_mask;
        float dot_enemy_floats = uo_dotproduct_ps(enemy_floats + i * k_half_floats, b, k_half_floats);

        b += k_half_floats;
        float dot_shared_mask = uo_dotproduct_mask_ps(shared_mask + i * k_shared_mask, b, k_shared_mask);

        b += k_shared_mask;
        float dot_shared_floats = uo_dotproduct_ps(shared_floats + i * k_shared_floats, b, k_shared_floats);

        C[i * (n + offset_C) + j] = dot_own_mask + dot_own_floats + dot_enemy_mask + dot_enemy_floats + dot_shared_mask + dot_shared_floats;
      }
    }
  }

  // C = AB, C is m x n matrix, k is "other" dimension
  static inline void uo_matmul_ps(const float *A, const float *B_t, float *C, size_t m, size_t n, size_t k, int offset_C, int offset_A, int offset_B)
  {
    for (size_t i = 0; i < m; ++i)
    {
      for (size_t j = 0; j < n; ++j)
      {
        C[i * (n + offset_C) + j] = uo_dotproduct_ps(A + i * (k + offset_A), B_t + j * (k + offset_B), k);
      }
    }
  }

  // C_t = B_t * A_t, C is m x n matrix, k is "other" dimension
  static inline void uo_matmul_t_ps(const float *A_t, const float *B, float *C_t, size_t m, size_t n, size_t k, int offset_C, int offset_A, int offset_B)
  {
    memset(C_t, 0, m * (n + offset_C) * sizeof(float));

    size_t mb = m / uo_floats_per_avx_float;
    size_t reminder = m % uo_floats_per_avx_float;

    __m256i mask = _mm256_cmpgt_epi32(_mm256_set1_epi32(reminder), _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0));

    for (size_t _k = 0; _k < k; ++_k)
    {
      for (size_t j = 0; j < n; ++j)
      {
        uo_avx_float b = _mm256_set1_ps(B[_k * (n + offset_B) + j]);

        for (size_t ii = 0; ii < mb; ++ii)
        {
          uo_avx_float a = _mm256_loadu_ps(A_t + _k * (m + offset_A) + ii * uo_floats_per_avx_float);
          uo_avx_float mul = _mm256_mul_ps(a, b);

          float *c = C_t + j * m + ii * uo_floats_per_avx_float;
          uo_avx_float _c = _mm256_loadu_ps(c);
          uo_avx_float add = _mm256_add_ps(_c, mul);
          _mm256_storeu_ps(c, add);
        }

        uo_avx_float a = _mm256_maskload_ps(A_t + _k * (m + offset_A) + mb * uo_floats_per_avx_float, mask);
        uo_avx_float mul = _mm256_mul_ps(a, b);

        float *c = C_t + j * m + mb * uo_floats_per_avx_float;
        uo_avx_float _c = _mm256_maskload_ps(c, mask);
        uo_avx_float add = _mm256_add_ps(_c, mul);
        _mm256_maskstore_ps(c, mask, add);
      }
    }
  }

  static inline void uo_gemm_nn(size_t m, size_t n, size_t k, float alpha,
    float *A, size_t lda,
    float *B, size_t ldb,
    float *C, size_t ldc)
  {
#pragma omp parallel for
    for (size_t i = 0; i < m; ++i)
    {
      for (size_t l = 0; l < k; ++l)
      {
        register float A_PART = alpha * A[i * lda + l];
        for (size_t j = 0; j < n; ++j)
        {
          C[i * ldc + j] += A_PART * B[l * ldb + j];
        }
      }
    }
  }

  static inline void uo_gemm_nt(size_t m, size_t n, size_t k, float alpha,
    float *A, size_t lda,
    float *B, size_t ldb,
    float *C, size_t ldc)
  {
#pragma omp parallel for
    for (size_t i = 0; i < m; ++i)
    {
      for (size_t j = 0; j < n; ++j)
      {
        register float sum = 0;
        for (size_t l = 0; l < k; ++l)
        {
          sum += alpha * A[i * lda + l] * B[j * ldb + l];
        }
        C[i * ldc + j] += sum;
      }
    }
  }

  static inline void uo_gemm_tn(size_t m, size_t n, size_t k, float alpha,
    float *A, size_t lda,
    float *B, size_t ldb,
    float *C, size_t ldc)
  {
#pragma omp parallel for
    for (size_t i = 0; i < m; ++i)
    {
      for (size_t l = 0; l < k; ++l)
      {
        register float A_PART = alpha * A[l * lda + i];
        for (size_t j = 0; j < n; ++j)
        {
          C[i * ldc + j] += A_PART * B[l * ldb + j];
        }
      }
    }
  }

  static inline void uo_gemm_tt(size_t m, size_t n, size_t k, float alpha,
    float *A, size_t lda,
    float *B, size_t ldb,
    float *C, size_t ldc)
  {
#pragma omp parallel for
    for (size_t i = 0; i < m; ++i)
    {
      for (size_t j = 0; j < n; ++j)
      {
        register float sum = 0;
        for (size_t l = 0; l < k; ++l)
        {
          sum += alpha * A[i + l * lda] * B[l + j * ldb];
        }
        C[i * ldc + j] += sum;
      }
    }
  }

  // see: https://en.wikipedia.org/wiki/Basic_Linear_Algebra_Subprograms#Level_3
  static inline void uo_gemm(bool ta, bool tb, size_t m, size_t n, size_t k,
    float alpha,
    float *A, size_t lda,
    float *B, size_t ldb,
    float beta,
    float *C, size_t ldc)
  {
    for (size_t i = 0; i < m; ++i)
    {
      for (size_t j = 0; j < n; ++j)
      {
        C[i * ldc + j] *= beta;
      }
    }

    if (!ta && !tb)
      uo_gemm_nn(m, n, k, alpha, A, lda, B, ldb, C, ldc);
    else if (ta && !tb)
      uo_gemm_tn(m, n, k, alpha, A, lda, B, ldb, C, ldc);
    else if (!ta && tb)
      uo_gemm_nt(m, n, k, alpha, A, lda, B, ldb, C, ldc);
    else
      uo_gemm_tt(m, n, k, alpha, A, lda, B, ldb, C, ldc);
  }

  bool uo_test_matmul(char *test_data_dir);

  void uo_print_matrix(FILE *const fp, float *A, size_t m, size_t n);

  char *uo_parse_matrix(char *ptr, float **data, size_t *m, size_t *n);

#ifdef __cplusplus
}
#endif

#endif
