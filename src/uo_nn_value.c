#include "uo_nn.h"

#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <stdint.h>

typedef struct uo_nn_value uo_nn_value;

typedef void (uo_nn_value_backprop_function)(uo_nn_value *node);

typedef union uo_tensor_data {
  void *ptr;
  float *s;
  double *d;
  int32_t *i;
  uint32_t *u;
} uo_tensor_data;

typedef struct uo_tensor
{
  /*
    s = single precision float (32 bit)
    d = double precision float (64 bit)
    i = signed integer (32 bit)
    u = unsigned integer (32 bit)
  */
  char type;
  size_t dimension_count;
  size_t element_count;
  size_t *dim_sizes;
  uo_tensor_data data;
} uo_tensor;

typedef struct uo_nn_value
{
  const char *op;
  uo_tensor *tensor;
  uo_tensor_data grad;
  uo_nn_value_backprop_function *backward;
  uo_nn_value **children;
  size_t children_count;
} uo_nn_value;

void uo_nn_value_build_topo(uo_nn_value *self, uo_nn_value **topo, size_t *topo_count, uo_nn_value **visited, size_t *visited_count)
{
  for (size_t j = 0; j < *visited_count; ++j)
  {
    if (visited[j] == self)
    {
      return;
    }
  }

  visited[*visited_count++] = self;

  for (size_t i = 0; i < self->children_count; ++i)
  {
    uo_nn_value_build_topo(self->children[i], topo, topo_count, visited, visited_count);
  }

  topo[*topo_count++] = self;
}

uo_nn_value **uo_nn_value_create_graph(uo_nn_value *self, size_t *size)
{
  uo_nn_value **graph = malloc(*size * 2 * sizeof(uo_nn_value *));
  uo_nn_value **visited = graph + *size;
  *size = 0;
  size_t visited_count = 0;
  uo_nn_value_build_topo(self, graph, size, visited, visited_count);
  return graph;
}

void uo_nn_value_backward(uo_nn_value *nn_value)
{
  nn_value->backward(nn_value);
}

void uo_nn_value_graph_backward(uo_nn_value **graph, size_t size)
{
  uo_nn_value *nn_value = graph[--size];
  for (size_t i = 0; i < nn_value->tensor->element_count; ++i)
  {
    nn_value->grad.s[i] = 1.0f;
  }

  while (size)
  {
    uo_nn_value_backward(graph[--size]);
  }
}

uo_tensor *uo_tensor_create(char type, size_t dimension_count, size_t *dim_sizes)
{
  size_t base_size = sizeof(uo_tensor) + dimension_count * sizeof(size_t);
  size_t element_count = 1;

  for (size_t i = 0; i < dimension_count; ++i)
  {
    element_count *= dim_sizes[i];
  }

  size_t size = base_size;

  switch (type)
  {
    case 's':
      size += sizeof(float) * element_count;
      break;

    case 'd':
      size += sizeof(double) * element_count;
      break;

    case 'i':
      size += sizeof(int32_t) * element_count;
      break;

    case 'u':
      size += sizeof(uint32_t) * element_count;
      break;

    default:
      return NULL;
  }

  void *mem = malloc(size);
  uo_tensor *tensor = mem;
  tensor->dimension_count = dimension_count;
  tensor->element_count = element_count;
  tensor->data.ptr = ((char *)mem) + base_size;

  for (size_t i = 0; i < dimension_count; ++i)
  {
    tensor->dim_sizes[i] = dim_sizes[i];
  }

  return tensor;
}

void uo_tensor_set(uo_tensor *tensor, size_t index, size_t offset, size_t count, const void *data)
{
  offset = uo_max(offset, 1);

  switch (tensor->type)
  {
    case 's': {
      const float *s = data;
      for (size_t i = index; i < count; ++i)
      {
        tensor->data.s[i * offset] = s[i];
      }
      return;
    }

    case 'd': {
      const double *d = data;
      for (size_t i = index; i < count; ++i)
      {
        tensor->data.d[i * offset] = d[i];
      }
      return;
    }

    case 'i': {
      const int32_t *int32 = data;
      for (size_t i = index; i < count; ++i)
      {
        tensor->data.i[i * offset] = int32[i];
      }
      return;
    }

    case 'u': {
      const uint32_t *u = data;
      for (size_t i = index; i < count; ++i)
      {
        tensor->data.u[i * offset] = u[i];
      }
      return;
    }
  }
}

void uo_tensor_set1(uo_tensor *tensor, size_t index, size_t offset, size_t count, const void *value)
{
  ++offset;

  switch (tensor->type)
  {
    case 's': {
      float s = *(float *)value;
      for (size_t i = index; i < count; ++i)
      {
        tensor->data.s[i * offset] = s;
      }
      return;
    }

    case 'd': {
      double d = *(double *)value;
      for (size_t i = index; i < count; ++i)
      {
        tensor->data.d[i * offset] = d;
      }
      return;
    }

    case 'i': {
      int32_t int32 = *(int32_t *)value;
      for (size_t i = index; i < count; ++i)
      {
        tensor->data.i[i * offset] = int32;
      }
      return;
    }

    case 'u': {
      uint32_t u = *(uint32_t *)value;
      for (size_t i = index; i < count; ++i)
      {
        tensor->data.u[i * offset] = u;
      }
      return;
    }
  }
}

uo_nn_value *uo_nn_value_create(uo_tensor *tensor, const char *op, size_t children_count)
{
  size_t base_size = sizeof(uo_nn_value);
  size_t children_size = sizeof(uo_nn_value) * children_count;
  size_t elements_size;

  switch (tensor->type)
  {
    case 's':
      elements_size = sizeof(float) * tensor->element_count;
      break;

    case 'd':
      elements_size = sizeof(double) * tensor->element_count;
      break;

    case 'i':
      elements_size = sizeof(int32_t) * tensor->element_count;
      break;

    case 'u':
      elements_size = sizeof(uint32_t) * tensor->element_count;
      break;

    default:
      return NULL;
  }

  void *mem = calloc(1, base_size + children_size + elements_size);
  uo_nn_value *value = mem;
  value->op = op;
  value->tensor = tensor;

  mem = (void *)(((char *)mem) + base_size);
  value->grad.ptr = mem;

  mem = (void *)(((char *)mem) + elements_size);

  if (children_count > 0)
  {
    value->children_count = children_count;
    value->children = mem;
  }

  return value;
}

#pragma region MatMul

void uo_nn_value_op_backward_matmul(uo_nn_value *self)
{
  uo_nn_value *a = self->children[0];
  uo_nn_value *b = self->children[1];
  uo_nn_value *c = self;

  float *A = a->tensor->data.s;
  float *A_grad = a->grad.s;
  size_t m_A = a->tensor->dim_sizes[0];
  size_t n_A = a->tensor->dim_sizes[1];

  float *B = b->tensor->data.s;
  float *B_grad = b->grad.s;
  size_t m_B = b->tensor->dim_sizes[0];
  size_t n_B = b->tensor->dim_sizes[1];

  float *C_grad = c->grad.s;
  size_t m_C = c->tensor->dim_sizes[0];
  size_t n_C = c->tensor->dim_sizes[1];

  uo_gemm(true, true, m_B, n_B, m_A, 1.0f,
    A, m_A,
    C_grad, m_C,
    0.0f,
    B_grad, n_B);

  uo_gemm(true, true, m_A, n_A, m_B, 1.0f,
    B, m_B,
    C_grad, m_C,
    0.0f,
    A_grad, n_A);
}

uo_nn_value *uo_nn_value_op_matmul(uo_nn_value *a, uo_nn_value *b, uo_nn_value *c)
{
  if (c == NULL)
  {
    uo_tensor *C = uo_tensor_create('s', 2, (size_t[]) {
      a->tensor->dim_sizes[0],
        b->tensor->dim_sizes[1]
    });

    c = uo_nn_value_create(C, "MatMul", 2);
  }

  float *A = a->tensor->data.s;
  size_t m_A = a->tensor->dim_sizes[0];
  size_t n_A = a->tensor->dim_sizes[1];

  float *B = b->tensor->data.s;
  size_t m_B = b->tensor->dim_sizes[0];
  size_t n_B = b->tensor->dim_sizes[1];

  float *C = c->tensor->data.s;
  size_t m_C = c->tensor->dim_sizes[0];
  size_t n_C = c->tensor->dim_sizes[1];

  uo_gemm(false, false, m_C, n_C, n_A, 1.0f,
    A, n_A,
    B, n_B,
    0.0f,
    C, n_C);

  c->backward = uo_nn_value_op_backward_matmul;
  c->children[0] = a;
  c->children[1] = b;

  return c;
}

#pragma endregion

#pragma region Add

void uo_nn_value_op_backward_add(uo_nn_value *self)
{
  uo_nn_value *a = self->children[0];
  uo_nn_value *b = self->children[1];
  uo_nn_value *c = self;

  float *A = a->tensor->data.s;
  float *A_grad = a->grad.s;
  size_t m_A = a->tensor->dim_sizes[0];
  size_t n_A = a->tensor->dim_sizes[1];

  float *B = b->tensor->data.s;
  float *B_grad = b->grad.s;
  size_t m_B = b->tensor->dim_sizes[0];
  size_t n_B = b->tensor->dim_sizes[1];

  float *C_grad = c->grad.s;
  size_t m_C = c->tensor->dim_sizes[0];
  size_t n_C = c->tensor->dim_sizes[1];

  for (size_t i = 0; i < a->tensor->element_count; ++i)
  {
    A_grad[i] += C_grad[i];
    B_grad[i] += C_grad[i];
  }
}

uo_nn_value *uo_nn_value_op_add(uo_nn_value *a, uo_nn_value *b, uo_nn_value *c)
{
  if (c == NULL)
  {
    uo_tensor *C = uo_tensor_create('s', 2, (size_t[]) {
      a->tensor->dim_sizes[0],
        b->tensor->dim_sizes[1]
    });

    c = uo_nn_value_create(C, "Add", 2);
  }

  float *A = a->tensor->data.s;
  size_t m_A = a->tensor->dim_sizes[0];
  size_t n_A = a->tensor->dim_sizes[1];

  float *B = b->tensor->data.s;
  size_t m_B = b->tensor->dim_sizes[0];
  size_t n_B = b->tensor->dim_sizes[1];

  float *C = c->tensor->data.s;
  size_t m_C = c->tensor->dim_sizes[0];
  size_t n_C = c->tensor->dim_sizes[1];

  for (size_t i = 0; i < a->tensor->element_count; ++i)
  {
    C[i] = A[i] + B[i];
  }

  c->backward = uo_nn_value_op_backward_matmul;
  c->children[0] = a;
  c->children[1] = b;

  return c;
}

#pragma endregion

#pragma region Relu

uo_avx_float uo_nn_function_relu(__m256 avx_float)
{
  __m256 zeros = _mm256_setzero_ps();
  return _mm256_max_ps(avx_float, zeros);
}

uo_avx_float uo_nn_function_relu_d(__m256 avx_float)
{
  __m256 zeros = _mm256_setzero_ps();
  __m256 mask = _mm256_cmp_ps(avx_float, zeros, _CMP_GT_OQ);
  __m256 ones = _mm256_set1_ps(1.0f);
  return _mm256_max_ps(mask, ones);
}

void uo_nn_value_op_backward_relu(uo_nn_value *self)
{
  uo_nn_value *x = self->children[0];
  uo_vec_mapfunc_ps(x->tensor->data.s, x->grad.s, x->tensor->element_count, uo_nn_function_relu_d);
}

uo_nn_value *uo_nn_value_op_relu(uo_nn_value *x, uo_nn_value *y)
{
  if (y == NULL)
  {
    uo_tensor *Y = uo_tensor_create('s', 2, (size_t[]) {
      x->tensor->dim_sizes[0],
        x->tensor->dim_sizes[1]
    });

    y = uo_nn_value_create(Y, "Relu", 2);
  }

  uo_vec_mapfunc_ps(x->tensor->data.s, y->tensor->data.s, x->tensor->element_count, uo_nn_function_relu);

  y->backward = uo_nn_value_op_backward_relu;
  y->children[0] = x;

  return y;
}

#pragma endregion

#pragma region loss MSE


uo_avx_float uo_nn_loss_function_mean_squared_error(uo_avx_float y_true, uo_avx_float y_pred)
{
  __m256 sub = _mm256_sub_ps(y_pred, y_true);
  __m256 mul = _mm256_mul_ps(sub, sub);
  return mul;
}

uo_avx_float uo_nn_loss_function_mean_squared_error_d(uo_avx_float y_true, uo_avx_float y_pred)
{
  __m256 twos = _mm256_set1_ps(2.0f);
  __m256 sub = _mm256_sub_ps(y_pred, y_true);
  return _mm256_mul_ps(twos, sub);
}

void uo_nn_loss_grad_mse(uo_nn_value *y_pred, float *y_true)
{
  float *grad = y_pred->grad.s;
  uo_vec_map2func_ps(y_true, y_pred->tensor->data.s, grad, y_pred->tensor->element_count, uo_nn_loss_function_mean_squared_error_d);
  uo_vec_mul1_ps(grad, 1.0f / (float)y_pred->tensor->dim_sizes[0], grad, y_pred->tensor->element_count);
}

float uo_nn_loss_mse(uo_nn_value *y_pred, float *y_true)
{
  float *losses = uo_alloca(sizeof(float) * y_pred->tensor->element_count);
  uo_vec_map2func_ps(y_true, y_pred->tensor->data.s, losses, y_pred->tensor->element_count, uo_nn_loss_function_mean_squared_error);
  float loss = uo_vec_mean_ps(losses, y_pred->tensor->element_count);
  return loss;
}

#pragma endregion

bool uo_test_nn_value()
{
  uo_tensor *X = uo_tensor_create('s', 2, (size_t[]) { 2, 3 });
  uo_tensor_set(X, 0, 0, 6, (float[]) {
    3.0, 2.0, 1.0,
      2.0, 2.0, 1.0
  });
  uo_nn_value *x = uo_nn_value_create(X, NULL, 0);

  uo_tensor *W1 = uo_tensor_create('s', 2, (size_t[]) { 3, 1 });
  uo_tensor_set(W1, 0, 0, 3, (float[]) {
    -1.0,
      2.0,
      3.0,
      3.5
  });
  uo_nn_value *w1 = uo_nn_value_create(W1, NULL, 0);

  uo_tensor *B1 = uo_tensor_create('s', 2, (size_t[]) { 1, 1 });
  uo_tensor_set(W1, 0, 0, 1, (float[]) {
    0.5
  });
  uo_nn_value *b1 = uo_nn_value_create(b1, NULL, 0);

  uo_nn_value *xw1 = uo_nn_value_op_matmul(x, w1, NULL);
  uo_nn_value *z1 = uo_nn_value_op_add(xw1, b1, NULL);

  uo_nn_value *a1 = uo_nn_value_op_relu(z1, NULL);


  uo_tensor *y_true = uo_tensor_create('s', 2, (size_t[]) { 2, 1 });
  uo_tensor_set(y_true, 0, 0, 2, (float[]) {
    0.5,
    0.5
  });

  uo_nn_loss_grad_mse(a1, y_true->data.s);

  size_t graph_size = 3;
  uo_nn_value **graph = uo_nn_value_create_graph(a1, &graph_size);
  uo_nn_value_graph_backward(graph, graph_size);
}


