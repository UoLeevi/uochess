#ifndef UO_NN_H
#define UO_NN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_math.h"
#include "uo_position.h"

  typedef struct uo_nn uo_nn;

  typedef uo_avx_float uo_nn_loss_function(uo_avx_float y_true, uo_avx_float y_pred);

  typedef uo_avx_float uo_nn_activation_function(uo_avx_float avx_float);

  typedef struct uo_nn_function_param
  {
    struct
    {
      uo_nn_activation_function *f;
      uo_nn_activation_function *df;
    } activation;
    struct
    {
      uo_nn_loss_function *f;
      uo_nn_loss_function *df;
    } loss;
    const char *name;
  } uo_nn_function_param;

  typedef void uo_nn_node;

  typedef struct uo_nn_node_base
  {
    const char *op_type;
  } uo_nn_node_base;

  typedef struct uo_nn_node_position
  {
    uo_nn_node_base base;
    uo_position *position;
  } uo_nn_node_position;

  typedef struct uo_nn_node_out_1f
  {
    uo_nn_node_base base;
    size_t m;
    size_t n;
    float *A;
    float *dA;
    void (*backward)(struct uo_nn_node_out_1f *node, float *d);
  } uo_nn_node_out_1f;

  typedef struct uo_nn_node_weights
  {
    uo_nn_node_out_1f base;
  } uo_nn_node_weights;

  typedef struct uo_nn_node_base_in_1f
  {
    uo_nn_node_base base;
    size_t n;
    float *A;
  } uo_nn_node_base_in_1f;

  typedef struct uo_nn_node_base_in_2f
  {
    uo_nn_node_base base;
    float *A;
    float *B;
  } uo_nn_node_base_in_2f;

  typedef struct uo_nn_node_base_in_1f_out_1f
  {
    uo_nn_node_base_in_1f in;
    float *Y;
    void (*forward)(struct uo_nn_node_base_in_1f_out_1f *node, float *A);
    void (*backward)(struct uo_nn_node_base_in_1f_out_1f *node, float *d);
  } uo_nn_node_base_in_1f_out_1f;

  typedef struct uo_nn_node_base_in_2f_out_1f
  {
    uo_nn_node_base_in_2f in;
    float *Y;
    void (*forward)(struct uo_nn_node_base_in_2f_out_1f *node, float *A, float *B);
    void (*backward)(struct uo_nn_node_base_in_2f_out_1f *node, float *d);
  } uo_nn_node_base_in_2f_out_1f;

  //const uo_nn_node graph[] = {
  //  uo_nn_node_input, uo_nn_
  //};

  void asdf()
  {
    // Layer 1
    uo_nn_node_position position_node;
    uo_nn_node_weights W1_t = { { "weights", .m = sizeof(uo_nn_position) / sizeof(uint32_t), .n = 127 } };
    uo_nn_node_base_in_1f_out_1f swish1;

    // Layer 2
    uo_nn_node_weights W2_t = { { "weights", .m = W1_t.base.n + 1, .n = 8 } };
    uo_nn_node_base_in_2f_out_1f matmul2;
    uo_nn_node_base_in_1f_out_1f swish2;

    // Layer 3
    uo_nn_node_weights W3_t = { { "weights", .m = W2_t.base.n + 1, .n = 1 } };
    uo_nn_node_base_in_2f_out_1f matmul3;
    uo_nn_node_base_in_1f_out_1f tanh3;

    uo_nn_node *graph[] =
    {
      // Layer 1
      &position_node, &W1_t,
      &swish1, &position_node,

      // Layer 2
      &matmul2, &swish1, &W2_t,
      &swish2, &matmul2,

      // Layer 3
      &matmul3, &swish2, &W3_t,
      &tanh3, &matmul3
    };
  }

  const uo_nn_function_param *uo_nn_get_function_by_name(const char *function_name);

  void uo_nn_load_position(uo_nn *nn, const uo_position *position, size_t index);

  int16_t uo_nn_evaluate(uo_nn *nn, const uo_position *position);

  uo_nn *uo_nn_read_from_file(uo_nn *nn, char *filepath, size_t batch_size);

  bool uo_nn_train_eval(char *dataset_filepath, char *nn_init_filepath, char *nn_output_file, float learning_rate, size_t iterations, size_t batch_size);

  bool uo_test_nn_train_xor(char *test_data_dir);

  void uo_nn_generate_dataset(char *dataset_filepath, char *engine_filepath, char *engine_option_commands, size_t position_count);

#ifdef __cplusplus
}
#endif

#endif
