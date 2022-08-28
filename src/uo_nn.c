#include "uo_nn.h"

#include <immintrin.h>
#include <stdlib.h>

typedef struct uo_nn_layer uo_nn_layer;

#define uo_avx_float __m256
#define uo_floats_per_avx_float (sizeof(uo_avx_float) / sizeof(float))

typedef struct uo_nn_layer
{
  uo_avx_float *biases;
  uo_avx_float *weights;
  size_t size_output;
} uo_nn_layer;

typedef struct uo_nn
{
  size_t layer_count;
  uo_nn_layer *layers;
  uo_avx_float *input;
  size_t size_input;
} uo_nn;


// see: https://stackoverflow.com/a/9194117
size_t uo_next_multiple_of(size_t value, size_t multiple)
{
  assert(multiple && ((multiple & (multiple - 1)) == 0));
  return (value + multiple - 1) & -multiple;
}

void uo_nn_input_layer_init(uo_nn *nn, size_t size_input, size_t size_output)
{
  uo_nn_layer *input_layer = nn->layers;

  nn->size_input = size_input;
  input_layer->size_output = size_output;

  size_input = uo_next_multiple_of(size_input, uo_floats_per_avx_float);
  size_output = uo_next_multiple_of(size_output, uo_floats_per_avx_float);

  nn->input = calloc(size_input / uo_floats_per_avx_float, sizeof(uo_avx_float));
  input_layer->weights = calloc((size_input + 1) * size_output / uo_floats_per_avx_float, sizeof(uo_avx_float));
  input_layer->biases = input_layer->weights + size_input * size_output / uo_floats_per_avx_float;

  size_t block_count = (size_input + 1) * size_output / uo_floats_per_avx_float;

  float r[uo_floats_per_avx_float];

  for (size_t i = 0; i < block_count; ++i)
  {
    for (size_t j = 0; i < uo_floats_per_avx_float; ++j)
    {
      r[j] = (float)rand() / (float)RAND_MAX;
    }

    input_layer->weights[i] = _mm256_loadu_ps(r);
  }
}

void uo_nn_hidden_layer_init(uo_nn *nn, size_t hidden_layer_index, size_t size_output)
{
  uo_nn_layer *hidden_layer = nn->layers + hidden_layer_index;
  uo_nn_layer *prev_layer = hidden_layer - 1;

  size_t size_input = prev_layer->size_output;
  hidden_layer->size_output = size_output;

  size_output = uo_next_multiple_of(size_output, uo_floats_per_avx_float);

  hidden_layer->weights = calloc((size_input + 1) * size_output / uo_floats_per_avx_float, sizeof(uo_avx_float));
  hidden_layer->biases = hidden_layer->weights + size_input * size_output / uo_floats_per_avx_float;

  size_t block_count = (size_input + 1) * size_output / uo_floats_per_avx_float;

  float r[uo_floats_per_avx_float];

  for (size_t i = 0; i < block_count; ++i)
  {
    for (size_t j = 0; i < uo_floats_per_avx_float; ++j)
    {
      r[j] = (float)rand() / (float)RAND_MAX;
    }

    hidden_layer->weights[i] = _mm256_loadu_ps(r);
  }
}


void uo_nn_init(uo_nn *nn, size_t layer_count)
{
  nn->layer_count = layer_count;
  nn->layers = calloc(nn->layer_count, sizeof * nn->layers);
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
  size_t block_count = nn->size_input / uo_floats_per_avx_float;

  for (size_t i = 0; i < block_count; ++i)
  {
    nn->input[i] = _mm256_loadu_ps(input + (i * uo_floats_per_avx_float));
  }
}

float uo_nn_evaluate(uo_position *position)
{
}



void uo_nn_example()
{
  uo_nn nn;
  uo_nn_init(&nn, 3);

  uo_nn_input_layer_init(&nn, 832, 64);
  uo_nn_hidden_layer_init(&nn, 1, 32);
  uo_nn_hidden_layer_init(&nn, 2, 1);
}
