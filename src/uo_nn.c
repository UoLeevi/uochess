#include "uo_nn.h"
#include "uo_math.h"

#include <stdbool.h>

#define uo_nn_learning_rate 0.01

typedef uo_avx_float uo_nn_activation_function(uo_avx_float avx_float);

typedef struct uo_nn_layer_param
{
  size_t m_W;
  size_t n_W;
  uo_nn_activation_function *f;
  uo_nn_activation_function *f_d;
} uo_nn_layer_param;

typedef struct uo_nn_layer
{
  float *X;   // input matrix, batch_size x k
  float *X_t;
  float *W;   // weights matrix, k x n
  float *W_t;
  float *A;   // output matrix, batch_size x n
  float *A_t;
  float *G;   // gradient
  size_t m_W;   // weights row count
  size_t n_W;   // weights column count
  uo_nn_activation_function *activation_func;
  uo_nn_activation_function *activation_func_d;
} uo_nn_layer;

typedef struct uo_nn
{
  size_t layer_count;
  uo_nn_layer *layers;
  size_t batch_size;
  size_t output_size;
  float *output;
} uo_nn;

uo_avx_float uo_avx_float_relu(__m256 avx_float)
{
  __m256 zeros = _mm256_setzero_ps();
  return _mm256_max_ps(avx_float, zeros);
}

uo_avx_float uo_avx_float_relu_d(__m256 avx_float)
{
  __m256 zeros = _mm256_setzero_ps();
  __m256 mask = _mm256_cmp_ps(avx_float, zeros, _CMP_GT_OQ);
  __m256 ones = _mm256_set1_ps(1.0);
  return _mm256_max_ps(mask, ones);
}

void uo_nn_init(uo_nn *nn, size_t layer_count, size_t batch_size, uo_nn_layer_param *layer_params)
{
  // Step 1. Allocate layers array
  nn->layer_count = layer_count;
  nn->layers = calloc(nn->layer_count, sizeof * nn->layers);
  uo_nn_layer *input_layer = nn->layers;

  // Step 2. Allocate input layer
  size_t m_W = input_layer->m_W = layer_params->m_W;
  size_t n_W = input_layer->n_W = layer_params->n_W;
  size_t size_input = batch_size * (m_W + 1);
  size_t size_weights = (m_W + 1) * n_W;
  size_t size_output = batch_size * (n_W + 1);
  size_t size_total = size_input * 2 + size_output * 2 + size_weights * 3;

  float *mem = calloc(size_total, sizeof(float));

  input_layer->X = mem;
  mem += size_input;
  input_layer->X_t = mem;
  mem += size_input;
  input_layer->A = mem;
  mem += size_output;
  input_layer->A_t = mem;
  mem += size_output;
  input_layer->G = mem;
  mem += size_weights;
  input_layer->W = mem;
  mem += size_weights;
  input_layer->W_t = mem;

  input_layer->activation_func = layer_params->f;
  input_layer->activation_func_d = layer_params->f_d;

  // Step 3. Allocate layers 1..n
  uo_nn_layer *layer = input_layer;

  for (size_t i = 1; i < layer_count; ++i)
  {
    ++layer;
    ++layer_params;

    size_t m_W = layer->m_W = input_layer->n_W;
    size_t n_W = layer->n_W = layer_params->n_W;

    layer->X = input_layer->A;
    layer->X_t = input_layer->A_t;
    ++input_layer;

    size_t size_weights = (m_W + 1) * n_W;
    size_t size_output = batch_size * (n_W + 1);
    size_t size_total = size_output * 2 + size_weights * 3;

    float *mem = calloc(size_total, sizeof(float));

    layer->A = mem;
    mem += size_output;
    layer->A_t = mem;
    mem += size_output;
    layer->G = mem;
    mem += size_weights;
    layer->W = mem;
    mem += size_weights;
    layer->W_t = mem;

    layer->activation_func = layer_params->f;
    layer->activation_func_d = layer_params->f_d;
  }

  // Step 4. Set output pointer
  uo_nn_layer *output_layer = nn->layers + layer_count - 1;
  nn->output_size = output_layer->n_W;
  nn->output = output_layer->A;

  // Step 5. Initialize weights to random values
  for (size_t i = 0; i < layer_count; ++i)
  {
    uo_nn_layer *layer = nn->layers + i;
    size_t count = (layer->m_W + 1) * layer->n_W;

    for (size_t j = 0; j < count; ++j)
    {
      layer->W[j] = (((float)rand() / (float)RAND_MAX) - 0.5) * 2;
    }

    uo_transpose_ps(layer->W, layer->W_t, layer->m_W, layer->n_W);
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
    float *W_t = layer->W_t;
    float *A = layer->A;

    // Set bias input
    for (size_t i = 1; i <= m_W; ++i)
    {
      X[i * n_W - 1] = 1.0;
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
    float *X_t = layer->X_t;
    uo_transpose_ps(X, X_t, nn->batch_size, m_W + 1);

    float *W = layer->W;
    float *W_t = layer->W_t;
    float *dA = layer->A;

    if (layer->activation_func_d)
    {
      uo_vec_mapfunc_ps(dA, nn->batch_size * n_W, layer->activation_func_d);
    }

    uo_matmul_ps(X, dA, layer->G, m_W, n_W, nn->batch_size);

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
    { .m_W = 832, .n_W = 64, .f = uo_avx_float_relu, .f_d = uo_avx_float_relu_d },
    { .n_W = 32, .f = uo_avx_float_relu, .f_d = uo_avx_float_relu_d },
    { .n_W = 1 }
  });
}

bool uo_test_nn_train()
{
  uo_nn nn;
  uo_nn_init(&nn, 2, 1, (uo_nn_layer_param[]) {
    { .m_W = 2, .n_W = 2, .f = uo_avx_float_relu, .f_d = uo_avx_float_relu_d },
    { .n_W = 1, .f = uo_avx_float_relu, .f_d = uo_avx_float_relu_d }
  });

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
