#include "uo_nn.h"

#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#pragma region uo_nn_node_weights

static void uo_nn_node_weights__init(uo_nn_node **graph, uo_nn *nn)
{
  uo_nn_node_weights *node = (uo_nn_node_weights *)(void *)*graph;

  size_t m_W = node->base.m;
  size_t n_W = node->base.n;
  size_t size = m_W * n_W;

  float *mem = calloc(size * 6, sizeof(float));
  node->base.A = mem;
  mem += size;
  node->base.dA = mem;
  mem += size;
  node->adam.m = mem;
  mem += size;
  node->adam.v = mem;
  mem += size;
  node->adam.m_hat = mem;
  mem += size;
  node->adam.v_hat = mem;

  node->adam.t = 1;
}

static void uo_nn_node_weights__backward(uo_nn_node **graph)
{
  uo_nn_node_weights *node = (uo_nn_node_weights *)(void *)graph[0];

  size_t m_W = node->base.m;
  size_t n_W = node->base.n;
  float *dW_t = node->base.dA;
  float *W_t = node->base.A;
  float *m = node->adam.m;
  float *v = node->adam.v;
  float *m_hat = node->adam.m_hat;
  float *v_hat = node->adam.v_hat;
  float t = node->adam.t;

  float learning_rate = node->adam.learning_rate;
  float weight_decay = node->adam.weight_decay;
  float beta1 = node->adam.beta1;
  float beta2 = node->adam.beta2;
  float epsilon = node->adam.epsilon;

  for (size_t i = 0; i < m_W * n_W; ++i)
  {
    m[i] = beta1 * m[i] + (1.0f - beta1) * dW_t[i];
    v[i] = beta2 * v[i] + (1.0f - beta2) * (dW_t[i] * dW_t[i]);
    m_hat[i] = m[i] / (1.0f - powf(beta1, t));
    v_hat[i] = v[i] / (1.0f - powf(beta2, t));
    W_t[i] -= learning_rate * m_hat[i] / (sqrtf(v_hat[i] + epsilon));

    // weight decay
    W_t[i] -= (i + 1) % m_W == 0 ? 0.0f : (weight_decay * learning_rate * W_t[i]);
  }
}

#pragma endregion

#pragma region uo_nn_node_matmul

static void uo_nn_node_matmul__init(uo_nn_node **graph, uo_nn *nn)
{
  uo_nn_node_matmul *node = (uo_nn_node_matmul *)(void *)*graph;
  uo_nn_node_out_1f *input2 = (uo_nn_node_out_1f *)(void *)graph[2];

  size_t m = node->base.m;
  size_t offset_m = node->base.offset_m;
  size_t n = node->base.n;
  size_t offset_n = node->base.offset_n;
  size_t size = (m + offset_m) * (n + offset_n);
  size_t size_B = (input2->m + input2->offset_m) * (input2->n + input2->offset_n);

  float *mem = calloc(size * 2 + size_B, sizeof(float));
  node->base.A = mem;
  mem += size;
  node->base.dA = mem;
  mem += size;
  node->B = mem;
}

static void uo_nn_node_matmul__forward(uo_nn_node **graph)
{
  uo_nn_node_matmul *node = (uo_nn_node_matmul *)(void *)graph[0];
  uo_nn_node_out_1f *input1 = (uo_nn_node_out_1f *)(void *)graph[1];
  uo_nn_node_out_1f *input2 = (uo_nn_node_out_1f *)(void *)graph[2];

  float *A = input1->A;
  size_t offset_A = input1->offset_n;
  float *B_t = input2->A;
  size_t offset_B = input2->offset_m;
  float *C = node->base.A;
  size_t offset_C = node->base.offset_n;
  uo_matmul_ps(A, B_t, C, input1->m, input2->n, input2->m, offset_C, offset_A, offset_B);
}

static void uo_nn_node_matmul__backward(uo_nn_node **graph)
{
  uo_nn_node_matmul *node = (uo_nn_node_matmul *)(void *)graph[0];
  uo_nn_node_out_1f *input1 = (uo_nn_node_out_1f *)(void *)graph[1];
  uo_nn_node_out_1f *input2 = (uo_nn_node_out_1f *)(void *)graph[2];

  float *A = input1->A;
  size_t offset_A = input1->offset_n;
  float *dB_t = input2->dA;
  size_t offset_B = input2->offset_m;
  size_t offset_C = node->base.offset_n;

  uo_matmul_t_ps(A, node->base.dA, dB_t, input2->m, input2->n, input1->m, offset_B, offset_A, offset_C);

  float *dA = input1->dA;
  float *B_t = input2->A;
  float *B = node->B;
  uo_transpose_ps(B_t, B, input2->n, input2->m);
  uo_matmul_ps(node->base.dA, B, dA, input1->m, input2->m, input2->n, offset_A, offset_C, offset_B);
}

#pragma endregion

#pragma region uo_nn_node_out_1f

static void uo_nn_node_out_1f__init(uo_nn_node **graph, uo_nn *nn)
{
  uo_nn_node_out_1f *node = (uo_nn_node_out_1f *)(void *)*graph;

  size_t m = node->m = nn->batch_size;
  size_t n = node->n;
  size_t size = m * n;

  float *mem = calloc(size * 2, sizeof(float));
  node->A = mem;
  mem += size;
  node->dA = mem;
}

#pragma endregion

#pragma region uo_nn_node_tanh

static void uo_nn_node_tanh__forward(uo_nn_node **graph)
{
  uo_nn_node_out_1f *node = (uo_nn_node_out_1f *)(void *)graph[0];
  uo_nn_node_out_1f *input = (uo_nn_node_out_1f *)(void *)graph[1];
  uo_vec_mapfunc_ps(input->A, node->A, node->m * node->n, _mm256_tanh_ps);
}

static uo_avx_float uo_nn_function_tanh_d(__m256 avx_float)
{
  __m256 tanh = _mm256_tanh_ps(avx_float);
  __m256 sqr = _mm256_mul_ps(tanh, tanh);
  __m256 ones = _mm256_set1_ps(1.0f);
  return _mm256_sub_ps(ones, sqr);
}

static void uo_nn_node_tanh__backward(uo_nn_node **graph)
{
  uo_nn_node_out_1f *node = (uo_nn_node_out_1f *)(void *)graph[0];
  uo_nn_node_out_1f *input = (uo_nn_node_out_1f *)(void *)graph[1];
  uo_vec_mapfunc_mul_ps(input->A, node->dA, input->dA, node->m * node->n, uo_nn_function_tanh_d);
}

#pragma endregion

#pragma region uo_nn_node_sigmoid

static uo_avx_float uo_nn_function_sigmoid(__m256 avx_float)
{
  __m256 neg = _mm256_sub_ps(_mm256_setzero_ps(), avx_float);
  __m256 exp = _mm256_exp_ps(neg);
  __m256 ones = _mm256_set1_ps(1.0f);
  __m256 add = _mm256_add_ps(ones, exp);
  return _mm256_div_ps(ones, add);
}

static void uo_nn_node_sigmoid__forward(uo_nn_node **graph)
{
  uo_nn_node_out_1f *node = (uo_nn_node_out_1f *)(void *)graph[0];
  uo_nn_node_out_1f *input = (uo_nn_node_out_1f *)(void *)graph[1];
  uo_vec_mapfunc_ps(input->A, node->A, node->m * node->n, uo_nn_function_sigmoid);
}

static uo_avx_float uo_nn_function_sigmoid_d(__m256 avx_float)
{
  __m256 sigmoid = uo_nn_function_sigmoid(avx_float);
  __m256 ones = _mm256_set1_ps(1.0f);
  __m256 sub = _mm256_sub_ps(ones, sigmoid);
  return _mm256_mul_ps(sigmoid, sub);
}

static void uo_nn_node_sigmoid__backward(uo_nn_node **graph)
{
  uo_nn_node_out_1f *node = (uo_nn_node_out_1f *)(void *)graph[0];
  uo_nn_node_out_1f *input = (uo_nn_node_out_1f *)(void *)graph[1];
  uo_vec_mapfunc_mul_ps(input->A, node->dA, input->dA, node->m * node->n, uo_nn_function_sigmoid_d);
}

#pragma endregion

#pragma region uo_nn_node_swish

static uo_avx_float uo_nn_function_swish(__m256 avx_float)
{
  __m256 sigmoid = uo_nn_function_sigmoid(avx_float);
  return _mm256_mul_ps(avx_float, sigmoid);
}

static void uo_nn_node_swish__forward(uo_nn_node **graph)
{
  uo_nn_node_out_1f *node = (uo_nn_node_out_1f *)(void *)graph[0];
  uo_nn_node_out_1f *input = (uo_nn_node_out_1f *)(void *)graph[1];
  uo_vec_mapfunc_ps(input->A, node->A, node->m * node->n, uo_nn_function_swish);
}

static uo_avx_float uo_nn_function_swish_d(__m256 avx_float)
{
  __m256 sigmoid = uo_nn_function_sigmoid(avx_float);
  __m256 swish = _mm256_mul_ps(avx_float, sigmoid);
  __m256 ones = _mm256_set1_ps(1.0f);
  __m256 sub = _mm256_sub_ps(ones, swish);
  __m256 mul = _mm256_mul_ps(sigmoid, sub);
  return _mm256_add_ps(swish, mul);
}

static void uo_nn_node_swish__backward(uo_nn_node **graph)
{
  uo_nn_node_out_1f *node = (uo_nn_node_out_1f *)(void *)graph[0];
  uo_nn_node_out_1f *input = (uo_nn_node_out_1f *)(void *)graph[1];
  uo_vec_mapfunc_mul_ps(input->A, node->dA, input->dA, node->m * node->n, uo_nn_function_swish_d);
}


static uo_nn_node *uo_nn_node_make_swish(va_list vlist)
{

}

#pragma endregion

uo_nn_node *uo_nn_node_make(const char *op_type, ...)
{
  va_list vlist;
  va_start(vlist, op_type);
  uo_nn_node *node;

  if (strcmp(op_type, "matmul"))
  {

  }
  else if (strcmp(op_type, "tanh"))
  {

  }
  else if (strcmp(op_type, "swish"))
  {
    node = uo_nn_node_make_swish(vlist);
  }
  else
  {
    node = NULL;
  }

  va_end(vlist);
}
