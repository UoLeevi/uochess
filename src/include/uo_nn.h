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
