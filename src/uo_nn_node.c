#include "uo_nn.h"
#include "uo_nn_node.h"

#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <stdint.h>

void uo_nn_node_build_topo(uo_nn_node *self, uo_nn_node **topo, size_t *topo_count, uo_nn_node **visited, size_t *visited_count)
{
  for (size_t j = 0; j < *visited_count; ++j)
  {
    if (visited[j] == self)
    {
      return;
    }
  }

  visited[(*visited_count)++] = self;

  for (size_t i = 0; i < self->children_count; ++i)
  {
    uo_nn_node_build_topo(self->children[i], topo, topo_count, visited, visited_count);
  }

  topo[(*topo_count)++] = self;
}

uo_nn_node **uo_nn_node_create_graph(uo_nn_node *self, size_t *size)
{
  uo_nn_node **graph = malloc(*size * 2 * sizeof(uo_nn_node *));
  uo_nn_node **visited = graph + *size;
  *size = 0;
  size_t visited_count = 0;
  uo_nn_node_build_topo(self, graph, size, visited, &visited_count);
  return graph;
}

uo_tensor *uo_tensor_create(char type, size_t dimension_count, size_t *dim_sizes)
{
  size_t base_size = sizeof(uo_tensor) + dimension_count * sizeof(size_t);
  size_t element_count = 1;
  size_t element_size;

  for (size_t i = 0; i < dimension_count; ++i)
  {
    element_count *= dim_sizes[i];
  }

  size_t size = base_size;

  switch (type)
  {
    case 's':
      element_size = sizeof(float);
      break;

    case 'd':
      element_size = sizeof(double);
      break;

    case 'i':
      element_size = sizeof(int32_t);
      break;

    case 'u':
      element_size = sizeof(uint32_t);
      break;

    default:
      return NULL;
  }

  size += element_size * element_count;

  void *mem = malloc(size);
  uo_tensor *tensor = mem;
  tensor->type = type;
  tensor->dimension_count = dimension_count;
  tensor->element_size = element_size;
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

void uo_tensor_set_rand_s(uo_tensor *tensor, size_t index, size_t offset, size_t count, float min, float max)
{
  ++offset;

  for (size_t i = index; i < count; ++i)
  {
    float rand_s = uo_rand_between(min, max);

    tensor->data.s[i * offset] = rand_s;
  }
}

void uo_tensor_set_rand(uo_tensor *tensor, size_t index, size_t offset, size_t count, const void *min, const void *max)
{
  ++offset;

  switch (tensor->type)
  {
    case 's': {
      for (size_t i = index; i < count; ++i)
      {
        float min_s = *(float *)min;
        float max_s = *(float *)max;
        float rand_s = uo_rand_between(min_s, max_s);

        tensor->data.s[i * offset] = rand_s;
      }
      return;
    }

    case 'd': {
      for (size_t i = index; i < count; ++i)
      {
        double min_d = *(double *)min;
        double max_d = *(double *)max;
        double rand_d = uo_rand_between(min_d, max_d);

        tensor->data.d[i * offset] = rand_d;
      }
      return;
    }

    case 'i': {
      for (size_t i = index; i < count; ++i)
      {
        int32_t min_i32 = *(int32_t *)min;
        int32_t max_i32 = *(int32_t *)max;
        int32_t rand_i32 = (int32_t)uo_rand_between(min_i32, max_i32);

        tensor->data.i[i * offset] = rand_i32;
      }
      return;
    }

    case 'u': {
      for (size_t i = index; i < count; ++i)
      {
        uint32_t min_u32 = *(uint32_t *)min;
        uint32_t max_u32 = *(uint32_t *)max;
        uint32_t rand_u32 = (uint32_t)uo_rand_between(min_u32, max_u32);

        tensor->data.i[i * offset] = rand_u32;
      }
      return;
    }
  }
}

uo_nn_node *uo_nn_node_create(char *name, uo_tensor *tensor, const char *op_type, size_t children_count, size_t attributes_size)
{
  size_t base_size = sizeof(uo_nn_node);
  size_t children_size = sizeof(uo_nn_node *) * children_count;
  size_t name_size = strlen(name) + 1;
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

  void *mem = calloc(1, base_size + name_size + children_size + elements_size + attributes_size);
  uo_nn_node *node = mem;
  node->op_type = op_type;
  node->tensor = tensor;
  node->children_count = children_count;

  mem = (void *)(((char *)mem) + base_size + children_size);
  node->name = mem;
  memcpy(node->name, name, name_size);

  mem = (void *)(((char *)mem) + name_size);
  node->grad.ptr = mem;

  mem = (void *)(((char *)mem) + elements_size);
  node->attributes = mem;

  return node;
}

uo_nn_node *uo_nn_node_input_tensor_create(const char *name, char type, size_t dimension_count, size_t *dim_sizes)
{
  uo_tensor *tensor = uo_tensor_create(type, dimension_count, dim_sizes);
  return uo_nn_node_create(name, tensor, "Input", 0, 0);
}

void uo_print_nn_graph(FILE *const fp, uo_nn_node **graph, size_t size)
{
  fprintf(fp, "graph {\n");

  for (size_t i = 0; i < size; ++i)
  {
    uo_nn_node *nn_value = graph[i];

    fprintf(fp, "node {\n");
    fprintf(fp, "- op_type: %s\n", nn_value->op_type ? nn_value->op_type : "-");
    fprintf(fp, "- children_count: %d\n", nn_value->children_count);

    if (nn_value->children_count)
    {
      fprintf(fp, "- children: [");
      for (size_t j = 0; j < nn_value->children_count; ++j)
      {
        if (j)
        {
          fprintf(fp, ",");
        }

        for (size_t k = 0; k < i; ++k)
        {
          if (graph[k] == nn_value->children[j])
          {
            fprintf(fp, " $%d", k);
            break;
          }
        }
      }
      fprintf(fp, " ]\n");
    }

    fprintf(fp, "- type: %c\n", nn_value->tensor->type);
    fprintf(fp, "- data: ");

    // TODO: handle different types
    uo_print_matrix(fp, nn_value->tensor->data.s, nn_value->tensor->dim_sizes[0], nn_value->tensor->dim_sizes[1]);

    // TODO: print attributes

    fprintf(fp, "}\n");
  }

  fprintf(fp, "}\n");
}

void uo_nn_node_op_print_tensor(uo_nn_node **node, uo_nn_node **graph, FILE *const fp)
{
  uo_tensor *tensor = (*node)->tensor;

  fprintf(fp, "\n  - type: %c", tensor->type);
  fprintf(fp, "\n  - dimension_count: %d", tensor->dimension_count);
  fprintf(fp, "\n  - dim_sizes: %d", tensor->dim_sizes[0]);
  for (size_t i = 1; i < tensor->dimension_count; ++i)
  {
    fprintf(fp, " x %d", tensor->dim_sizes[i]);
  }

  fprintf(fp, "\n  - data:");

  switch (tensor->type)
  {
    case 's': {
      for (size_t i = 0; i < tensor->element_count; ++i)
      {
        fprintf(fp, " %.9g", tensor->data.s[i]);
      }
    }

    case 'd': {
      for (size_t i = 0; i < tensor->element_count; ++i)
      {
        fprintf(fp, " %.17g", tensor->data.d[i]);
      }
    }

    case 'i': {
      for (size_t i = 0; i < tensor->element_count; ++i)
      {
        fprintf(fp, " %d", tensor->data.i[i]);
      }
    }

    case 'u': {
      for (size_t i = 0; i < tensor->element_count; ++i)
      {
        fprintf(fp, " %d", tensor->data.u[i]);
      }
    }
  }
}

#pragma region GemmAMask

void uo_nn_node_op_reset_gemm_a_mask(uo_nn_node *self)
{
  memset(self->tensor->data.ptr, 0, self->tensor->element_count * self->tensor->element_size);
}

void uo_nn_node_op_forward_gemm_a_mask(uo_nn_node *self)
{
  uo_nn_node *a = self->children[0];
  uo_nn_node *b = self->children[1];
  uo_nn_node *c = self;

  uint32_t *A = a->tensor->data.u;
  size_t m_A = a->tensor->dim_sizes[0];
  size_t n_A = a->tensor->dim_sizes[1];

  float *B = b->tensor->data.s;
  size_t m_B = b->tensor->dim_sizes[0];
  size_t n_B = b->tensor->dim_sizes[1];

  float *C = c->tensor->data.s;
  size_t m_C = c->tensor->dim_sizes[0];
  size_t n_C = c->tensor->dim_sizes[1];

  struct
  {
    float alpha;
    float beta;
    bool ta;
    bool tb;
  } *attributes = c->attributes;

  uo_gemm_a_mask(attributes->ta, attributes->tb, m_C, n_C, n_A, attributes->alpha,
    A, n_A,
    B, n_B,
    attributes->beta,
    C, n_C);
}

void uo_nn_node_op_backward_gemm_a_mask(uo_nn_node *self)
{
  uo_nn_node *a = self->children[0];
  uo_nn_node *b = self->children[1];
  uo_nn_node *c = self;

  uint32_t *A = a->tensor->data.u;
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

  struct
  {
    float alpha;
    float beta;
    bool ta;
    bool tb;
  } *attributes = c->attributes;

  uo_gemm_a_mask(!attributes->ta, false, m_B, n_B, m_A, attributes->alpha,
    A, n_A,
    C_grad, n_C,
    attributes->beta,
    B_grad, n_B);

  uo_gemm(false, !attributes->tb, m_A, n_A, n_B, attributes->alpha,
    C_grad, n_C,
    B, n_B,
    attributes->beta,
    A_grad, n_A);
}

uo_nn_node *uo_nn_node_op_gemm_a_mask(uo_nn_node *a, uo_nn_node *b, float alpha, float beta, bool ta, bool tb)
{
  uo_tensor *C = uo_tensor_create('s', 2, (size_t[]) {
    a->tensor->dim_sizes[0],
      b->tensor->dim_sizes[1]
  });

  struct
  {
    float alpha;
    float beta;
    bool ta;
    bool tb;
  } attributes = {
    .alpha = alpha,
    .beta = beta,
    .ta = ta,
    .tb = tb
  };

  uo_nn_node *c = uo_nn_node_create(C, "GemmAMask", 2, sizeof attributes);
  memcpy(c->attributes, &attributes, sizeof attributes);

  c->reset = uo_nn_node_op_reset_gemm_a_mask;
  c->forward = uo_nn_node_op_forward_gemm_a_mask;
  c->backward = uo_nn_node_op_backward_gemm_a_mask;
  c->children[0] = a;
  c->children[1] = b;

  return c;
}

#pragma endregion

#pragma region Gemm

void uo_nn_node_op_reset_gemm(uo_nn_node *self)
{
  memset(self->tensor->data.ptr, 0, self->tensor->element_count * self->tensor->element_size);
}

void uo_nn_node_op_forward_gemm(uo_nn_node *self)
{
  uo_nn_node *a = self->children[0];
  uo_nn_node *b = self->children[1];
  uo_nn_node *c = self;

  float *A = a->tensor->data.s;
  size_t m_A = a->tensor->dim_sizes[0];
  size_t n_A = a->tensor->dim_sizes[1];

  float *B = b->tensor->data.s;
  size_t m_B = b->tensor->dim_sizes[0];
  size_t n_B = b->tensor->dim_sizes[1];

  float *C = c->tensor->data.s;
  size_t m_C = c->tensor->dim_sizes[0];
  size_t n_C = c->tensor->dim_sizes[1];

  struct
  {
    float alpha;
    float beta;
    bool ta;
    bool tb;
  } *attributes = c->attributes;

  uo_gemm(attributes->ta, attributes->tb, m_C, n_C, n_A, attributes->alpha,
    A, n_A,
    B, n_B,
    attributes->beta,
    C, n_C);
}

void uo_nn_node_op_backward_gemm(uo_nn_node *self)
{
  uo_nn_node *a = self->children[0];
  uo_nn_node *b = self->children[1];
  uo_nn_node *c = self;

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

  struct
  {
    float alpha;
    float beta;
    bool ta;
    bool tb;
  } *attributes = c->attributes;

  uo_gemm(!attributes->ta, false, m_B, n_B, m_A, attributes->alpha,
    A, n_A,
    C_grad, n_C,
    attributes->beta,
    B_grad, n_B);

  uo_gemm(false, !attributes->tb, m_A, n_A, n_B, attributes->alpha,
    C_grad, n_C,
    B, n_B,
    attributes->beta,
    A_grad, n_A);
}

void uo_nn_node_op_print_gemm(uo_nn_node **node, uo_nn_node **graph, FILE *const fp)
{
  uo_nn_node *c = *node;

  fprintf(fp, "\n  - children_count: %d", c->children_count);
  fprintf(fp, "\n  - children:");

  for (size_t i = 0; i < c->children_count; ++i)
  {
    uo_nn_node **child = graph;
    while (child < node)
    {
      if (*child == c->children[i])
      {
        fprintf(fp, " $%d", graph - child);
        break;
      }

      ++child;
    }
  }

  struct
  {
    float alpha;
    float beta;
    bool ta;
    bool tb;
  } *attributes = c->attributes;

  fprintf(fp, "\n  - attributes: ");
  fprintf(fp, "\n    - alpha: %.9g", attributes->alpha);
  fprintf(fp, "\n    - beta: %.9g", attributes->beta);
  fprintf(fp, "\n    - ta: %s", attributes->ta ? "true" : "false");
  fprintf(fp, "\n    - tb: %s", attributes->tb ? "true" : "false");
}

void uo_nn_node_op_parse_gemm(uo_nn_node **node, uo_nn_node **graph, const char **ptr)
{
  size_t children_count;
  const char *ptr_children_count = strstr(*ptr, "  - children_count: ");
  sscanf(ptr_children_count, "  - children_count: &d", &children_count);

  uo_nn_node **children = uo_alloca(children_count * sizeof(uo_nn_node *));
  const char *ptr_children = strstr(*ptr, "  - children: ");
  ptr_children += strlen("  - children: $");
  for (size_t i = 0; i < children_count; ++i)
  {
    size_t child_index = strtoul(ptr_children, &ptr_children, 10);
    children[i] = graph[child_index];
    ptr_children += strlen(" $");
  }

  const char *ptr_attributes = strstr(ptr, "  - attributes: ");

  float alpha;
  const char *ptr_alpha = strstr(ptr_attributes, "    - alpha: ");
  sscanf(ptr_alpha, "    - alpha: %f", &alpha);

  float beta;
  const char *ptr_beta = strstr(ptr_attributes, "    - beta: ");
  sscanf(ptr_beta, "    - beta: %f", &beta);

  const char *ptr_ta = strstr(ptr_attributes, "    - ta: ");
  bool ta = ptr_ta[11] == 't';

  bool tb;
  const char *ptr_tb = strstr(ptr_attributes, "    - tb: ");
  bool tb = ptr_tb[11] == 't';

  *node = uo_nn_node_op_gemm(children[0], children[1], alpha, beta, ta, tb);
}

uo_nn_node *uo_nn_node_op_gemm(uo_nn_node *a, uo_nn_node *b, float alpha, float beta, bool ta, bool tb)
{
  uo_tensor *C = uo_tensor_create('s', 2, (size_t[]) {
    a->tensor->dim_sizes[0],
      b->tensor->dim_sizes[1]
  });

  struct
  {
    float alpha;
    float beta;
    bool ta;
    bool tb;
  } attributes = {
    .alpha = alpha,
    .beta = beta,
    .ta = ta,
    .tb = tb
  };

  uo_nn_node *c = uo_nn_node_create(C, "Gemm", 2, sizeof attributes);
  memcpy(c->attributes, &attributes, sizeof attributes);

  c->reset = uo_nn_node_op_reset_gemm;
  c->forward = uo_nn_node_op_forward_gemm;
  c->backward = uo_nn_node_op_backward_gemm;
  c->print = uo_nn_node_op_print_gemm;
  c->children[0] = a;
  c->children[1] = b;

  return c;
}

#pragma endregion

#pragma region MatMul

void uo_nn_node_op_forward_matmul(uo_nn_node *self)
{
  uo_nn_node *a = self->children[0];
  uo_nn_node *b = self->children[1];
  uo_nn_node *c = self;

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
}

void uo_nn_node_op_backward_matmul(uo_nn_node *self)
{
  uo_nn_node *a = self->children[0];
  uo_nn_node *b = self->children[1];
  uo_nn_node *c = self;

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

  uo_gemm(true, false, m_B, n_B, m_A, 1.0f,
    A, n_A,
    C_grad, n_C,
    0.0f,
    B_grad, n_B);

  uo_gemm(false, true, m_A, n_A, n_B, 1.0f,
    C_grad, n_C,
    B, n_B,
    0.0f,
    A_grad, n_A);
}

uo_nn_node *uo_nn_node_op_matmul(uo_nn_node *a, uo_nn_node *b)
{
  uo_tensor *C = uo_tensor_create('s', 2, (size_t[]) {
    a->tensor->dim_sizes[0],
      b->tensor->dim_sizes[1]
  });
  uo_nn_node *c = uo_nn_node_create(C, "MatMul", 2, 0);

  c->forward = uo_nn_node_op_forward_matmul;
  c->backward = uo_nn_node_op_backward_matmul;
  c->children[0] = a;
  c->children[1] = b;

  return c;
}

#pragma endregion

#pragma region Add

void uo_nn_node_op_forward_add(uo_nn_node *self)
{
  uo_nn_node *a = self->children[0];
  uo_nn_node *b = self->children[1];
  uo_nn_node *c = self;

  float *A = a->tensor->data.s;
  size_t m_A = a->tensor->dim_sizes[0];
  size_t n_A = a->tensor->dim_sizes[1];

  float *B = b->tensor->data.s;
  size_t m_B = b->tensor->dim_sizes[0];
  size_t n_B = b->tensor->dim_sizes[1];

  float *C = c->tensor->data.s;
  size_t m_C = c->tensor->dim_sizes[0];
  size_t n_C = c->tensor->dim_sizes[1];

  if (m_A == m_B && n_A == n_B)
  {
    for (size_t i = 0; i < c->tensor->element_count; ++i)
    {
      C[i] = A[i] + B[i];
    }
  }
  else if (m_A == m_B && n_A == 1)
  {
    for (size_t i = 0; i < c->tensor->element_count; ++i)
    {
      C[i] = A[i % m_A] + B[i];
    }
  }
  else if (n_A == n_B && m_A == 1)
  {
    for (size_t i = 0; i < c->tensor->element_count; ++i)
    {
      C[i] = A[i % n_A] + B[i];
    }
  }
  else if (m_A == m_B && n_B == 1)
  {
    for (size_t i = 0; i < c->tensor->element_count; ++i)
    {
      C[i] = A[i] + B[i % m_B];
    }
  }
  else if (n_A == n_B && m_B == 1)
  {
    for (size_t i = 0; i < c->tensor->element_count; ++i)
    {
      C[i] = A[i] + B[i % n_B];
    }
  }
  else
  {
    // sizes not compatible
    exit(-1);
  }
}

void uo_nn_node_op_backward_add(uo_nn_node *self)
{
  uo_nn_node *a = self->children[0];
  uo_nn_node *b = self->children[1];
  uo_nn_node *c = self;

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

  if (m_A == m_B && n_A == n_B)
  {
    for (size_t i = 0; i < c->tensor->element_count; ++i)
    {
      A_grad[i] += C_grad[i];
      B_grad[i] += C_grad[i];
    }
  }
  else if (m_A == m_B && n_A == 1)
  {
    for (size_t i = 0; i < c->tensor->element_count; ++i)
    {
      A_grad[i % m_A] += C_grad[i];
      B_grad[i] += C_grad[i];
    }
  }
  else if (n_A == n_B && m_A == 1)
  {
    for (size_t i = 0; i < c->tensor->element_count; ++i)
    {
      A_grad[i % n_A] += C_grad[i];
      B_grad[i] += C_grad[i];
    }
  }
  else if (m_A == m_B && n_B == 1)
  {
    for (size_t i = 0; i < c->tensor->element_count; ++i)
    {
      A_grad[i] += C_grad[i];
      B_grad[i % m_B] += C_grad[i];
    }
  }
  else if (n_A == n_B && m_B == 1)
  {
    for (size_t i = 0; i < c->tensor->element_count; ++i)
    {
      A_grad[i] += C_grad[i];
      B_grad[i % n_B] += C_grad[i];
    }
  }
  else
  {
    // sizes not compatible
    exit(-1);
  }
}

uo_nn_node *uo_nn_node_op_add(uo_nn_node *a, uo_nn_node *b)
{
  uo_tensor *C = uo_tensor_create('s', 2, (size_t[]) {
    uo_max(a->tensor->dim_sizes[0], b->tensor->dim_sizes[0]),
      uo_max(a->tensor->dim_sizes[1], b->tensor->dim_sizes[1])
  });
  uo_nn_node *c = uo_nn_node_create(C, "Add", 2, 0);

  c->forward = uo_nn_node_op_forward_add;
  c->backward = uo_nn_node_op_backward_add;
  c->children[0] = a;
  c->children[1] = b;

  return c;
}

#pragma endregion


#pragma region Concat

void uo_nn_node_op_forward_concat(uo_nn_node *self)
{
  uo_nn_node *c = self;

  float *C = c->tensor->data.s;
  size_t m_C = c->tensor->dim_sizes[0];
  size_t n_C = c->tensor->dim_sizes[1];

  if (m_C > c->children[0]->tensor->dim_sizes[0]) // axis == 0
  {
    for (size_t k = 0; k < c->children_count; ++k)
    {
      size_t size = c->children[k]->tensor->element_count * c->children[k]->tensor->element_size;
      memcpy(C, c->children[k]->tensor->data.ptr, size);
    }
  }
  else // axis == 1
  {
    char **ptrs = uo_alloca(c->children_count * sizeof * ptrs);
    size_t *row_sizes = uo_alloca(c->children_count * sizeof * row_sizes);

    for (size_t k = 0; k < c->children_count; ++k)
    {
      ptrs[k] = c->children[k]->tensor->data.ptr;
      row_sizes[k] = c->children[k]->tensor->dim_sizes[1] * c->children[k]->tensor->element_size;
    }

    char *ptr_C = c->tensor->data.ptr;

    for (size_t i = 0; i < m_C; ++i)
    {
      for (size_t k = 0; k < c->children_count; ++k)
      {
        ptrs[k] = c->children[k]->tensor->data.ptr;
        row_sizes[k] = c->children[k]->tensor->dim_sizes[1] * c->children[k]->tensor->element_size;

        memcpy(ptr_C, ptrs[k], row_sizes[k]);
        ptrs[k] += row_sizes[k];
        ptr_C += row_sizes[k];
      }
    }
  }
}

void uo_nn_node_op_backward_concat(uo_nn_node *self)
{
  uo_nn_node *c = self;

  float *C_grad = c->grad.s;
  size_t m_C = c->tensor->dim_sizes[0];
  size_t n_C = c->tensor->dim_sizes[1];

  if (m_C > self->children[0]->tensor->dim_sizes[0]) // axis == 0
  {
    for (size_t k = 0; k < c->children_count; ++k)
    {
      float *grad = c->children[k]->grad.s;
      size_t m = c->children[k]->tensor->dim_sizes[0];
      for (size_t i = 0; i < m; ++i)
      {
        *grad++ = *C_grad++;
      }
    }
  }
  else // axis == 1
  {
    for (size_t i = 0; i < m_C; ++i)
    {
      for (size_t k = 0; k < c->children_count; ++k)
      {
        float *grad = c->children[k]->grad.s;
        size_t n = c->children[k]->tensor->dim_sizes[1];
        for (size_t j = 0; j < n; ++j)
        {
          *grad++ += *C_grad++;
        }
      }
    }
  }
}

uo_nn_node *uo_nn_node_op_concat(int axis, size_t count, uo_nn_node **values)
{
  size_t dim_sizes[2] =
  {
    values[0]->tensor->dim_sizes[0],
    values[0]->tensor->dim_sizes[1]
  };

  for (size_t i = 1; i < count; ++i)
  {
    dim_sizes[axis] += values[i]->tensor->dim_sizes[axis];
  }

  uo_tensor *C = uo_tensor_create('s', 2, dim_sizes);
  uo_nn_node *c = uo_nn_node_create(C, "Concat", count, 0);

  c->forward = uo_nn_node_op_forward_concat;
  c->backward = uo_nn_node_op_backward_concat;

  for (size_t i = 0; i < count; ++i)
  {
    c->children[i] = values[i];
  }

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
  return _mm256_and_ps(mask, ones);
}

void uo_nn_node_op_forward_relu(uo_nn_node *self)
{
  uo_nn_node *x = self->children[0];
  uo_nn_node *y = self;
  uo_vec_mapfunc_ps(x->tensor->data.s, y->tensor->data.s, x->tensor->element_count, uo_nn_function_relu);
}

void uo_nn_node_op_backward_relu(uo_nn_node *self)
{
  uo_nn_node *x = self->children[0];
  uo_nn_node *y = self;

  //uo_vec_mapfunc_mul_ps(y->tensor->data.s, y->grad.s, x->grad.s, y->tensor->element_count, uo_nn_function_relu_d);

  for (size_t i = 0; i < y->tensor->element_count; i++)
  {
    x->grad.s[i] += (y->tensor->data.s[i] > 0) * y->grad.s[i];
  }
}

uo_nn_node *uo_nn_node_op_relu(uo_nn_node *x)
{
  uo_tensor *Y = uo_tensor_create('s', 2, (size_t[]) {
    x->tensor->dim_sizes[0],
      x->tensor->dim_sizes[1]
  });
  uo_nn_node *y = uo_nn_node_create(Y, "Relu", 1, 0);

  y->forward = uo_nn_node_op_forward_relu;
  y->backward = uo_nn_node_op_backward_relu;
  y->children[0] = x;

  return y;
}

#pragma endregion

#pragma region Tanh

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

void uo_nn_node_op_forward_tanh(uo_nn_node *self)
{
  uo_nn_node *x = self->children[0];
  uo_nn_node *y = self;
  uo_vec_mapfunc_ps(x->tensor->data.s, y->tensor->data.s, x->tensor->element_count, uo_nn_function_tanh);
}

void uo_nn_node_op_backward_tanh(uo_nn_node *self)
{
  uo_nn_node *x = self->children[0];
  uo_nn_node *y = self;

  uo_vec_mapfunc_mul_ps(y->tensor->data.s, y->grad.s, x->grad.s, y->tensor->element_count, uo_nn_function_tanh_d);
}

uo_nn_node *uo_nn_node_op_tanh(uo_nn_node *x)
{
  uo_tensor *Y = uo_tensor_create('s', 2, (size_t[]) {
    x->tensor->dim_sizes[0],
      x->tensor->dim_sizes[1]
  });
  uo_nn_node *y = uo_nn_node_create(Y, "Tanh", 1, 0);

  y->forward = uo_nn_node_op_forward_tanh;
  y->backward = uo_nn_node_op_backward_tanh;
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

void uo_nn_loss_grad_mse(uo_nn_node *y_pred, float *y_true)
{
  float *grad = y_pred->grad.s;
  uo_vec_map2func_ps(y_true, y_pred->tensor->data.s, grad, y_pred->tensor->element_count, uo_nn_loss_function_mean_squared_error_d);
  uo_vec_mul1_ps(grad, 1.0f / (float)y_pred->tensor->dim_sizes[0], grad, y_pred->tensor->element_count);
}

float uo_nn_loss_mse(uo_nn_node *y_pred, float *y_true)
{
  float *losses = uo_alloca(sizeof(float) * y_pred->tensor->element_count);
  uo_vec_map2func_ps(y_true, y_pred->tensor->data.s, losses, y_pred->tensor->element_count, uo_nn_loss_function_mean_squared_error);
  float loss = uo_vec_mean_ps(losses, y_pred->tensor->element_count);
  return loss;
}

#pragma endregion

uo_nn_adam_params *uo_nn_node_adam_params_create(uo_nn_node *value)
{
  size_t size = sizeof(uo_nn_adam_params) + sizeof(float) * value->tensor->element_count * 4;
  void *mem = calloc(1, size);
  uo_nn_adam_params *params = mem;

  mem = (void *)(((char *)mem) + sizeof(uo_nn_adam_params));
  params->m = mem;

  mem = (void *)(((char *)mem) + sizeof(float) * value->tensor->element_count);
  params->m_hat = mem;

  mem = (void *)(((char *)mem) + sizeof(float) * value->tensor->element_count);
  params->v = mem;

  mem = (void *)(((char *)mem) + sizeof(float) * value->tensor->element_count);
  params->v_hat = mem;

  params->value = value;
  params->t = 1;

  params->lr = 1e-5;
  params->beta1 = 0.9;
  params->beta2 = 0.999;
  params->epsilon = 1e-8;
  params->weight_decay = 1e-4;

  return params;
}

void uo_nn_node_update_adam(uo_nn_adam_params *params)
{
  size_t count = params->value->tensor->element_count;
  float *value = params->value->tensor->data.s;
  float *grad = params->value->grad.s;

  float *m = params->m;
  float *v = params->v;
  float *m_hat = params->m_hat;
  float *v_hat = params->v_hat;
  float t = params->t;
  float lr = params->lr;
  float beta1 = params->beta1;
  float beta2 = params->beta2;
  float epsilon = params->epsilon;
  float weight_decay = params->weight_decay;

  for (size_t i = 0; i < count; ++i)
  {
    m[i] = beta1 * m[i] + (1.0f - beta1) * grad[i];
    v[i] = beta2 * v[i] + (1.0f - beta2) * (grad[i] * grad[i]);
    m_hat[i] = m[i] / (1.0f - powf(beta1, t));
    v_hat[i] = v[i] / (1.0f - powf(beta2, t));
    value[i] -= lr * m_hat[i] / (sqrtf(v_hat[i] + epsilon));

    // weight decay
    value[i] -= weight_decay * lr * value[i];
  }

  params->t++;
}

uo_nn_node *uo_nn_node_parse(const char *op_type, char **ptr)
{
  if (strcmp(op_type, "Gemm") == 0)
  {
    return uo_nn_node_op_parse_gemm(ptr);
  }

  exit(1);
}
