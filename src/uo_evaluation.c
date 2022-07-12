#include "uo_evaluation.h"
#include "uo_util.h"

#include <inttypes.h>

int16_t uo_position_evaluate(uo_position *const position)
{
  int16_t score = 0;

  // material
  score += (uo_popcnt(position->own & position->P) - uo_popcnt(position->enemy & position->P)) * 100;
  score += (uo_popcnt(position->own & position->N) - uo_popcnt(position->enemy & position->N)) * 300;
  score += (uo_popcnt(position->own & position->B) - uo_popcnt(position->enemy & position->B)) * 301;
  score += (uo_popcnt(position->own & position->R) - uo_popcnt(position->enemy & position->R)) * 500;
  score += (uo_popcnt(position->own & position->Q) - uo_popcnt(position->enemy & position->Q)) * 900;

  return score;
}
