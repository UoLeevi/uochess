#ifndef UO_EVALUATION_H
#define UO_EVALUATION_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_position.h"
#include "uo_piece.h"
#include "uo_def.h"

  int16_t uo_position_evaluate(const uo_position *position);

  static inline bool uo_score_is_checkmate(int16_t score)
  {
    return score > UO_SCORE_MATE_IN_THRESHOLD || score < -UO_SCORE_MATE_IN_THRESHOLD;
  }

  static inline int16_t uo_score_adjust_for_mate_to_ttable(int16_t score)
  {
    if (score > UO_SCORE_MATE_IN_THRESHOLD)
    {
      return UO_SCORE_CHECKMATE;
    }
    else if (score < -UO_SCORE_MATE_IN_THRESHOLD)
    {
      return UO_SCORE_CHECKMATE;
    }

    return score;
  }

  static inline int16_t uo_score_adjust_for_mate_from_ttable(const uo_position *position, int16_t score)
  {
    if (score == UO_SCORE_CHECKMATE)
    {
      return UO_SCORE_CHECKMATE - position->ply - 1;
    }
    else if (score == -UO_SCORE_CHECKMATE)
    {
      return -UO_SCORE_CHECKMATE + position->ply + 1;
    }

    return score;
  }

  static inline int16_t uo_score_adjust_for_mate(int16_t score)
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
