#include "uo_move.h"

#include <stdio.h>

size_t uo_move_print(uo_move move, char str[6])
{
  uo_square square_from = uo_move_square_from(move);
  uo_square square_to = uo_move_square_to(move);
  uo_move_type move_type_promo = uo_move_get_type(move) & uo_move_type__promo_Q;

  sprintf(str, "%c%d%c%d",
    'a' + uo_square_file(square_from), 1 + uo_square_rank(square_from),
    'a' + uo_square_file(square_to), 1 + uo_square_rank(square_to));

  if (move_type_promo)
  {
    switch (move_type_promo)
    {
      case uo_move_type__promo_Q: str[4] = 'q'; break;
      case uo_move_type__promo_R: str[4] = 'r'; break;
      case uo_move_type__promo_B: str[4] = 'b'; break;
      case uo_move_type__promo_N: str[4] = 'n'; break;
      default: break;
    }

    str[5] = '\0';
    return 5;
  }
  else
  {
    return 4;
  }
}
