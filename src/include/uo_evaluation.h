#ifndef UO_EVALUATION_H
#define UO_EVALUATION_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_position.h"
#include "uo_def.h"

  int16_t uo_position_evaluate(uo_position *const position);

  static int16_t uo_score_adjust_for_mate(int16_t score)
  {
    if (score > UO_SCORE_MATE_IN_THRESHOLD)
    {
      return score - 1;
    }
    else if (score < -UO_SCORE_MATE_IN_THRESHOLD)
    {
      return score + 1;
    }

    return score;
  }

#ifdef __cplusplus
}
#endif

#endif
