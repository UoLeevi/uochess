#include "uo_bitboard.h"
#include "uo_util.h"
#include "uo_piece.h"
#include "uo_util.h"
#include "uo_def.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

static const uo_bitboard uo_bitboard__a_file = (uo_bitboard)0x0101010101010101;
static const uo_bitboard uo_bitboard__h_file = (uo_bitboard)0x8080808080808080;
static const uo_bitboard uo_bitboard__1st_rank = (uo_bitboard)0x00000000000000FF;
static const uo_bitboard uo_bitboard__8th_rank = (uo_bitboard)0xFF00000000000000;
static const uo_bitboard uo_bitboard__a1_h8_diagonal = (uo_bitboard)0x8040201008040201;
static const uo_bitboard uo_bitboard__h1_a8_antidiagonal = (uo_bitboard)0x0102040810204080;
static const uo_bitboard uo_bitboard__light_squares = (uo_bitboard)0x55AA55AA55AA55AA;
static const uo_bitboard uo_bitboard__dark_squares = (uo_bitboard)0xAA55AA55AA55AA55;

uo_bitboard uo_bitboard_file[8];          //  |
uo_bitboard uo_bitboard_rank[8];          //  -
uo_bitboard uo_bitboard_diagonal[15];     //  /
uo_bitboard uo_bitboard_antidiagonal[15]; //  \

uo_bitboard uo_moves_K[64];
uo_bitboard uo_moves_N[64];
uo_bitboard uo_attacks_P[2][64];
uo_slider_moves uo_moves_B[64];
uo_slider_moves uo_moves_R[64];

static bool init;

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

void uo_moves_K_init(void)
{
  for (uo_square i = 0; i < 64; ++i)
  {
    uo_moves_K[i] = uo_square_bitboard(i);
  }

  // one file right
  for (int file = 0; file < 7; ++file)
  {
    for (int rank = 0; rank < 8; ++rank)
    {
      uo_square i = (rank << 3) + file;
      uo_bitboard moves = uo_moves_K[i];
      moves |= moves << 1; // one file right
      uo_moves_K[i] = moves;
    }
  }

  // one file left
  for (int file = 1; file < 8; ++file)
  {
    for (int rank = 0; rank < 8; ++rank)
    {
      uo_square i = (rank << 3) + file;
      uo_bitboard moves = uo_moves_K[i];
      moves |= moves >> 1; // one file left
      uo_moves_K[i] = moves;
    }
  }

  // one rank up
  for (int rank = 0; rank < 7; ++rank)
  {
    for (int file = 0; file < 8; ++file)
    {
      uo_square i = (rank << 3) + file;
      uo_bitboard moves = uo_moves_K[i];
      moves |= moves << 8; // one rank up
      uo_moves_K[i] = moves;
    }
  }

  // one rank down
  for (int rank = 1; rank < 8; ++rank)
  {
    for (int file = 0; file < 8; ++file)
    {
      uo_square i = (rank << 3) + file;
      uo_bitboard moves = uo_moves_K[i];
      moves |= moves >> 8; // one rank down
      uo_moves_K[i] = moves;
    }
  }

  for (uo_square i = 0; i < 64; ++i)
  {
    uo_bitboard moves = uo_moves_K[i];
    moves ^= uo_square_bitboard(i);
    uo_moves_K[i] = moves;

    // printf("%d\n", i);
    // uo_bitboard_print(moves);
    // printf("\n");
  }
}

void uo_moves_N_init(void)
{
  // two ranks up, one file right
  for (int file = 0; file < 7; ++file)
  {
    for (int rank = 0; rank < 6; ++rank)
    {
      uo_square i = (rank << 3) + file;
      uo_moves_N[i] |= uo_square_bitboard(i) << (8 + 8 + 1); // two ranks up, one file right
    }
  }

  // two ranks up, one file left
  for (int file = 1; file < 8; ++file)
  {
    for (int rank = 0; rank < 6; ++rank)
    {
      uo_square i = (rank << 3) + file;
      uo_moves_N[i] |= uo_square_bitboard(i) << (8 + 8 - 1); // two ranks up, one file left
    }
  }

  // one rank up, two files right
  for (int file = 0; file < 6; ++file)
  {
    for (int rank = 0; rank < 7; ++rank)
    {
      uo_square i = (rank << 3) + file;
      uo_moves_N[i] |= uo_square_bitboard(i) << (8 + 1 + 1); // one rank up, two files right
    }
  }

  // one rank up, two files left
  for (int file = 2; file < 8; ++file)
  {
    for (int rank = 0; rank < 7; ++rank)
    {
      uo_square i = (rank << 3) + file;
      uo_moves_N[i] |= uo_square_bitboard(i) << (8 - 1 - 1); // one rank up, two files left
    }
  }

  // two ranks down, one file right
  for (int file = 0; file < 7; ++file)
  {
    for (int rank = 2; rank < 8; ++rank)
    {
      uo_square i = (rank << 3) + file;
      uo_moves_N[i] |= uo_square_bitboard(i) >> (8 + 8 - 1); // two ranks down, one file right
    }
  }

  // two ranks down, one file left
  for (int file = 1; file < 8; ++file)
  {
    for (int rank = 2; rank < 8; ++rank)
    {
      uo_square i = (rank << 3) + file;
      uo_moves_N[i] |= uo_square_bitboard(i) >> (8 + 8 + 1); // two ranks down, one file left
    }
  }

  // one rank down, two files right
  for (int file = 0; file < 6; ++file)
  {
    for (int rank = 1; rank < 8; ++rank)
    {
      uo_square i = (rank << 3) + file;
      uo_moves_N[i] |= uo_square_bitboard(i) >> (8 - 1 - 1); // one rank down, two files right
    }
  }

  // one rank down, two files left
  for (int file = 2; file < 8; ++file)
  {
    for (int rank = 1; rank < 8; ++rank)
    {
      uo_square i = (rank << 3) + file;
      uo_moves_N[i] |= uo_square_bitboard(i) >> (8 + 1 + 1); // one rank down, two files left
    }
  }

  // for (uo_square i = 0; i < 64; ++i)
  // {
  //   uo_bitboard moves = uo_moves_N[i];
  //   printf("%d\n", i);
  //   uo_bitboard_print(moves);
  //   printf("\n");
  // }
}

void uo_attacks_P_init(void)
{
  for (uint8_t rank = 0; rank < 8; ++rank)
  {
    for (uint8_t file = 0; file < 7; ++file)
    {
      uo_square square = (rank << 3) + file;
      uo_bitboard mask = uo_square_bitboard(square);

      uo_attacks_P[uo_white][square] |= mask << 9;
      uo_attacks_P[uo_black][square] |= mask >> 7;
    }

    for (uint8_t file = 1; file < 8; ++file)
    {
      uo_square square = (rank << 3) + file;
      uo_bitboard mask = uo_square_bitboard(square);

      uo_attacks_P[uo_white][square] |= mask << 7;
      uo_attacks_P[uo_black][square] |= mask >> 9;
    }
  }
}

static inline uo_bitboard uo_bitboard_permutation(uo_bitboard mask, uint8_t bits, uint16_t permutation)
{
  uo_bitboard blockers = 0;
  uo_square square = 0;
  uint8_t bit = 0;

  for (uint8_t bit = 0; bit < bits; ++bit)
  {
    while (square < 64 && !(mask & uo_square_bitboard(square)))
    {
      ++square;
    }

    if (permutation & ((uint16_t)1 << bit))
    {
      blockers |= uo_square_bitboard(square);
    }

    ++square;
  }

  return blockers;
}

static inline uo_bitboard uo_bitboard_gen_masks_B(uo_square square, uo_slider_moves *slider_moves)
{
  int rank = uo_square_rank(square);
  int file = uo_square_file(square);
  uo_bitboard mask_edge_rank = uo_andn(uo_bitboard_rank[rank], uo_bitboard_rank[0] | uo_bitboard_rank[7]);
  uo_bitboard mask_edge_file = uo_andn(uo_bitboard_file[file], uo_bitboard_file[0] | uo_bitboard_file[7]);
  uo_bitboard mask_edge = mask_edge_rank | mask_edge_file;
  int diagonal = uo_square_diagonal[square];
  int antidiagonal = uo_square_antidiagonal[square];
  uo_bitboard mask = uo_bitboard_diagonal[diagonal] | uo_bitboard_antidiagonal[antidiagonal];
  mask = uo_andn(uo_square_bitboard(square), mask); 
  slider_moves->mask_rays = mask;
  mask = uo_andn(mask_edge, mask);
  slider_moves->mask_blockers = mask;
  return mask;
}

static inline uo_bitboard uo_bitboard_gen_masks_R(uo_square square, uo_slider_moves *slider_moves)
{
  int rank = uo_square_rank(square);
  int file = uo_square_file(square);
  uo_bitboard mask_edge_rank = uo_andn(uo_bitboard_rank[rank], uo_bitboard_rank[0] | uo_bitboard_rank[7]);
  uo_bitboard mask_edge_file = uo_andn(uo_bitboard_file[file], uo_bitboard_file[0] | uo_bitboard_file[7]);
  uo_bitboard mask_edge = mask_edge_rank | mask_edge_file;
  uo_bitboard mask = uo_bitboard_file[file] | uo_bitboard_rank[rank];
  mask = uo_andn(uo_square_bitboard(square), mask); 
  slider_moves->mask_rays = mask;
  mask = uo_andn(mask_edge, mask);
  slider_moves->mask_blockers = mask;
  return mask;
}

static inline uo_bitboard uo_bitboard_gen_moves_B(uo_bitboard blockers, uo_square square)
{
  int rank = uo_square_rank(square);
  int file = uo_square_file(square);
  uo_bitboard mask = uo_square_bitboard(square);
  uo_bitboard moves = 0;

  // up right
  for (size_t i = 1; (i < 8 - file) && (i < 8 - rank); i++)
  {
    moves |= mask << ((i << 3) + i); // one ranks up, one file right
    if (blockers & uo_square_bitboard(((rank + i) << 3) + file + i))
      break;
  }

  // up left
  for (size_t i = 1; (i <= file) && (i < 8 - rank); i++)
  {
    moves |= mask << ((i << 3) - i); // one ranks up, one file left
    if (blockers & uo_square_bitboard(((rank + i) << 3) + file - i))
      break;
  }

  // down right
  for (size_t i = 1; (i < 8 - file) && (i <= rank); i++)
  {
    moves |= mask >> ((i << 3) - i); // one ranks down, one file right
    if (blockers & uo_square_bitboard(((rank - i) << 3) + file + i))
      break;
  }

  // down left
  for (size_t i = 1; (i <= file) && (i <= rank); i++)
  {
    moves |= mask >> ((i << 3) + i); // one ranks down, one file left
    if (blockers & uo_square_bitboard(((rank - i) << 3) + file - i))
      break;
  }

  return moves;
}

static inline uo_bitboard uo_bitboard_gen_moves_R(uo_bitboard blockers, uo_square square)
{
  int rank = uo_square_rank(square);
  int file = uo_square_file(square);
  uo_bitboard mask = uo_square_bitboard(square);
  uo_bitboard moves = 0;

  // right
  for (size_t i = 1; i < 8 - file; i++)
  {
    moves |= mask << i; // one file right
    if (blockers & uo_square_bitboard((rank << 3) + file + i))
      break;
  }

  // left
  for (size_t i = 1; i <= file; i++)
  {
    moves |= mask >> i; // one file left
    if (blockers & uo_square_bitboard((rank << 3) + file - i))
      break;
  }

  // up
  for (size_t i = 1; i < 8 - rank; i++)
  {
    moves |= mask << (i << 3); // one rank up
    if (blockers & uo_square_bitboard(((rank + i) << 3) + file))
      break;
  }

  // down
  for (size_t i = 1; i <= rank; i++)
  {
    moves |= mask >> (i << 3); // one rank down
    if (blockers & uo_square_bitboard(((rank - i) << 3) + file))
      break;
  }

  return moves;
}

void uo_moves_B_init(void)
{
  // 1. Generate blocker masks

  int blocker_board_count_total = 0;
  int blocker_board_count_max = 0;

  for (uo_square square = 0; square < 64; ++square)
  {
    uo_bitboard mask = uo_bitboard_gen_masks_B(square, uo_moves_B + square);
    int bits = uo_popcnt(mask);
    int blocker_board_count = 1 << bits;
    blocker_board_count_total += blocker_board_count;
  }

  // 2. Allocate memory for move boards
  uint16_t *boards = malloc(blocker_board_count_total * sizeof * boards);

  // 3. Generate blocker boards and move boards
  for (uo_square square = 0; square < 64; ++square)
  {
    uo_moves_B[square].moves = boards;
    uo_bitboard mask_blockers = uo_moves_B[square].mask_blockers;
    uo_bitboard mask_rays = uo_moves_B[square].mask_rays;
    int bits = uo_popcnt(mask_blockers);
    int permutation_count = 1 << bits;
    boards += permutation_count;

    for (int permutation = 0; permutation < permutation_count; ++permutation)
    {
      uo_bitboard blockers = uo_bitboard_permutation(mask_blockers, bits, permutation);
      uo_bitboard moves = uo_bitboard_gen_moves_B(blockers, square);
      uo_moves_B[square].moves[uo_pext(blockers, mask_blockers)] = uo_pext(moves, mask_rays);
    }
  }
}

void uo_moves_R_init(void)
{
  // 1. Generate blocker masks

  int blocker_board_count_total = 0;
  int blocker_board_count_max = 0;

  for (uo_square square = 0; square < 64; ++square)
  {
    uo_bitboard mask = uo_bitboard_gen_masks_R(square, uo_moves_R + square);
    int bits = uo_popcnt(mask);
    int blocker_board_count = 1 << bits;
    blocker_board_count_total += blocker_board_count;
  }

  // 2. Allocate memory for move boards
  uint16_t *boards = malloc(blocker_board_count_total * sizeof * boards);

  // 3. Generate blocker boards and move boards
  for (uo_square square = 0; square < 64; ++square)
  {
    uo_moves_R[square].moves = boards;
    uo_bitboard mask_blockers = uo_moves_R[square].mask_blockers;
    uo_bitboard mask_rays = uo_moves_R[square].mask_rays;
    int bits = uo_popcnt(mask_blockers);
    int permutation_count = 1 << bits;
    boards += permutation_count;

    for (int permutation = 0; permutation < permutation_count; ++permutation)
    {
      uo_bitboard blockers = uo_bitboard_permutation(mask_blockers, bits, permutation);
      uo_bitboard moves = uo_bitboard_gen_moves_R(blockers, square);
      uo_moves_R[square].moves[uo_pext(blockers, mask_blockers)] = uo_pext(moves, mask_rays);
    }
  }
}

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

  uo_moves_K_init();
  uo_moves_N_init();
  uo_attacks_P_init();
  uo_moves_B_init();
  uo_moves_R_init();
}

uo_bitboard uo_bitboard_pins(uo_square square, uo_bitboard blockers, uo_bitboard diagonal_attackers, uo_bitboard line_attackers)
{
  const uint8_t file = uo_square_file(square);
  const uint8_t rank = uo_square_rank(square);
  line_attackers &= uo_bitboard_file[file] | uo_bitboard_rank[rank];
  const uint8_t diagonal = uo_square_diagonal[square];
  const uint8_t antidiagonal = uo_square_antidiagonal[square];
  diagonal_attackers &= uo_bitboard_diagonal[diagonal] | uo_bitboard_antidiagonal[antidiagonal];

  uo_bitboard pins = 0;
  uo_square pin = 0;

  // diagonal pin below

  for (int i = square - 9; i >= 9 && uo_square_file(i) < file; i -= 9)
  {
    if (uo_square_bitboard(i) & blockers)
    {
      pin = i;

      for (int j = i - 9; j >= 0 && uo_square_file(j) < file; j -= 9)
      {
        if (uo_square_bitboard(j) & diagonal_attackers)
        {
          goto break__diagonal_pin_below;
        }

        if (uo_square_bitboard(j) & blockers)
        {
          pin = 0;
          goto break__diagonal_pin_below;
        }
      }

      pin = 0;
    }
  }
break__diagonal_pin_below:

  if (pin)
  {
    pins |= uo_square_bitboard(pin);
    pin = 0;
  }

  // diagonal pin above

  for (int i = square + 9; i < 55 && uo_square_file(i) > file; i += 9)
  {
    if (uo_square_bitboard(i) & blockers)
    {
      pin = i;

      for (int j = i + 9; j < 64 && uo_square_file(j) > file; j += 9)
      {
        if (uo_square_bitboard(j) & diagonal_attackers)
        {
          goto break__diagonal_pin_above;
        }

        if (uo_square_bitboard(j) & blockers)
        {
          pin = 0;
          goto break__diagonal_pin_above;
        }
      }

      pin = 0;
    }
  }
break__diagonal_pin_above:

  if (pin)
  {
    pins |= uo_square_bitboard(pin);
    pin = 0;
  }

  // antidiagonal pin below

  for (int i = square - 7; i >= 9 && uo_square_file(i) > file; i -= 7)
  {
    if (uo_square_bitboard(i) & blockers)
    {
      pin = i;

      for (int j = i - 7; j >= 0 && uo_square_file(j) > file; j -= 7)
      {
        if (uo_square_bitboard(j) & diagonal_attackers)
        {
          goto break__antidiagonal_pin_below;
        }

        if (uo_square_bitboard(j) & blockers)
        {
          pin = 0;
          goto break__antidiagonal_pin_below;
        }
      }

      pin = 0;
    }
  }
break__antidiagonal_pin_below:

  if (pin)
  {
    pins |= uo_square_bitboard(pin);
    pin = 0;
  }

  // antidiagonal pin above

  for (int i = square + 7; i < 55 && uo_square_file(i) < file; i += 7)
  {
    if (uo_square_bitboard(i) & blockers)
    {
      pin = i;

      for (int j = i + 7; j < 64 && uo_square_file(j) < file; j += 7)
      {
        if (uo_square_bitboard(j) & diagonal_attackers)
        {
          goto break__antidiagonal_pin_above;
        }

        if (uo_square_bitboard(j) & blockers)
        {
          pin = 0;
          goto break__antidiagonal_pin_above;
        }
      }

      pin = 0;
    }
  }
break__antidiagonal_pin_above:

  if (pin)
  {
    pins |= uo_square_bitboard(pin);
    pin = 0;
  }

  // rank pins left

  for (int i = square - 1; i > square - file; --i)
  {
    if (uo_square_bitboard(i) & blockers)
    {
      pin = i;

      for (int j = i - 1; j >= square - file; --j)
      {
        if (uo_square_bitboard(j) & line_attackers)
        {
          goto break__rank_pin_left;
        }

        if (uo_square_bitboard(j) & blockers)
        {
          pin = 0;
          goto break__rank_pin_left;
        }
      }

      pin = 0;
    }
  }
break__rank_pin_left:

  if (pin)
  {
    pins |= uo_square_bitboard(pin);
    pin = 0;
  }

  // rank pins right

  for (int i = square + 1; i < square - file + 7; ++i)
  {
    if (uo_square_bitboard(i) & blockers)
    {
      pin = i;

      for (int j = i + 1; j < square - file + 8; ++j)
      {
        if (uo_square_bitboard(j) & line_attackers)
        {
          goto break__rank_pin_right;
        }

        if (uo_square_bitboard(j) & blockers)
        {
          pin = 0;
          goto break__rank_pin_right;
        }
      }

      pin = 0;
    }
  }
break__rank_pin_right:

  if (pin)
  {
    pins |= uo_square_bitboard(pin);
    pin = 0;
  }


  // file pins below

  for (int i = square - 8; i > file; i -= 8)
  {
    if (uo_square_bitboard(i) & blockers)
    {
      pin = i;

      for (int j = i - 8; j >= file; j -= 8)
      {
        if (uo_square_bitboard(j) & line_attackers)
        {
          goto break__file_pin_below;
        }

        if (uo_square_bitboard(j) & blockers)
        {
          pin = 0;
          goto break__file_pin_below;
        }
      }

      pin = 0;
    }
  }
break__file_pin_below:

  if (pin)
  {
    pins |= uo_square_bitboard(pin);
    pin = 0;
  }

  // file pins above

  for (int i = square + 8; i < 56; i += 8)
  {
    if (uo_square_bitboard(i) & blockers)
    {
      pin = i;

      for (int j = i + 8; j < 64; j += 8)
      {
        if (uo_square_bitboard(j) & line_attackers)
        {
          goto break__file_pin_above;
        }

        if (uo_square_bitboard(j) & blockers)
        {
          pin = 0;
          goto break__file_pin_above;
        }
      }

      pin = 0;
    }
  }
break__file_pin_above:

  if (pin)
  {
    pins |= uo_square_bitboard(pin);
    pin = 0;
  }

  return pins;
}

uo_bitboard uo_bitboard_pins_B(uo_square square, uo_bitboard blockers, uo_bitboard diagonal_attackers)
{
  uint8_t diagonal = uo_square_diagonal[square];
  uint8_t antidiagonal = uo_square_antidiagonal[square];

  uo_bitboard mask = uo_square_bitboard(square);
  diagonal_attackers = uo_andn(mask, diagonal_attackers);
  blockers = uo_andn(mask, blockers);

  uo_bitboard pins = 0;

  uo_bitboard antidiagonal_attackers = diagonal_attackers & uo_bitboard_antidiagonal[antidiagonal];

  if (antidiagonal_attackers)
  {
    uo_bitboard antidiagonal_blockers = blockers & uo_bitboard_antidiagonal[antidiagonal];

    uo_bitboard antidiagonal_attackers_below = uo_bzhi(antidiagonal_attackers, square + 1);
    uo_square square_antidiagonal_attacker_below = uo_msb(antidiagonal_attackers_below);

    if (square_antidiagonal_attacker_below != -1)
    {
      uo_bitboard antidiagonal_blockers_below = uo_bzhi(antidiagonal_blockers, square + 1);
      antidiagonal_blockers_below = (antidiagonal_blockers_below >> square_antidiagonal_attacker_below) << square_antidiagonal_attacker_below;
      if (uo_popcnt(antidiagonal_blockers_below) == 2) pins |= uo_andn(uo_square_bitboard(square_antidiagonal_attacker_below), antidiagonal_blockers_below);
    }

    uo_bitboard antidiagonal_attackers_above = uo_bzlo(antidiagonal_attackers, square);
    uo_square square_antidiagonal_attacker_above = uo_lsb(antidiagonal_attackers_above);

    if (square_antidiagonal_attacker_above != -1)
    {
      uo_bitboard antidiagonal_blockers_above = uo_bzlo(antidiagonal_blockers, square);
      antidiagonal_blockers_above = uo_bzhi(antidiagonal_blockers_above, square_antidiagonal_attacker_above + 1);
      if (uo_popcnt(antidiagonal_blockers_above) == 2) pins |= uo_andn(uo_square_bitboard(square_antidiagonal_attacker_above), antidiagonal_blockers_above);
    }
  }

  diagonal_attackers &= uo_bitboard_diagonal[diagonal];

  if (diagonal_attackers)
  {
    uo_bitboard diagonal_blockers = blockers & uo_bitboard_diagonal[diagonal];
    uo_bitboard diagonal_attackers_below = uo_bzhi(diagonal_attackers, square + 1);
    uo_square square_diagonal_attacker_below = uo_msb(diagonal_attackers_below);

    if (square_diagonal_attacker_below != -1)
    {
      uo_bitboard diagonal_blockers_below = uo_bzhi(diagonal_blockers, square + 1);
      diagonal_blockers_below = (diagonal_blockers_below >> square_diagonal_attacker_below) << square_diagonal_attacker_below;
      if (uo_popcnt(diagonal_blockers_below) == 2) pins |= uo_andn(uo_square_bitboard(square_diagonal_attacker_below), diagonal_blockers_below);
    }

    uo_bitboard diagonal_attackers_above = uo_bzlo(diagonal_attackers, square);
    uo_square square_diagonal_attacker_above = uo_lsb(diagonal_attackers_above);

    if (square_diagonal_attacker_above != -1)
    {
      uo_bitboard diagonal_blockers_above = uo_bzlo(diagonal_blockers, square);
      diagonal_blockers_above = uo_bzhi(diagonal_blockers_above, square_diagonal_attacker_above + 1);
      if (uo_popcnt(diagonal_blockers_above) == 2) pins |= uo_andn(uo_square_bitboard(square_diagonal_attacker_above), diagonal_blockers_above);
    }
  }

  return pins;
}

uo_bitboard uo_bitboard_pins_R(uo_square square, uo_bitboard blockers, uo_bitboard line_attackers)
{
  uint8_t file = uo_square_file(square);
  uint8_t rank = uo_square_rank(square);

  uo_bitboard mask = uo_square_bitboard(square);
  line_attackers = uo_andn(mask, line_attackers);
  blockers = uo_andn(mask, blockers);

  uo_bitboard pins = 0;

  uo_bitboard rank_attackers = line_attackers & uo_bitboard_rank[rank];

  if (rank_attackers)
  {
    uo_bitboard rank_blockers = blockers & uo_bitboard_rank[rank];
    uo_bitboard rank_attackers_left = uo_bzhi(rank_attackers, square + 1);
    uo_square square_rank_attacker_left = uo_msb(rank_attackers_left);

    if (square_rank_attacker_left != -1)
    {
      uo_bitboard rank_blockers_left = uo_bzhi(rank_blockers, square + 1);
      rank_blockers_left = (rank_blockers_left >> square_rank_attacker_left) << square_rank_attacker_left;
      if (uo_popcnt(rank_blockers_left) == 2) pins |= uo_andn(uo_square_bitboard(square_rank_attacker_left), rank_blockers_left);
    }

    uo_bitboard rank_attackers_right = uo_bzlo(rank_attackers, square);
    uo_square square_rank_attacker_right = uo_lsb(rank_attackers_right);

    if (square_rank_attacker_right != -1)
    {
      uo_bitboard rank_blockers_right = uo_bzlo(rank_blockers, square);
      rank_blockers_right = uo_bzhi(rank_blockers_right, square_rank_attacker_right + 1);
      if (uo_popcnt(rank_blockers_right) == 2) pins |= uo_andn(uo_square_bitboard(square_rank_attacker_right), rank_blockers_right);
    }
  }

  uo_bitboard file_attackers = line_attackers & uo_bitboard_file[file];

  if (file_attackers)
  {
    uo_bitboard file_blockers = blockers & uo_bitboard_file[file];
    uo_bitboard file_attackers_below = uo_bzhi(file_attackers, square + 1);
    uo_square square_file_attacker_below = uo_msb(file_attackers_below);

    if (square_file_attacker_below != -1)
    {
      uo_bitboard file_blockers_below = uo_bzhi(file_blockers, square + 1);
      file_blockers_below = (file_blockers_below >> square_file_attacker_below) << square_file_attacker_below;
      if (uo_popcnt(file_blockers_below) == 2) pins |= uo_andn(uo_square_bitboard(square_file_attacker_below), file_blockers_below);
    }

    uo_bitboard file_attackers_above = uo_bzlo(file_attackers, square);
    uo_square square_file_attacker_above = uo_lsb(file_attackers_above);

    if (square_file_attacker_above != -1)
    {
      uo_bitboard file_blockers_above = uo_bzlo(file_blockers, square);
      file_blockers_above = uo_bzhi(file_blockers_above, square_file_attacker_above + 1);
      if (uo_popcnt(file_blockers_above) == 2) pins |= uo_andn(uo_square_bitboard(square_file_attacker_above), file_blockers_above);
    }
  }

  return pins;
}
