#ifndef UO_TEST_H
#define UO_TEST_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_search.h"

#include <stdbool.h>

  bool uo_test_move_generation(uo_search_thread *thread, char *test_data_dir);


#ifdef __cplusplus
}
#endif

#endif
