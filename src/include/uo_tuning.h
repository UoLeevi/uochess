#ifndef UO_TUNING_H
#define UO_TUNING_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_piece.h"
#include "uo_util.h"
#include "uo_def.h"
#include "uo_evaluation.h"
#include "uo_position.h"

#include <math.h>

  void uo_tuning_generate_dataset(char *dataset_filepath, char *engine_filepath, char *engine_option_commands, size_t position_count);

  bool uo_tuning_calculate_eval_mean_square_error(char *dataset_filepath, double *mse);
  
#ifdef __cplusplus
}
#endif

#endif
