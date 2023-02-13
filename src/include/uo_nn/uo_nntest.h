#ifndef UO_NNTEST_H
#define UO_NNTEST_H

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct uo_nn2 uo_nn2;

  uo_nn2 *uo_nn2_create_xor();

  void *uo_nn2_input_data_ptr(uo_nn2 *nn, int input_index);

  void *uo_nn2_output_data_ptr(uo_nn2 *nn, int output_index);

  void uo_nn2_zero_grad(uo_nn2 *nn);

  void uo_nn2_forward(uo_nn2 *nn);

  void uo_nn2_backward(uo_nn2 *nn);

  void uo_nn2_update_parameters(uo_nn2 *nn);

#ifdef __cplusplus
}
#endif

#endif
