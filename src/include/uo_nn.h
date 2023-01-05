#ifndef UO_NN_H
#define UO_NN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_nn_value.h"
#include "uo_math.h"
#include "uo_position.h"

  typedef struct uo_nn
  {
    uo_nn_value **graph;
    size_t graph_size;
    uo_tensor **inputs;
    size_t input_count;
    uo_nn_value **outputs;
    size_t output_count;
    uo_nn_adam_params **parameters;
    size_t parameter_count;
    void *state;
  } uo_nn;

  typedef void uo_nn_select_batch(uo_nn *nn, size_t iteration, uo_tensor *y_true);
  typedef void uo_nn_report(uo_nn *nn, size_t iteration, float error, float learning_rate);

  //void uo_nn_load_position(uo_nn *nn, const uo_position *position, size_t index);

  int16_t uo_nn_evaluate(uo_nn *nn, const uo_position *position);

  //uo_nn *uo_nn_read_from_file(uo_nn *nn, char *filepath, size_t batch_size);

  bool uo_nn_train_eval(char *dataset_filepath, char *nn_init_filepath, char *nn_output_file, float learning_rate, size_t iterations, size_t batch_size);

  bool uo_test_nn_train_xor(char *test_data_dir);

  void uo_nn_generate_dataset(char *dataset_filepath, char *engine_filepath, char *engine_option_commands, size_t position_count);

  static inline void uo_nn_update_parameters(uo_nn *nn)
  {
    for (size_t i = 0; i < nn->parameter_count; ++i)
    {
      uo_nn_value_update_adam(nn->parameters[i]);
    }
  }

  static inline void uo_nn_reset(uo_nn *nn)
  {
    uo_nn_graph_reset(nn->graph, nn->graph_size);
  }

  static inline void uo_nn_forward(uo_nn *nn)
  {
    uo_nn_graph_forward(nn->graph, nn->graph_size);
  }

  static inline void uo_nn_backward(uo_nn *nn)
  {
    uo_nn_graph_backward(nn->graph, nn->graph_size);
  }

#ifdef __cplusplus
}
#endif

#endif
