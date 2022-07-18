#include "uo_nn.h"

#include <immintrin.h>

typedef struct uo_nn_layer uo_nn_layer;

#define uo_avx_float __m256
#define uo_floats_per_avx_float (sizeof(float) / sizeof(uo_avx_float))

typedef struct uo_nn_layer
{
  size_t m;
  size_t n;
  uo_avx_float *biases;
  uo_avx_float *weights;
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

void uo_nn_layer_init(uo_nn_layer *nn_layer, size_t m, size_t n)
{
  nn_layer->m = m;
  nn_layer->n = n;

  m = uo_next_multiple_of(m, uo_floats_per_avx_float);
  n = uo_next_multiple_of(n, uo_floats_per_avx_float);
  nn_layer->weights = calloc((m + 1) * n / uo_floats_per_avx_float, sizeof(uo_avx_float));
  nn_layer->biases = nn_layer->weights + m * n / uo_floats_per_avx_float;
}


void uo_nn_init(uo_nn *nn, size_t layer_count)
{
  nn->layer_count = layer_count;
  nn->layers = calloc(nn->layer_count, sizeof * nn->layers);
}

float uo_nn_evaluate(uo_position *position)
{
}



void uo_nn_example()
{
  uo_nn nn;
  uo_nn_init(&nn, 3);

  uo_nn_layer_init(&nn.layers[0], 832, 64);
  uo_nn_layer_init(&nn.layers[1], 64, 32);
  uo_nn_layer_init(&nn.layers[2], 32, 1);
}
