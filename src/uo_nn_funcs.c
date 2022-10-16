#include "uo_nn.h"

#include <math.h>

const uo_nn_function_param null = { 0 };

uo_avx_float uo_nn_function_relu(__m256 avx_float)
{
  __m256 zeros = _mm256_setzero_ps();
  return _mm256_max_ps(avx_float, zeros);
}

uo_avx_float uo_nn_function_relu_d(__m256 avx_float)
{
  __m256 zeros = _mm256_setzero_ps();
  __m256 mask = _mm256_cmp_ps(avx_float, zeros, _CMP_GT_OQ);
  __m256 ones = _mm256_set1_ps(1.0);
  return _mm256_max_ps(mask, ones);
}

const uo_nn_function_param activation_relu = {
  .activation = {
    .f = uo_nn_function_relu,
    .df = uo_nn_function_relu_d
  },
  .name = "relu"
};

uo_avx_float uo_nn_function_sigmoid(__m256 avx_float)
{
  float f[uo_floats_per_avx_float];
  _mm256_storeu_ps(f, avx_float);

  for (size_t i = 0; i < uo_floats_per_avx_float; ++i)
  {
    f[i] = 1.0 / (1.0 + expf(-f[i]));
  }

  return _mm256_loadu_ps(f);
}

uo_avx_float uo_nn_function_sigmoid_d(__m256 avx_float)
{
  __m256 sigmoid = uo_nn_function_sigmoid(avx_float);
  __m256 ones = _mm256_set1_ps(1.0);
  __m256 sub = _mm256_sub_ps(ones, sigmoid);
  return _mm256_mul_ps(sigmoid, sub);
}

const uo_nn_function_param activation_sigmoid = {
  .activation = {
    .f = uo_nn_function_sigmoid,
    .df = uo_nn_function_sigmoid_d,
  },
  .name = "sigmoid"
};

uo_avx_float uo_nn_function_tanh(__m256 avx_float)
{
  return _mm256_tanh_ps(avx_float);
}

uo_avx_float uo_nn_function_tanh_d(__m256 avx_float)
{
  __m256 tanh = _mm256_tanh_ps(avx_float);
  __m256 sqr = _mm256_mul_ps(tanh, tanh);
  __m256 ones = _mm256_set1_ps(1.0);
  return _mm256_sub_ps(ones, sqr);
}

const uo_nn_function_param activation_tanh = {
  .activation = {
    .f = uo_nn_function_tanh,
    .df = uo_nn_function_tanh_d,
  },
  .name = "tanh"
};

uo_avx_float uo_nn_function_swish(__m256 avx_float)
{
  __m256 sigmoid = uo_nn_function_sigmoid(avx_float);
  return _mm256_mul_ps(avx_float, sigmoid);
}

uo_avx_float uo_nn_function_swish_d(__m256 avx_float)
{
  __m256 sigmoid = uo_nn_function_sigmoid(avx_float);
  __m256 swish = _mm256_mul_ps(avx_float, sigmoid);
  __m256 ones = _mm256_set1_ps(1.0);
  __m256 sub = _mm256_sub_ps(ones, swish);
  __m256 mul = _mm256_mul_ps(swish, sub);
  return _mm256_add_ps(swish, mul);
}

const uo_nn_function_param activation_swish = {
  .activation = {
    .f = uo_nn_function_swish,
    .df = uo_nn_function_swish_d
  },
  .name = "swish"
};

uo_avx_float uo_nn_loss_function_mean_squared_error(uo_avx_float y_true, uo_avx_float y_pred)
{
  __m256 sub = _mm256_sub_ps(y_pred, y_true);
  __m256 mul = _mm256_mul_ps(sub, sub);
  return mul;
}

uo_avx_float uo_nn_loss_function_mean_squared_error_d(uo_avx_float y_true, uo_avx_float y_pred)
{
  return _mm256_sub_ps(y_pred, y_true);
}

uo_nn_function_param loss_mse = {
  .loss = {
    .f = uo_nn_loss_function_mean_squared_error,
    .df = uo_nn_loss_function_mean_squared_error_d
  },
  .name = "loss_mse"
};

uo_avx_float uo_nn_loss_function_mean_absolute_error(uo_avx_float y_true, uo_avx_float y_pred)
{
  __m256 sub = _mm256_sub_ps(y_pred, y_true);
  __m256 a = _mm256_set1_ps(-0.0);
  return _mm256_andnot_ps(a, sub);
}

uo_avx_float uo_nn_loss_function_mean_absolute_error_d(uo_avx_float y_true, uo_avx_float y_pred)
{
  __m256 mask_gt = _mm256_cmp_ps(y_pred, y_true, _CMP_GT_OS);
  __m256 ones = _mm256_set1_ps(1.0f);
  ones = _mm256_and_ps(ones, mask_gt);
  __m256 mask_lt = _mm256_cmp_ps(y_pred, y_true, _CMP_LT_OS);
  __m256 minus_ones = _mm256_set1_ps(-1.0f);
  minus_ones = _mm256_and_ps(minus_ones, mask_lt);
  return _mm256_or_ps(ones, minus_ones);
}

uo_nn_function_param loss_mae = {
  .loss = {
    .f = uo_nn_loss_function_mean_absolute_error,
    .df = uo_nn_loss_function_mean_absolute_error_d
  },
  .name = "loss_mae"
};

uo_nn_function_param sigmoid_loss_mae = {
  .activation = {
    .f = uo_nn_function_sigmoid,
    .df = uo_nn_function_sigmoid_d,
  },
  .loss = {
    .f = uo_nn_loss_function_mean_absolute_error,
    .df = uo_nn_loss_function_mean_absolute_error_d
  },
  .name = "sigmoid_loss_mae"
};

uo_avx_float uo_nn_loss_function_binary_cross_entropy(uo_avx_float y_true, uo_avx_float y_pred)
{
  __m256 ln_y_pred = _mm256_log_ps(y_pred);
  __m256 mul1 = _mm256_mul_ps(y_true, ln_y_pred);

  __m256 ones = _mm256_set1_ps(1.0f);
  __m256 one_minus_y = _mm256_sub_ps(ones, y_true);
  __m256 one_minus_y_pred = _mm256_sub_ps(ones, y_pred);
  __m256 ln_one_minus_y_pred = _mm256_log_ps(one_minus_y_pred);
  __m256 mul2 = _mm256_mul_ps(one_minus_y, ln_one_minus_y_pred);

  __m256 add = _mm256_add_ps(mul1, mul2);
  __m256 minus_ones = _mm256_set1_ps(-1.0f);
  return _mm256_mul_ps(minus_ones, add);
}

uo_avx_float uo_nn_loss_function_binary_cross_entropy_for_sigmoid_d(uo_avx_float y_true, uo_avx_float y_pred)
{
  return _mm256_sub_ps(y_pred, y_true);
}

uo_nn_function_param sigmoid_loss_binary_cross_entropy = {
  .activation = {
    .f = uo_nn_function_sigmoid,
  },
  .loss = {
    .f = uo_nn_loss_function_binary_cross_entropy,
    .df = uo_nn_loss_function_binary_cross_entropy_for_sigmoid_d,
  },
  .name = "sigmoid_loss_binary_cross_entropy"
};

uo_avx_float uo_nn_loss_function_binary_kl_divergence(uo_avx_float y_true, uo_avx_float y_pred)
{
  __m256 epsilon = _mm256_set1_ps(1e-6f);

  __m256 y_pred_nz = _mm256_add_ps(y_pred, epsilon);
  __m256 y_nz = _mm256_add_ps(y_true, epsilon);
  __m256 div1 = _mm256_div_ps(y_nz, y_pred_nz);
  __m256 ln1 = _mm256_log_ps(div1);
  __m256 mul1 = _mm256_mul_ps(y_true, ln1);

  __m256 ones = _mm256_set1_ps(1.0f);
  __m256 one_minus_y = _mm256_sub_ps(ones, y_true);
  __m256 one_minus_y_nz = _mm256_add_ps(one_minus_y, epsilon);
  __m256 one_minus_y_pred = _mm256_sub_ps(ones, y_pred);
  __m256 one_minus_y_pred_nz = _mm256_add_ps(one_minus_y_pred, epsilon);
  __m256 div2 = _mm256_div_ps(one_minus_y_nz, one_minus_y_pred_nz);
  __m256 ln2 = _mm256_log_ps(div2);
  __m256 mul2 = _mm256_mul_ps(one_minus_y, ln2);

  return _mm256_add_ps(mul1, mul2);
}

uo_avx_float uo_nn_loss_function_binary_kl_divergence_for_sigmoid_d(uo_avx_float y_true, uo_avx_float y_pred)
{
  return _mm256_sub_ps(y_pred, y_true);
}

uo_nn_function_param sigmoid_loss_binary_kl_divergence = {
  .activation = {
    .f = uo_nn_function_sigmoid,
  },
  .loss = {
    .f = uo_nn_loss_function_binary_kl_divergence,
    .df = uo_nn_loss_function_binary_kl_divergence_for_sigmoid_d,
  },
  .name = "sigmoid_loss_binary_kl_divergence"
};

const uo_nn_function_param *uo_nn_get_function_by_name(const char *function_name)
{
  if (function_name == NULL)
  {
    return &null;
  }

  if (strcmp(function_name, "sigmoid") == 0)
  {
    return &activation_sigmoid;
  }

  if (strcmp(function_name, "tanh") == 0)
  {
    return &activation_tanh;
  }

  if (strcmp(function_name, "relu") == 0)
  {
    return &activation_relu;
  }

  if (strcmp(function_name, "swish") == 0)
  {
    return &activation_swish;
  }

  if (strcmp(function_name, "loss_mse") == 0)
  {
    return &loss_mse;
  }

  if (strcmp(function_name, "loss_mae") == 0)
  {
    return &loss_mae;
  }

  if (strcmp(function_name, "sigmoid_loss_mae") == 0)
  {
    return &sigmoid_loss_mae;
  }

  if (strcmp(function_name, "sigmoid_loss_binary_cross_entropy") == 0)
  {
    return &sigmoid_loss_binary_cross_entropy;
  }

  if (strcmp(function_name, "sigmoid_loss_binary_kl_divergence") == 0)
  {
    return &sigmoid_loss_binary_kl_divergence;
  }

  return NULL;
}
