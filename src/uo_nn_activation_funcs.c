#include "uo_nn.h"

#include <math.h>

const uo_nn_activation_function_param activation_null = {0};

uo_avx_float uo_nn_activation_function_relu(__m256 avx_float)
{
  __m256 zeros = _mm256_setzero_ps();
  return _mm256_max_ps(avx_float, zeros);
}

uo_avx_float uo_nn_activation_function_relu_d(__m256 avx_float)
{
  __m256 zeros = _mm256_setzero_ps();
  __m256 mask = _mm256_cmp_ps(avx_float, zeros, _CMP_GT_OQ);
  __m256 ones = _mm256_set1_ps(1.0);
  return _mm256_max_ps(mask, ones);
}

const uo_nn_activation_function_param activation_relu = {
  .f = uo_nn_activation_function_relu,
  .df = uo_nn_activation_function_relu_d
};

uo_avx_float uo_nn_activation_function_sigmoid(__m256 avx_float)
{
  float f[uo_floats_per_avx_float];
  _mm256_storeu_ps(f, avx_float);

  for (size_t i = 0; i < uo_floats_per_avx_float; ++i)
  {
    f[i] = 1.0 / (1.0 + expf(-f[i]));
  }

  return _mm256_loadu_ps(f);
}

uo_avx_float uo_nn_activation_function_sigmoid_d(__m256 avx_float)
{
  __m256 sigmoid = uo_nn_activation_function_sigmoid(avx_float);
  __m256 ones = _mm256_set1_ps(1.0);
  __m256 sub = _mm256_sub_ps(ones, sigmoid);
  return _mm256_mul_ps(sigmoid, sub);
}

const uo_nn_activation_function_param activation_sigmoid = {
  .f = uo_nn_activation_function_sigmoid,
  .df = uo_nn_activation_function_sigmoid_d
};

uo_avx_float uo_nn_activation_function_swish(__m256 avx_float)
{
  __m256 sigmoid = uo_nn_activation_function_sigmoid(avx_float);
  return _mm256_mul_ps(avx_float, sigmoid);
}

uo_avx_float uo_nn_activation_function_swish_d(__m256 avx_float)
{
  __m256 sigmoid = uo_nn_activation_function_sigmoid(avx_float);
  __m256 swish = _mm256_mul_ps(avx_float, sigmoid);
  __m256 ones = _mm256_set1_ps(1.0);
  __m256 sub = _mm256_sub_ps(ones, swish);
  __m256 mul = _mm256_mul_ps(swish, sub);
  return _mm256_add_ps(swish, mul);
}

const uo_nn_activation_function_param activation_swish = {
  .f = uo_nn_activation_function_swish,
  .df = uo_nn_activation_function_swish_d
};

const uo_nn_activation_function_param *uo_nn_get_activation_function_by_name(const char *activation_function_name)
{
  if (activation_function_name == NULL)
  {
    return &activation_null;
  }

  if (strcmp(activation_function_name, "sigmoid") == 0)
  {
    return &activation_sigmoid;
  }

  if (strcmp(activation_function_name, "relu") == 0)
  {
    return &activation_relu;
  }

  if (strcmp(activation_function_name, "swish") == 0)
  {
    return &activation_swish;
  }

  return NULL;
}

const char *uo_nn_get_activation_function_name(uo_nn_activation_function *f)
{
  if (f == NULL)
  {
    return NULL;
  }

  if (f == activation_sigmoid.f)
  {
    return "sigmoid";
  }

  if (f == activation_relu.f)
  {
    return "relu";
  }

  if (f == activation_swish.f)
  {
    return "swish";
  }

  return NULL;
}
