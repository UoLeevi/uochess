#include "uo_nn.h"

#include <immintrin.h>
#include <stdlib.h>

typedef struct uo_nn_layer uo_nn_layer;

#define uo_avx_float __m256
#define uo_floats_per_avx_float (sizeof(uo_avx_float) / sizeof(float))

#define uo_nn_learning_rate 0.01

typedef uo_avx_float uo_nn_activation_function(uo_avx_float avx_float);

typedef struct uo_nn_layer
{
  uo_avx_float *input;
  uo_avx_float *biases;
  uo_avx_float *weights;
  uo_avx_float *output;
  uo_avx_float *delta;
  size_t size_input;
  size_t size_output;
  uo_nn_activation_function *activation_func;
  uo_nn_activation_function *activation_func_d;
} uo_nn_layer;

typedef struct uo_nn
{
  size_t layer_count;
  uo_nn_layer *layers;
} uo_nn;


// see: https://stackoverflow.com/a/9194117
size_t uo_next_multiple_of(size_t value, size_t multiple)
{
  assert(multiple && ((multiple & (multiple - 1)) == 0));
  return (value + multiple - 1) & -multiple;
}

uo_avx_float uo_avx_float_relu(__m256 avx_float)
{
  __m256 zeros = _mm256_setzero_ps();
  return _mm256_max_ps(avx_float, zeros);
}

uo_avx_float uo_avx_float_relu_d(__m256 avx_float)
{
  __m256 zeros = _mm256_setzero_ps();
  __m256 mask = _mm256_cmp_ps(avx_float, zeros, _CMP_GT_OQ);
  __m256 ones = _mm256_set_ps(1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
  return _mm256_max_ps(mask, ones);
}

void uo_nn_init(uo_nn *nn, size_t layer_count, size_t size_input, ...)
{
  nn->layer_count = layer_count;
  nn->layers = calloc(nn->layer_count, sizeof * nn->layers);

  va_list args;
  va_start(args, size_input);

  uo_nn_layer *layer = nn->layers;
  size_t size_output = va_arg(args, size_t);
  layer->size_input = size_input = uo_next_multiple_of(size_input, uo_floats_per_avx_float);
  layer->input = calloc(size_input / uo_floats_per_avx_float, sizeof(uo_avx_float));
  layer->size_output = size_output = uo_next_multiple_of(size_output, uo_floats_per_avx_float);
  layer->output = calloc(size_output / uo_floats_per_avx_float, sizeof(uo_avx_float));
  layer->delta = calloc(size_output / uo_floats_per_avx_float, sizeof(uo_avx_float));
  layer->weights = calloc((size_input + 1) * size_output / uo_floats_per_avx_float, sizeof(uo_avx_float));
  layer->biases = layer->weights + size_input * size_output / uo_floats_per_avx_float;

  layer->activation_func = va_arg(args, uo_nn_activation_function *);
  layer->activation_func_d = va_arg(args, uo_nn_activation_function *);

  for (size_t i = 1; i < layer_count; ++i)
  {
    uo_avx_float *input = layer->output;
    ++layer;
    layer->size_input = size_input = size_output;
    layer->input = input;

    size_output = va_arg(args, size_t);
    layer->size_output = size_output = uo_next_multiple_of(size_output, uo_floats_per_avx_float);
    layer->output = calloc(size_output / uo_floats_per_avx_float, sizeof(uo_avx_float));
    layer->delta = calloc(size_output / uo_floats_per_avx_float, sizeof(uo_avx_float));
    layer->weights = calloc((size_input + 1) * size_output / uo_floats_per_avx_float, sizeof(uo_avx_float));
    layer->biases = layer->weights + size_input * size_output / uo_floats_per_avx_float;
  }

  va_end(args);
  float r[uo_floats_per_avx_float];

  for (size_t i = 0; i < layer_count; ++i)
  {
    uo_nn_layer *layer = nn->layers + i;
    size_t block_count = (layer->size_input + 1) * layer->size_output / uo_floats_per_avx_float;

    for (size_t j = 0; j < block_count; ++j)
    {
      for (size_t k = 0; k < uo_floats_per_avx_float; ++k)
      {
        r[k] = (float)rand() / (float)RAND_MAX;
      }

      layer->weights[j] = _mm256_loadu_ps(r);
    }
  }
}

void uo_nn_load_position(uo_nn *nn, const uo_position *position)
{
  float input[832] = { 0 };

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

  // Step 4. Load vector
  uo_nn_layer *input_layer = nn->layers;
  size_t block_count = input_layer->size_input / uo_floats_per_avx_float;

  for (size_t i = 0; i < block_count; ++i)
  {
    input_layer->input[i] = _mm256_loadu_ps(input + (i * uo_floats_per_avx_float));
  }
}

void uo_nn_feed_forward(uo_nn *nn)
{
  for (size_t layer_index = 0; layer_index < nn->layer_count; ++layer_index)
  {
    uo_nn_layer *layer = nn->layers + layer_index;
    size_t size_input = layer->size_input;
    size_t size_output = layer->size_output;
    uo_avx_float *input = layer->input;
    uo_avx_float *weights = layer->weights;
    uo_avx_float *biases = layer->biases;
    uo_avx_float *output = layer->output;

    size_t input_block_count = size_input / uo_floats_per_avx_float;
    size_t output_block_count = size_output / uo_floats_per_avx_float;

    for (size_t i = 0; i < output_block_count; ++i)
    {
      uo_avx_float activation = biases[i];

      for (size_t j = 0; j < input_block_count; ++j)
      {
        uo_avx_float value = input[j];
        uo_avx_float weight = weights[i * output_block_count + j];
        uo_avx_float mulres = _mm256_mul_ps(value, weight);
        activation = _mm256_add_ps(activation, mulres);
      }

      if (layer->activation_func)
      {
        activation = layer->activation_func(activation);
      }

      output[i] = activation;
    }
  }
}

void uo_nn_calculate_delta(uo_nn *nn, uo_avx_float *target)
{
  uo_nn_layer *output_layer = nn->layers + nn->layer_count - 1;
  uo_avx_float *output = output_layer->output;
  uo_avx_float *delta = output_layer->delta;
  size_t size_output = output_layer->size_output;
  size_t output_block_count = size_output / uo_floats_per_avx_float;

  for (size_t i = 0; i < output_block_count; ++i)
  {
    delta[i] = _mm256_sub_ps(target[i], output[i]);
  }
}

void uo_nn_backprop(uo_nn *nn)
{
  size_t layer_index = nn->layer_count;
  while (layer_index-- > 1)
  {
    uo_nn_layer *layer = nn->layers + layer_index;
    uo_avx_float *delta_out = layer->delta;
    uo_avx_float *delta_in = (layer - 1)->delta;

    size_t size_output = layer->size_output;
    size_t output_block_count = size_output / uo_floats_per_avx_float;

    size_t size_input = layer->size_input;
    size_t input_block_count = size_input / uo_floats_per_avx_float;

    for (size_t i = 0; i < input_block_count; ++i)
    {
      uo_avx_float error = _mm256_setzero_ps();
      for (size_t j = 0; j < output_block_count; ++j)
      {
        uo_avx_float value = delta_out[j];
        uo_avx_float weight = layer->weights[i * input_block_count + j];
        uo_avx_float mulres = _mm256_mul_ps(value, weight);
        error = _mm256_add_ps(error, mulres);
      }

      if (layer->activation_func_d)
      {
        delta_in[i] = layer->activation_func_d(delta_in[i]);
      }

      delta_in[i] = _mm256_mul_ps(error, delta_in[i]);
    }
  }

  // Update weights
  layer_index = nn->layer_count;
  while (layer_index--)
  {
    uo_nn_layer *layer = nn->layers + layer_index;
    size_t size_input = layer->size_input;
    size_t size_output = layer->size_output;
    uo_avx_float *weights = layer->weights;
    uo_avx_float *biases = layer->biases;
    uo_avx_float *input = layer->input;
    uo_avx_float *delta = layer->delta;

    size_t input_block_count = size_input / uo_floats_per_avx_float;
    size_t output_block_count = size_output / uo_floats_per_avx_float;

    __m256 lr = _mm256_set_ps(uo_nn_learning_rate, uo_nn_learning_rate, uo_nn_learning_rate, uo_nn_learning_rate, uo_nn_learning_rate, uo_nn_learning_rate, uo_nn_learning_rate, uo_nn_learning_rate);

    for (size_t i = 0; i < output_block_count; ++i)
    {
      biases[i] = _mm256_add_ps(biases[i], _mm256_mul_ps(delta[i], lr));

      for (size_t j = 0; j < input_block_count; ++j)
      {
        uo_avx_float value = input[j];
        uo_avx_float weight = weights[i * output_block_count + j];
        uo_avx_float mulres = _mm256_mul_ps(value, delta[i]);
        weights[i * output_block_count + j] = _mm256_add_ps(weight, _mm256_mul_ps(mulres, lr));
      }
    }
  }
}

void uo_nn_example()
{
  uo_nn nn;
  uo_nn_init(&nn, 3, 832,
    64, uo_avx_float_relu, uo_avx_float_relu_d,
    32, uo_avx_float_relu, uo_avx_float_relu_d,
    1, NULL, NULL);
}
