#include "uo_nn.h"
#include "uo_math.h"

#include <stdbool.h>
#include <math.h>
#include <stdio.h>

// see: https://arxiv.org/pdf/1412.6980.pdf
#define uo_nn_adam_learning_rate 1e-3
#define uo_nn_adam_beta1 0.9
#define uo_nn_adam_beta2 0.999
#define uo_nn_adam_epsilon 1e-8

typedef uo_avx_float uo_nn_loss_function(uo_avx_float y_true, uo_avx_float y_pred);

typedef struct uo_nn_loss_function_param
{
  uo_nn_loss_function *f;
  uo_nn_loss_function *df;
} uo_nn_loss_function_param;

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

uo_nn_loss_function_param loss_mse = {
  .f = uo_nn_loss_function_mean_squared_error,
  .df = uo_nn_loss_function_mean_squared_error_d
};

typedef uo_avx_float uo_nn_activation_function(uo_avx_float avx_float);

typedef struct uo_nn_activation_function_param
{
  uo_nn_activation_function *f;
  uo_nn_activation_function *df;
} uo_nn_activation_function_param;

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

uo_nn_activation_function_param activation_relu = {
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

uo_nn_activation_function_param activation_sigmoid = {
  .f = uo_nn_activation_function_sigmoid,
  .df = uo_nn_activation_function_sigmoid_d
};

typedef struct uo_nn_layer_param
{
  size_t n;
  uo_nn_activation_function_param activation_function;
} uo_nn_layer_param;

typedef struct uo_nn_layer
{
  float *X;   // input matrix, batch_size x m
  float *dX;  // gradient of input, batch_size x m
  float *W;   // weights matrix, m x n
  float *dW;  // gradient of weights, m x n
  float *A;   // output matrix, batch_size x n
  float *dA;  // gradient of output, batch_size x n
  size_t m_W;   // weights row count
  size_t n_W;   // weights column count
  uo_nn_activation_function *activation_func;
  uo_nn_activation_function *activation_func_d;

  struct
  {
    float *m; // first moment vector
    float *v; // second moment vector
    float *m_hat; // bias-corrected first moment matrix, m x n
    float *v_hat; // bias-corrected second moment matrix, m x n
  } adam;

} uo_nn_layer;

#define uo_nn_temp_var_count 5

typedef struct uo_nn
{
  size_t layer_count;
  uo_nn_layer *layers;
  size_t batch_size;
  size_t output_size;
  float *output;
  uo_nn_loss_function *loss_func;
  uo_nn_loss_function *loss_func_d;

  // Some allocated memory to use in calculations
  float *temp[uo_nn_temp_var_count];

  struct
  {
    size_t t; // timestep
  } adam;
} uo_nn;

void uo_nn_init(uo_nn *nn, size_t layer_count, size_t batch_size, uo_nn_layer_param *layer_params, uo_nn_loss_function_param loss)
{
  // Step 1. Allocate layers array and set options
  nn->batch_size = batch_size;
  nn->loss_func = loss.f;
  nn->loss_func_d = loss.df;
  nn->layer_count = layer_count;
  nn->layers = calloc(nn->layer_count, sizeof * nn->layers);
  uo_nn_layer *input_layer = nn->layers;

  // Step 2. Allocate input layer
  ++layer_params;
  size_t m_W = input_layer->m_W = layer_params[-1].n + 1;
  size_t n_W = input_layer->n_W = layer_params[0].n;
  size_t size_input = batch_size * m_W;
  size_t size_weights = m_W * n_W;
  size_t size_output = batch_size * n_W;
  size_t size_total = size_input * 2 + size_output * 2 + size_weights * 6;
  size_t size_max = uo_max(size_input, size_output);
  size_max = uo_max(size_max, size_weights);

  float *mem = calloc(size_total, sizeof(float));

  input_layer->X = mem;
  mem += size_input;
  input_layer->dX = mem;
  mem += size_input;
  input_layer->A = mem;
  mem += size_output;
  input_layer->dA = mem;
  mem += size_output;
  input_layer->W = mem;
  mem += size_weights;
  input_layer->dW = mem;
  mem += size_weights;
  input_layer->adam.m = mem;
  mem += size_weights;
  input_layer->adam.v = mem;
  mem += size_weights;
  input_layer->adam.m_hat = mem;
  mem += size_weights;
  input_layer->adam.v_hat = mem;

  input_layer->activation_func = layer_params->activation_function.f;
  input_layer->activation_func_d = layer_params->activation_function.df;

  // Step 3. Allocate layers 1..n
  uo_nn_layer *layer = input_layer;

  for (size_t i = 2; i <= layer_count; ++i)
  {
    ++layer;
    ++layer_params;

    size_t m_W = layer->m_W = input_layer->n_W + 1;
    size_t n_W = layer->n_W = layer_params->n;

    layer->X = input_layer->A;
    layer->dX = input_layer->dA;
    ++input_layer;

    size_t size_weights = m_W * n_W;
    size_max = uo_max(size_max, size_weights);
    size_t size_output = batch_size * n_W;
    size_max = uo_max(size_max, size_output);
    size_t size_total = size_output * 2 + size_weights * 6;

    float *mem = calloc(size_total, sizeof(float));

    layer->A = mem;
    mem += size_output;
    layer->dA = mem;
    mem += size_output;
    layer->W = mem;
    mem += size_weights;
    layer->dW = mem;
    mem += size_weights;
    layer->adam.m = mem;
    mem += size_weights;
    layer->adam.v = mem;
    mem += size_weights;
    layer->adam.m_hat = mem;
    mem += size_weights;
    layer->adam.v_hat = mem;

    layer->activation_func = layer_params->activation_function.f;
    layer->activation_func_d = layer_params->activation_function.df;
  }

  // Step 4. Set output pointer
  uo_nn_layer *output_layer = nn->layers + layer_count - 1;
  nn->output_size = output_layer->n_W;
  nn->output = output_layer->A;

  // Step 5. Initialize weights to random values
  for (size_t i = 0; i < layer_count; ++i)
  {
    uo_nn_layer *layer = nn->layers + i;
    size_t count = layer->m_W * layer->n_W;

    for (size_t j = 0; j < count; ++j)
    {
      layer->W[j] = (((float)rand() / (float)RAND_MAX) - 0.5) * 2;
    }
  }

  // Step 6. Allocate space for temp data
  float *temp = calloc(size_max * uo_nn_temp_var_count, sizeof(float));
  for (size_t i = 0; i < uo_nn_temp_var_count; ++i)
  {
    nn->temp[i] = temp + size_max * i;
  }
}

void uo_nn_load_position(uo_nn *nn, const uo_position *position, size_t index)
{
  size_t m_W = nn->layers->m_W;
  float *input = nn->layers->X + m_W * index;
  memset(input, 0, m_W * sizeof(float));

  // Step 1. Piece locations

  for (uo_square i = 0; i < 64; ++i)
  {
    uo_piece piece = position->board[i];

    if (piece <= 1) continue;

    size_t offset = uo_color(piece) ? 64 * 5 + 48 : 0;

    switch (piece)
    {
      case uo_piece__P:
        offset += 64 * 5 - 8;
        break;

      case uo_piece__N:
        offset += 64 * 4;
        break;

      case uo_piece__B:
        offset += 64 * 3;
        break;

      case uo_piece__R:
        offset += 64 * 2;
        break;

      case uo_piece__Q:
        offset += 64 * 1;
        break;
    }

    input[offset + i] = 1.0;
  }

  // Step 2. Castling
  size_t offset_castling = (64 * 5 + 48) * 2;
  input[offset_castling + 0] = uo_position_flags_castling_OO(position->flags);
  input[offset_castling + 1] = uo_position_flags_castling_OOO(position->flags);
  input[offset_castling + 2] = uo_position_flags_castling_enemy_OO(position->flags);
  input[offset_castling + 3] = uo_position_flags_castling_enemy_OOO(position->flags);

  // Step 3. Enpassant
  size_t offset_enpassant = offset_castling + 4;
  size_t enpassant_file = uo_position_flags_enpassant_file(position->flags);
  if (enpassant_file)
  {
    input[offset_enpassant + enpassant_file - 1] = 1.0;
  }
}

float uo_nn_calculate_loss(uo_nn *nn, float *y_true)
{
  float *losses = nn->temp[0];
  uo_vec_map2func_ps(y_true, nn->output, losses, nn->output_size * nn->batch_size, nn->loss_func);
  float loss = uo_vec_mean_ps(losses, nn->output_size * nn->batch_size);
  return loss;
}

void uo_nn_feed_forward(uo_nn *nn)
{
  // Hidden layers
  for (size_t layer_index = 0; layer_index < nn->layer_count - 1; ++layer_index)
  {
    uo_nn_layer *layer = nn->layers + layer_index;
    size_t m_W = layer->m_W;
    size_t n_W = layer->n_W;
    float *X = layer->X;
    float *W = layer->W;
    float *W_t = nn->temp[0];
    uo_transpose_ps(W, W_t, m_W, n_W);
    float *A = layer->A;

    // Set bias input
    for (size_t i = 1; i <= nn->batch_size; ++i)
    {
      X[i * m_W - 1] = 1.0;
    }

    uo_matmul_ps(X, W_t, A, nn->batch_size, n_W, m_W, 1);

    if (layer->activation_func)
    {
      uo_vec_mapfunc_ps(A, A, nn->batch_size * n_W, layer->activation_func);
    }
  }

  // Output layer
  uo_nn_layer *layer = nn->layers + nn->layer_count - 1;
  size_t m_W = layer->m_W;
  size_t n_W = layer->n_W;
  float *X = layer->X;
  float *W = layer->W;
  float *W_t = nn->temp[0];
  uo_transpose_ps(W, W_t, m_W, n_W);
  float *A = layer->A;

  // Set bias input
  for (size_t i = 1; i <= nn->batch_size; ++i)
  {
    X[i * m_W - 1] = 1.0;
  }

  uo_matmul_ps(X, W_t, A, nn->batch_size, n_W, m_W, 0);

  if (layer->activation_func)
  {
    uo_vec_mapfunc_ps(A, A, nn->batch_size * n_W, layer->activation_func);
  }
}

void uo_nn_backprop(uo_nn *nn, float *y_true)
{
  // Derivative of loss wrt output
  float *dA = nn->layers[nn->layer_count - 1].dA;
  uo_vec_map2func_ps(y_true, nn->output, dA, nn->batch_size * nn->output_size, nn->loss_func_d);

  size_t layer_index = nn->layer_count;
  while (layer_index-- > 0)
  {
    uo_nn_layer *layer = nn->layers + layer_index;
    size_t m_W = layer->m_W;
    size_t n_W = layer->n_W;

    float *dZ = layer->dA;

    if (layer->activation_func_d)
    {
      // Derivative of output wrt Z
      float *dAdZ = nn->temp[0];
      float *A = layer->A;
      uo_vec_mapfunc_ps(A, dAdZ, nn->batch_size * n_W, layer->activation_func_d);

      // Derivative of loss wrt output (activation)
      float *dA = layer->dA;
      uo_vecmul_ps(dAdZ, dA, dZ, nn->batch_size * n_W);
    }

    // Derivative of loss wrt weights
    float *dW = layer->dW;
    float *X = layer->X;
    float *X_t = nn->temp[1];
    uo_transpose_ps(X, X_t, nn->batch_size, m_W);
    uo_matmul_ps(X_t, dZ, dW, m_W, n_W, nn->batch_size, 0);

    if (layer_index)
    {
      // Derivative of loss wrt input
      float *dX = layer->dX;
      float *W = layer->W;
      uo_matmul_ps(dZ, W, dX, nn->batch_size, m_W, n_W, -1);
    }

    // Update weights using Adam update
    float *W = layer->W;
    float *m = layer->adam.m;
    float *v = layer->adam.v;
    float *m_hat = layer->adam.m_hat;
    float *v_hat = layer->adam.v_hat;

    for (size_t i = 0; i < m_W * n_W; ++i)
    {
      m[i] = uo_nn_adam_beta1 * m[i] + (1 - uo_nn_adam_beta1) * dW[i];
      v[i] = uo_nn_adam_beta2 * v[i] + (1 - uo_nn_adam_beta2) * (dW[i] * dW[i]);
      m_hat[i] = m[i] / (1 - powf(uo_nn_adam_beta1, (float)nn->adam.t));
      v_hat[i] = v[i] / (1 - powf(uo_nn_adam_beta2, (float)nn->adam.t));
      W[i] -= uo_nn_adam_learning_rate * m_hat[i] / (sqrtf(v_hat[i] + uo_nn_adam_epsilon));
    }
  }

  // Increment timestep
  nn->adam.t++;
}

void uo_nn_example()
{
  uo_nn nn;
  uo_nn_init(&nn, 3, 1, (uo_nn_layer_param[]) {
    { 832 },
    { 64, activation_relu },
    { 32, activation_relu },
    { 1 }
  }, loss_mse);
}

bool uo_test_nn_train()
{
  bool passed = false;

  size_t batch_size = 64;
  uo_nn nn;
  uo_nn_init(&nn, 3, batch_size, (uo_nn_layer_param[]) {
    { 2 },
    { 4, activation_relu },
    { 4, activation_relu },
    { 1 }
  }, loss_mse);

  float *y_true = uo_alloca(batch_size * sizeof(float));

  for (size_t i = 0; i < 100000; ++i)
  {
    memset(nn.layers->X, 0, nn.batch_size * nn.layers->m_W * sizeof(float));
    for (size_t j = 0; j < batch_size; ++j)
    {
      float x0 = rand() > (RAND_MAX / 2) ? 1.0 : 0.0;
      float x1 = rand() > (RAND_MAX / 2) ? 1.0 : 0.0;
      float y = (int)x0 ^ (int)x1;
      y_true[j] = y;
      nn.layers->X[j * nn.layers->m_W] = x0;
      nn.layers->X[j * nn.layers->m_W + 1] = x1;
    }

    uo_nn_feed_forward(&nn);

    float mse = uo_nn_calculate_loss(&nn, y_true);
    float rmse = sqrt(mse);

    if (i % 1000 == 0)
    {
      printf("iteration %zu rmse: %f\n", i, rmse);
    }

    if (rmse < 0.001)
    {
      passed = true;
    }

    uo_nn_backprop(&nn, y_true);
  }

  for (size_t i = 0; i < 1000; ++i)
  {
    memset(nn.layers->X, 0, nn.batch_size * nn.layers->m_W * sizeof(float));
    for (size_t j = 0; j < batch_size; ++j)
    {
      float x0 = rand() > (RAND_MAX / 2) ? 1.0 : 0.0;
      float x1 = rand() > (RAND_MAX / 2) ? 1.0 : 0.0;
      float y = (int)x0 ^ (int)x1;
      y_true[j] = y;
      nn.layers->X[j * nn.layers->m_W] = x0;
      nn.layers->X[j * nn.layers->m_W + 1] = x1;
    }

    uo_nn_feed_forward(&nn);

    float mse = uo_nn_calculate_loss(&nn, y_true);
    float rmse = sqrt(mse);

    if (rmse > 0.001)
    {
      return false;
    }
  }

  return true;
}
