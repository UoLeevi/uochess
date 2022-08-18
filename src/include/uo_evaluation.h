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
    return score > uo_score_mate_in_threshold || score < -uo_score_mate_in_threshold;
  }

  static inline int16_t uo_score_adjust_for_mate_to_ttable(int16_t score)
  {
    if (score > uo_score_mate_in_threshold)
    {
      return uo_score_checkmate;
    }
    else if (score < -uo_score_mate_in_threshold)
    {
      return uo_score_checkmate;
    }

    return score;
  }

  static inline int16_t uo_score_adjust_for_mate_from_ttable(const uo_position *position, int16_t score)
  {
    if (score == uo_score_checkmate)
    {
      return uo_score_checkmate - position->ply - 1;
    }
    else if (score == -uo_score_checkmate)
    {
      return -uo_score_checkmate + position->ply + 1;
    }

    return score;
  }

  static inline int16_t uo_score_adjust_for_mate(int16_t score)
  {
    if (score > uo_score_mate_in_threshold)
    {
      return score - 1;
    }
    else if (score < -uo_score_mate_in_threshold)
    {
      return score + 1;
    }

    return score;
  }

#ifdef __cplusplus
}
#endif

#endif
