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

  bool uo_tuning_train_evaluation_parameters(char *dataset_filepath);
  
#ifdef __cplusplus
}
#endif

#endif
