#include "uo_move.h"

#include <stdio.h>

uo_move uo_move_parse(char str[5])
{
  if (!str) return 0;

  char *ptr = str;

  int file_from = ptr[0] - 'a';
  if (file_from < 0 || file_from > 7) return 0;
  int rank_from = ptr[1] - '1';
  if (rank_from < 0 || rank_from > 7) return 0;
  uo_square square_from = (rank_from << 3) + file_from;

  int file_to = ptr[2] - 'a';
  if (file_to < 0 || file_to > 7) return 0;
  int rank_to = ptr[3] - '1';
  if (rank_to < 0 || rank_to > 7) return 0;
  uo_square square_to = (rank_to << 3) + file_to;

  char promotion = ptr[4];
  switch (promotion)
  {
    case 'q': return uo_move_encode(square_from, square_to, uo_move_type__promo_Q);
    case 'r': return uo_move_encode(square_from, square_to, uo_move_type__promo_R);
    case 'b': return uo_move_encode(square_from, square_to, uo_move_type__promo_B);
    case 'n': return uo_move_encode(square_from, square_to, uo_move_type__promo_N);
    default: return uo_move_encode(square_from, square_to, uo_move_type__quiet);
  }
}

size_t uo_move_print(uo_move move, char str[6])
{
  uo_square square_from = uo_move_square_from(move);
  uo_square square_to = uo_move_square_to(move);
  uo_move_type move_type_promo = uo_move_get_type(move) & uo_move_type__promo_Q;

  sprintf(str, "%c%d%c%d",
    'a' + uo_square_file(square_from), 1 + uo_square_rank(square_from),
    'a' + uo_square_file(square_to), 1 + uo_square_rank(square_to));

  if (move_type_promo & uo_move_type__promo)
  {
    switch (move_type_promo)
    {
      case uo_move_type__promo_Q: str[4] = 'q';
      case uo_move_type__promo_R: str[4] = 'r';
      case uo_move_type__promo_B: str[4] = 'b';
      case uo_move_type__promo_N: str[4] = 'n';
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
