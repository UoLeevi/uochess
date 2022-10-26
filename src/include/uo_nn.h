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

  typedef struct uo_nn_node
  {
    const char *op_type;
    const size_t input_arg_count;
    void (*init)(struct uo_nn_node **graph, uo_nn *nn);
    void (*forward)(struct uo_nn_node **graph);
    void (*backward)(struct uo_nn_node **graph);
    bool is_init;
  } uo_nn_node;

  typedef struct uo_nn_node_position
  {
    uo_nn_node base;
    uo_position *position;
  } uo_nn_node_position;

  typedef struct uo_nn_node_out_1f
  {
    uo_nn_node base;
    size_t m;
    size_t offset_m;
    size_t offset_n;
    float *A;
    float *dA;
  } uo_nn_node_out_1f;

  typedef struct uo_nn_node_weights
  {
    uo_nn_node_out_1f base;

    struct
    {
      float learning_rate;
      float weight_decay;
      float beta1;
      float beta2;
      float epsilon;

      float *m; // first moment vector
      float *v; // second moment vector
      float *m_hat; // bias-corrected first moment matrix, m x n
      float *v_hat; // bias-corrected second moment matrix, m x n
      size_t t; // timestep
    } adam;
  } uo_nn_node_weights;

  typedef struct uo_nn_node_matmul
  {
    uo_nn_node_out_1f base;
    float *B;
  } uo_nn_node_matmul;


  typedef struct uo_nn_node_in_2f
  {
    uo_nn_node base;
    float *A;
    float *B;
  } uo_nn_node_in_2f;

  //const uo_nn_node graph[] = {
  //  uo_nn_node_input, uo_nn_
  //};

  typedef struct uo_nn
  {
    size_t batch_size;
    uo_nn_node **graph;
    uo_nn_node ***nodes;
    size_t op_count;
    size_t n_y;
    float *y;
    uo_nn_loss_function *loss_func;
    uo_nn_loss_function *loss_func_d;

    // User data
    void *state;
  } uo_nn;

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
