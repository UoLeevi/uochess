#ifndef UO_NN_H
#define UO_NN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_position.h"

  typedef struct uo_nn uo_nn;

  void uo_nn_load_position(uo_nn *nn, const uo_position *position);

#ifdef __cplusplus
}
#endif

#endif
