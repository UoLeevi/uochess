#ifndef UO_NN_VALUE_H
#define UO_NN_VALUE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

  typedef struct uo_nn_value uo_nn_value;

  typedef void (uo_nn_value_function)(uo_nn_value *node);
  typedef void (uo_nn_value_print_function)(uo_nn_value **node, uo_nn_value **graph, FILE *const fp);
  typedef void (uo_nn_value_parse_function)(uo_nn_value **node, uo_nn_value **graph, const char **ptr);

  typedef union uo_tensor_data
  {
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
    size_t element_size;
    size_t element_count;
    uo_tensor_data data;
    size_t dim_sizes[];
  } uo_tensor;

  typedef struct uo_nn_value
  {
    const char *op;
    uo_tensor *tensor;
    uo_tensor_data grad;
    uo_nn_value_function *forward;
    uo_nn_value_function *backward;
    uo_nn_value_function *reset;
    uo_nn_value_print_function *print;
    void *attributes;
    size_t children_count;
    uo_nn_value *children[];
  } uo_nn_value;

  typedef struct uo_nn_adam_params
  {
    uo_nn_value *value;
    size_t t;

    float lr;
    float beta1;
    float beta2;
    float epsilon;
    float weight_decay;

    float *m; // first moment vector
    float *v; // second moment vector
    float *m_hat; // bias-corrected first moment matrix, m x n
    float *v_hat; // bias-corrected second moment matrix, m x n
  } uo_nn_adam_params;

  uo_nn_value **uo_nn_value_create_graph(uo_nn_value *self, size_t *size);

  uo_tensor *uo_tensor_create(char type, size_t dimension_count, size_t *dim_sizes);

  void uo_tensor_set(uo_tensor *tensor, size_t index, size_t offset, size_t count, const void *data);

  void uo_tensor_set1(uo_tensor *tensor, size_t index, size_t offset, size_t count, const void *value);

  void uo_tensor_set_rand_s(uo_tensor *tensor, size_t index, size_t offset, size_t count, float min, float max);

  void uo_tensor_set_rand(uo_tensor *tensor, size_t index, size_t offset, size_t count, const void *min, const void *max);

  uo_nn_value *uo_nn_value_create(uo_tensor *tensor, const char *op, size_t children_count, size_t attributes_size);

  static inline void uo_nn_value_zero_grad(uo_nn_value *nn_value)
  {
    size_t element_size;

    switch (nn_value->tensor->type)
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
        exit(1);
    }

    memset(nn_value->grad.ptr, 0, nn_value->tensor->element_count * element_size);
  }

  static inline void uo_nn_value_reset(uo_nn_value *nn_value)
  {
    if (nn_value->reset) nn_value->reset(nn_value);
  }

  static inline void uo_nn_value_forward(uo_nn_value *nn_value)
  {
    if (nn_value->forward) nn_value->forward(nn_value);
  }

  static inline void uo_nn_value_backward(uo_nn_value *nn_value)
  {
    if (nn_value->backward) nn_value->backward(nn_value);
  }

  static inline void uo_nn_graph_backward(uo_nn_value **graph, size_t size)
  {
    for (size_t i = 0; i < size - 1; ++i)
    {
      uo_nn_value_zero_grad(graph[i]);
    }

    while (size)
    {
      uo_nn_value_backward(graph[--size]);
    }
  }

  static inline void uo_nn_graph_forward(uo_nn_value **graph, size_t size)
  {
    for (size_t i = 0; i < size; ++i)
    {
      uo_nn_value_forward(graph[i]);
    }
  }

  static inline void uo_nn_graph_reset(uo_nn_value **graph, size_t size)
  {
    for (size_t i = 0; i < size; ++i)
    {
      uo_nn_value_reset(graph[i]);
    }
  }

  void uo_print_nn_graph(FILE *const fp, uo_nn_value **graph, size_t size);

  uo_nn_value *uo_nn_value_parse(const char *op, char **ptr);

  uo_nn_value *uo_nn_value_op_matmul(uo_nn_value *a, uo_nn_value *b);
  uo_nn_value *uo_nn_value_op_gemm_a_mask(uo_nn_value *a, uo_nn_value *b, float alpha, float beta, bool ta, bool tb);
  uo_nn_value *uo_nn_value_op_gemm(uo_nn_value *a, uo_nn_value *b, float alpha, float beta, bool ta, bool tb);
  uo_nn_value *uo_nn_value_op_add(uo_nn_value *a, uo_nn_value *b);
  uo_nn_value *uo_nn_value_op_concat(int axis, size_t count, uo_nn_value **values);
  uo_nn_value *uo_nn_value_op_relu(uo_nn_value *x);
  uo_nn_value *uo_nn_value_op_tanh(uo_nn_value *x);

  float uo_nn_loss_mse(uo_nn_value *y_pred, float *y_true);
  void uo_nn_loss_grad_mse(uo_nn_value *y_pred, float *y_true);

  uo_nn_adam_params *uo_nn_value_adam_params_create(uo_nn_value *value);
  void uo_nn_value_update_adam(uo_nn_adam_params *params);


#ifdef __cplusplus
}
#endif

#endif
