#include "uo_square.h"

#include <stddef.h>

const int uo_square_diagonal[64] = {
     7,  8,  9, 10, 11, 12, 13, 14,
     6,  7,  8,  9, 10, 11, 12, 13,
     5,  6,  7,  8,  9, 10, 11, 12,
     4,  5,  6,  7,  8,  9, 10, 11,
     3,  4,  5,  6,  7,  8,  9, 10,
     2,  3,  4,  5,  6,  7,  8,  9,
     1,  2,  3,  4,  5,  6,  7,  8,
     0,  1,  2,  3,  4,  5,  6,  7 };

const int uo_square_antidiagonal[64] = {
    14, 13, 12, 11, 10,  9,  8,  7,
    13, 12, 11, 10,  9,  8,  7,  6,
    12, 11, 10,  9,  8,  7,  6,  5,
    11, 10,  9,  8,  7,  6,  5,  4,
    10,  9,  8,  7,  6,  5,  4,  3,
     9,  8,  7,  6,  5,  4,  3,  2,
     8,  7,  6,  5,  4,  3,  2,  1,
     7,  6,  5,  4,  3,  2,  1,  0 };

size_t uo_squares_between(uo_square from, uo_square to, uo_square between[6])
{
  size_t count = 0;

  uint8_t file_from = uo_square_file(from);
  uint8_t file_to = uo_square_file(to);

  uo_square increment;

  if (file_from == file_to)
  {
    increment = 8;
    goto return_squares_between;
  }

  uint8_t rank_from = uo_square_rank(from);
  uint8_t rank_to = uo_square_rank(to);

  if (rank_from == rank_to)
  {
    increment = 1;
    goto return_squares_between;
  }

  uint8_t diagonal_from = uo_square_diagonal[from];
  uint8_t diagonal_to = uo_square_diagonal[to];

  if (diagonal_from == diagonal_to)
  {
    increment = 9;
    goto return_squares_between;
  }

  uint8_t antidiagonal_from = uo_square_antidiagonal[from];
  uint8_t antidiagonal_to = uo_square_antidiagonal[to];

  if (antidiagonal_from == antidiagonal_to)
  {
    increment = 7;
    goto return_squares_between;
  }

  return count;

return_squares_between:

  if (from < to)
  {
    for (uo_square i = from + increment; i < to; i += increment)
    {
      *between++ = i;
      ++count;
    }
  }
  else
  {
    for (uo_square i = from - increment; i > to; i -= increment)
    {
      *between++ = i;
      ++count;
    }
  }

  return count;
}
