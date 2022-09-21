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

  typedef struct uo_nn_loss_function_param
  {
    uo_nn_loss_function *f;
    uo_nn_loss_function *df;
  } uo_nn_loss_function_param;

  typedef uo_avx_float uo_nn_activation_function(uo_avx_float avx_float);

  typedef struct uo_nn_activation_function_param
  {
    uo_nn_activation_function *f;
    uo_nn_activation_function *df;
  } uo_nn_activation_function_param;

  const uo_nn_activation_function_param *uo_nn_get_activation_function_by_name(const char *activation_function_name);

  const char *uo_nn_get_activation_function_name(uo_nn_activation_function *f);

  void uo_nn_load_position(uo_nn *nn, const uo_position *position, size_t index);

  bool uo_test_nn_train(char *test_data_dir);

#ifdef __cplusplus
}
#endif

#endif
