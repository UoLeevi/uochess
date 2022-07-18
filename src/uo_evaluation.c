#include "uo_evaluation.h"
#include "uo_util.h"

#include <inttypes.h>

int16_t uo_position_evaluate(uo_position *const position)
{
  int16_t score = 0;

  // material
  score += ((int16_t)uo_popcnt(position->own & position->P) - (int16_t)uo_popcnt(position->enemy & position->P)) * 100;
  score += ((int16_t)uo_popcnt(position->own & position->N) - (int16_t)uo_popcnt(position->enemy & position->N)) * 300;
  score += ((int16_t)uo_popcnt(position->own & position->B) - (int16_t)uo_popcnt(position->enemy & position->B)) * 301;
  score += ((int16_t)uo_popcnt(position->own & position->R) - (int16_t)uo_popcnt(position->enemy & position->R)) * 500;
  score += ((int16_t)uo_popcnt(position->own & position->Q) - (int16_t)uo_popcnt(position->enemy & position->Q)) * 900;

  return score;
}
