#include "uo_bitboard.h"

#include <stdbool.h>
#include <stdio.h>

const uo_bitboard uo_bitboard__a_file = (uo_bitboard)0x0101010101010101;
const uo_bitboard uo_bitboard__h_file = (uo_bitboard)0x8080808080808080;
const uo_bitboard uo_bitboard__1st_rank = (uo_bitboard)0x00000000000000FF;
const uo_bitboard uo_bitboard__8th_rank = (uo_bitboard)0xFF00000000000000;
const uo_bitboard uo_bitboard__a1_h8_diagonal = (uo_bitboard)0x8040201008040201;
const uo_bitboard uo_bitboard__h1_a8_antidiagonal = (uo_bitboard)0x0102040810204080;
const uo_bitboard uo_bitboard__light_squares = (uo_bitboard)0x55AA55AA55AA55AA;
const uo_bitboard uo_bitboard__dark_squares = (uo_bitboard)0xAA55AA55AA55AA55;

uo_bitboard uo_bitboard_file[8];          //  |
uo_bitboard uo_bitboard_rank[8];          //  -
uo_bitboard uo_bitboard_diagonal[15];     //  /
uo_bitboard uo_bitboard_antidiagonal[15]; //  \

static bool init;

void uo_bitboard_init()
{
  if (init)
    return;
  init = true;

  // files and ranks

  for (int i = 0; i < 8; ++i)
  {
    uo_bitboard_file[i] = uo_bitboard__a_file << i;
    uo_bitboard_rank[i] = uo_bitboard__1st_rank << (i << 3);
  }

  // diagonals and antidiagonals

  uo_bitboard_diagonal[7] = uo_bitboard__a1_h8_diagonal;
  uo_bitboard_antidiagonal[7] = uo_bitboard__h1_a8_antidiagonal;

  for (int i = 1; i < 8; ++i)
  {
    uo_bitboard_diagonal[7 - i] = uo_bitboard__a1_h8_diagonal << (i << 3);
    uo_bitboard_diagonal[7 + i] = uo_bitboard__a1_h8_diagonal >> (i << 3);
    uo_bitboard_antidiagonal[7 - i] = uo_bitboard__h1_a8_antidiagonal << (i << 3);
    uo_bitboard_antidiagonal[7 + i] = uo_bitboard__h1_a8_antidiagonal >> (i << 3);
  }

  // for (int i = 0; i < 64; ++i)
  // {
  //   printf("%c%d - %d\n", 'a' + uo_square_file(i), 1 + uo_square_rank(i), i);
  //   uo_bitboard_print(uo_bitboard_antidiagonal[uo_square_antidiagonal[i]]);
  //   printf("\n");
  // }
}

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
    uo_bitboard mask = uo_square_bitboard(i);
    bool isset = mask & bitboard;
    int rank = uo_square_rank(i);
    int file = uo_square_file(i);
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
