#ifndef UO_TEST_H
#define UO_TEST_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_position.h"
#include "uo_math.h"

#include <stdbool.h>

  bool uo_test_move_generation(uo_position *position, char *test_data_dir);

#ifdef __cplusplus
}
#endif

#endif
