#include "uo_nn.h"

#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <stdint.h>

typedef struct uo_nn_value uo_nn_value;

typedef struct uo_nn_backprop_callback
{
  uo_nn_value *(*invoke)(uo_nn_value *self, uo_nn_value *other);
  uo_nn_value *other;
} uo_nn_backprop_callback;

typedef struct uo_tensor_dim_def {
  size_t size;
  size_t offset;
} uo_tensor_dim_def;

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
  uo_tensor_dim_def *dims;
  uo_tensor_data data;
} uo_tensor;

typedef struct uo_nn_value
{
  const char *op;
  uo_tensor *tensor;
  uo_tensor_data grad;
  uo_nn_backprop_callback backprob_callback;
} uo_nn_value;

uo_tensor *uo_tensor_create(char type, size_t dimension_count, uo_tensor_dim_def *dims)
{
  size_t base_size = sizeof(uo_tensor) + dimension_count * sizeof(uo_tensor_dim_def);
  size_t element_count = 1;

  for (size_t i = 0; i < dimension_count; ++i)
  {
    element_count *= dims[i].size + dims[i].offset;
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
    tensor->dims[i] = dims[i];
  }

  return tensor;
}

void uo_tensor_set(uo_tensor *tensor, size_t index, size_t offset, size_t count, const void *data)
{
  offset = uo_max(offset, 1);

  switch (tensor->type)
  {
    case 's': {
      float *s = data;
      for (size_t i = index; i < count; ++i)
      {
        tensor->data.s[i * offset] = s[i];
      }
      return;
    }

    case 'd': {
      double *d = data;
      for (size_t i = index; i < count; ++i)
      {
        tensor->data.d[i * offset] = d[i];
      }
      return;
    }

    case 'i': {
      int32_t *int32 = data;
      for (size_t i = index; i < count; ++i)
      {
        tensor->data.i[i * offset] = int32[i];
      }
      return;
    }

    case 'u': {
      float *u = data;
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

uo_nn_value *uo_nn_value_create(uo_tensor *tensor, const char *op, uo_nn_value *children)
{
  size_t base_size = sizeof(uo_nn_value);
  size_t size = base_size;

  switch (tensor->type)
  {
    case 's':
      size += sizeof(float) * tensor->element_count;
      break;

    case 'd':
      size += sizeof(double) * tensor->element_count;
      break;

    case 'i':
      size += sizeof(int32_t) * tensor->element_count;
      break;

    case 'u':
      size += sizeof(uint32_t) * tensor->element_count;
      break;

    default:
      return NULL;
  }

  void *mem = calloc(1, size);
  uo_nn_value *value = mem;
  value->op = op;
  value->tensor = tensor;
  value->grad.ptr = (void *)(((char *)mem) + base_size);
  return value;
}

uo_nn_value *uo_nn_value_op_matmul(uo_nn_value *a, uo_nn_value *b, uo_nn_value *c)
{
  if (c == NULL)
  {
    uo_tensor *C = uo_tensor_create('s', 2, (uo_tensor_dim_def[]) {
      a->tensor->dims[0],
      b->tensor->dims[1]
    });

    c = uo_nn_value_create(C, NULL, NULL);
  }

  // TODO
}

bool uo_test_nn_value()
{
  uo_tensor *A = uo_tensor_create('s', 2, (uo_tensor_dim_def[]) {
    { 2 },
    { 3 }
  });

  uo_tensor_set(A, 0, 0, 6, (float[]) {
    3.0, 2.0, 1.0,
    2.0, 2.0, 1.0
  });

  uo_nn_value *a = uo_nn_value_create(A, NULL, NULL);

  uo_tensor *B = uo_tensor_create('s', 2, (uo_tensor_dim_def[]) {
    { 3 },
    { 1 }
  });

  uo_tensor_set(B, 0, 0, 6, (float[]) {
    -1.0,
     2.0,
     3.0
  });

  uo_nn_value *b = uo_nn_value_create(B, NULL, NULL);

  uo_nn_value *c = NULL;


  c = uo_nn_value_op_matmul(a, b, c, NULL);

}


