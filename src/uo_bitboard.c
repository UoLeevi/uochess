#include "uo_bitboard.h"

#include <stdbool.h>
#include <stdio.h>

int uo_bitboard_print(uo_bitboard bitboard)
{
  // a8 b8 c8 d8 e8 f8 g8 h8
  // a7 b7 c7 d7 e7 f7 g7 h7
  // a6 b6 c6 d6 e6 f6 g6 h6
  // a5 b5 c5 d5 e5 f5 g5 h5
  // a4 b4 c4 d4 e4 f4 g4 h4
  // a3 b3 c3 d3 e3 f3 g3 h3
  // a2 b2 c2 d2 e2 f2 g2 h2
  // a1 b1 c1 d1 e1 f1 g1 h1
  char diagram[72];

  for (int i = 0; i < 64; ++i)
  {
    uo_bitboard mask = (uo_bitboard)1 << i;
    bool isset = mask & bitboard;
    int rank = uo_bitboard_rank(i);
    int file = uo_bitboard_file(i);
    int row = 7 - rank;
    int index = (row << 3) + file + row;
    diagram[index] = isset ? 'X' : '.';
  }

  for (int i = 8; i < 72; i += 9)
  {
    diagram[i] = '\n';
  }

  diagram[71] = '\0';

  return printf("%s\n", diagram);
}
