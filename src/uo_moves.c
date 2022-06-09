#include "uo_moves.h"
#include "uo_util.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

static bool init;

uo_bitboard uo_moves_K[64];
uo_bitboard uo_moves_N[64];

typedef struct uo_magic
{
  uint32_t number;
  uint32_t shift;
  uo_bitboard mask;
  union
  {
    uo_bitboard *_blockers;
    uo_bitboard *moves;
  } board;
} uo_magic;

uo_magic uo_magics_B[64];
uo_magic uo_magics_R[64];

void uo_moves_K_init()
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

void uo_moves_N_init()
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

void uo_magics_B_init()
{
  // 1. Generate blocker masks

  int blocker_board_count = 0;

  for (uo_square i = 0; i < 64; ++i)
  {
    int rank = uo_square_rank(i);
    int file = uo_square_file(i);
    uo_bitboard mask_edge_rank = (uo_bitboard_rank[0] | uo_bitboard_rank[7]) & ~uo_bitboard_rank[rank];
    uo_bitboard mask_edge_file = (uo_bitboard_file[0] | uo_bitboard_file[7]) & ~uo_bitboard_file[file];
    uo_bitboard mask_edge = mask_edge_rank | mask_edge_file;
    int diagonal = uo_square_diagonal[i];
    int antidiagonal = uo_square_antidiagonal[i];
    uo_bitboard mask = uo_bitboard_diagonal[diagonal] | uo_bitboard_antidiagonal[antidiagonal];
    mask &= ~mask_edge;
    mask ^= uo_square_bitboard(i);

    uo_magics_B[i].mask = mask;
    uo_magics_B[i].shift = uo_popcount_64(mask);
    blocker_board_count += 1 << uo_magics_B[i].shift;

    // printf("%c%d - %d\n", 'a' + uo_square_file(i), 1 + uo_square_rank(i), i);
    // uo_bitboard_print(mask);
    // printf("\n");
  }

  // 2. Allocate memory for blocker and move boards
  uo_bitboard *blockers = malloc(blocker_board_count * sizeof blockers);
}

void uo_magics_R_init()
{
  // 1. Generate blocker masks

  int blocker_board_count = 0;

  for (uo_square i = 0; i < 64; ++i)
  {
    int rank = uo_square_rank(i);
    int file = uo_square_file(i);
    uo_bitboard mask_edge_rank = (uo_bitboard_rank[0] | uo_bitboard_rank[7]) & ~uo_bitboard_rank[rank];
    uo_bitboard mask_edge_file = (uo_bitboard_file[0] | uo_bitboard_file[7]) & ~uo_bitboard_file[file];
    uo_bitboard mask_edge = mask_edge_rank | mask_edge_file;
    uo_bitboard mask = uo_bitboard_file[file] | uo_bitboard_rank[rank];
    mask &= ~mask_edge;
    mask ^= uo_square_bitboard(i);

    uo_magics_R[i].mask = mask;
    uo_magics_R[i].shift = uo_popcount_64(mask);
    blocker_board_count += 1 << uo_magics_R[i].shift;

    // printf("%c%d - %d\n", 'a' + uo_square_file(i), 1 + uo_square_rank(i), i);
    // uo_bitboard_print(mask);
    // printf("\n");
  }

  // 2. Allocate memory for blocker and move boards
  uo_bitboard *blockers = malloc(blocker_board_count * sizeof blockers);


  // 3. Generate blocker boards and move boards
  for (uo_square i = 0; i < 64; ++i)
  {
    uo_magic magic = uo_magics_R[i];
    uo_bitboard mask = magic.mask;
    int bits

  }
}

void uo_moves_init()
{
  if (init)
    return;
  init = true;
  uo_moves_K_init();
  uo_moves_N_init();
  uo_magics_B_init();
  uo_magics_R_init();
}

bool uo_test_moves()
{
  bool passed = true;

  return passed;
}
