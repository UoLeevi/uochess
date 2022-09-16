#include "uo_nn.h"
#include "uo_math.h"

#include <stdbool.h>

#define uo_nn_learning_rate 0.01

typedef uo_avx_float uo_nn_loss_function(uo_avx_float y_true, uo_avx_float y_pred);

typedef struct uo_nn_loss_function_param
{
  uo_nn_loss_function *f;
  uo_nn_loss_function *df;
} uo_nn_loss_function_param;

uo_avx_float uo_nn_loss_function_mean_squared_error(uo_avx_float y_true, uo_avx_float y_pred)
{
  __m256 sub = _mm256_sub_ps(y_true, y_pred);
  __m256 mul = _mm256_mul_ps(sub, sub);
  return mul;
}

uo_avx_float uo_nn_loss_function_mean_squared_error_d(uo_avx_float y_true, uo_avx_float y_pred)
{
  __m256 sub = _mm256_sub_ps(y_true, y_pred);
  __m256 n2 = _mm256_set1_ps(-2.0);
  __m256 mul = _mm256_mul_ps(sub, n2);
  return mul;
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
  size_t m_W = input_layer->m_W = layer_params[0].n + 1;
  size_t n_W = input_layer->n_W = layer_params[1].n;
  size_t size_input = batch_size * m_W;
  size_t size_weights = m_W * n_W;
  size_t size_output = batch_size * n_W;
  size_t size_total = size_input * 2 + size_output * 2 + size_weights * 2;
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
    size_t size_total = size_output * 2 + size_weights * 2;

    float *mem = calloc(size_total, sizeof(float));

    layer->A = mem;
    mem += size_output;
    layer->dA = mem;
    mem += size_output;
    layer->W = mem;
    mem += size_weights;
    layer->dW = mem;

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
    nn->temp[0] = temp + size_max * i;
  }
}

void uo_nn_load_position(uo_nn *nn, const uo_position *position, size_t index)
{
  size_t m_W = nn->layers->m_W;
  float *input = nn->layers->X + (m_W + 1) * index;
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

void uo_nn_feed_forward(uo_nn *nn)
{
  for (size_t layer_index = 0; layer_index < nn->layer_count; ++layer_index)
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

    uo_matmul_ps(X, W_t, A, nn->batch_size, n_W, m_W + 1);

    if (layer->activation_func)
    {
      uo_vec_mapfunc_ps(A, nn->batch_size * n_W, layer->activation_func);
    }
  }
}

void uo_nn_calculate_loss(uo_nn *nn, float *target)
{
  uo_vecsub_ps(target, nn->output, nn->output, nn->output_size);
}

void uo_nn_backprop(uo_nn *nn)
{
  size_t layer_index = nn->layer_count;
  while (layer_index-- > 0)
  {
    uo_nn_layer *layer = nn->layers + layer_index;
    size_t m_W = layer->m_W;
    size_t n_W = layer->n_W;

    float *X = layer->X;
    float *X_t = nn->temp[0];
    uo_transpose_ps(X, X_t, nn->batch_size, m_W + 1);

    float *W = layer->W;
    float *W_t = nn->temp[1];
    uo_transpose_ps(W, W_t, m_W, n_W);

    float *dA = layer->A;

    if (layer->activation_func_d)
    {
      uo_vec_mapfunc_ps(dA, nn->batch_size * n_W, layer->activation_func_d);
    }

    uo_matmul_ps(X, dA, layer->dW, m_W, n_W, nn->batch_size);

    if (layer_index > 0)
    {
      float *dX = layer->X;
      uo_matmul_ps(dA, W_t, dX, nn->batch_size, m_W, n_W);
    }
  }

  //// Update weights
  //float lr = _mm256_set1_ps(uo_nn_learning_rate);

  //layer_index = nn->layer_count;
  //while (layer_index--)
  //{
  //  // TODO:

  //  uo_nn_layer *layer = nn->layers + layer_index;
  //  size_t size_input = layer->size_input;
  //  size_t size_output = layer->size_output;
  //  float *weights = layer->weights;
  //  float *biases = layer->biases;
  //  float *input = layer->input;
  //  float *delta = layer->delta;

  //  size_t input_block_count = size_input / uo_floats_per_avx_float;
  //  size_t output_block_count = size_output / uo_floats_per_avx_float;

  //  for (size_t i = 0; i < output_block_count; ++i)
  //  {
  //    biases[i] = _mm256_add_ps(biases[i], _mm256_mul_ps(delta[i], lr));

  //    for (size_t j = 0; j < input_block_count; ++j)
  //    {
  //      float weight = weights[i * output_block_count + j];
  //      float mulres = _mm256_mul_ps(input[j], delta[i]);
  //      weights[i * output_block_count + j] = _mm256_add_ps(weight, _mm256_mul_ps(mulres, lr));
  //    }
  //  }
  //}
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
  uo_nn nn;
  uo_nn_init(&nn, 2, 1, (uo_nn_layer_param[]) {
    { 2 },
    { 2, activation_relu },
    { 1, activation_relu }
  }, loss_mse);

  for (size_t i = 0; i < 1000000; ++i)
  {
    float x0 = rand() > (RAND_MAX / 2) ? 1.0 : 0.0;
    float x1 = rand() > (RAND_MAX / 2) ? 1.0 : 0.0;
    float y = (int)x0 ^ (int)x1;

    memset(nn.layers->X, 0, nn.batch_size * (nn.layers->m_W + 1) * sizeof(float));
    nn.layers->X[0] = x0;
    nn.layers->X[1] = x1;

    uo_nn_feed_forward(&nn);

    float target[8] = { y };
    uo_nn_calculate_loss(&nn, target);
    uo_nn_backprop(&nn);
  }

  bool passed = true;

  for (size_t i = 0; i < 10000; ++i)
  {
    float x0 = rand() > (RAND_MAX / 2) ? 1.0 : 0.0;
    float x1 = rand() > (RAND_MAX / 2) ? 1.0 : 0.0;
    float y = (int)x0 ^ (int)x1;

    memset(nn.layers->X, 0, nn.batch_size * (nn.layers->m_W + 1) * sizeof(float));
    nn.layers->X[0] = x0;
    nn.layers->X[1] = x1;

    uo_nn_feed_forward(&nn);

    float output = *nn.output;

    float delta = y - output;
    delta = delta > 0.0 ? delta : -delta;
    passed &= delta < 0.1;
  }

  return passed;
}
