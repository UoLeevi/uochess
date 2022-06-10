#include "uo_moves.h"
#include "uo_piece.h"
#include "uo_util.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

int rand_seed;

const uint64_t known_magics_R[64][2] = {
    // square      number               bits
    [0 /* a1 */] = {0x177801395e43fffa, 12},
    [1 /* b1 */] = {0x8400020009007c4, 11}};

static bool init;

uo_bitboard uo_moves_K[64];
uo_bitboard uo_moves_N[64];

typedef struct uo_magic
{
  uo_bitboard *moves;
  uo_bitboard mask;
  uint64_t number;
  uint8_t shift;
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

static inline uo_bitboard uo_bitboard_moves_R(uo_bitboard blockers, uo_square square)
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

static inline void uo_magic_init(uo_magic *magic, uo_bitboard *blockers_moves /* alternating list of blockers and moves*/, uo_square square, const uint64_t known_magic[2])
{
  if (known_magic[0])
  {
    magic->number = known_magic[0];
    magic->shift = 64 - known_magic[1];
  }

  uint8_t shift = magic->shift;
  uint8_t bits = 64 - shift;
  uint16_t count = 1 << bits;
  uint64_t number = magic->number ? magic->number : rand();

  while (true)
  {
    memset(magic->moves, 0, count * sizeof *magic->moves);

    uo_bitboard *ptr = blockers_moves;

    for (size_t i = 0; i < count; i++)
    {
      uo_bitboard blockers = *ptr++;
      uo_bitboard moves = *ptr++;

      int index = (blockers * number) >> shift;

      if (!magic->moves[index])
      {
        magic->moves[index] = moves;
      }
      else if (magic->moves[index] != moves)
      {
        number <<= 3;
        number ^= (uint64_t)rand();
        goto try_next_number;
      }
    }

    if (!known_magic[0])
    {
      // suitable magic number was found
      printf("suitable magic number was found\n");
      printf("rand_seed: %d\n", rand_seed);
      printf("square: %d\n", square);
      printf("bits: %d\n", bits);
      printf("number: 0x%lx\n", number);
      printf("\n");
      magic->number = number;
    }

    return;
  try_next_number:;
  }
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
}

void uo_magics_R_init()
{
  // 1. Generate blocker masks

  int blocker_board_count_total = 0;
  int blocker_board_count_max = 0;

  for (uo_square square = 0; square < 64; ++square)
  {
    int rank = uo_square_rank(square);
    int file = uo_square_file(square);
    uo_bitboard mask_edge_rank = (uo_bitboard_rank[0] | uo_bitboard_rank[7]) & ~uo_bitboard_rank[rank];
    uo_bitboard mask_edge_file = (uo_bitboard_file[0] | uo_bitboard_file[7]) & ~uo_bitboard_file[file];
    uo_bitboard mask_edge = mask_edge_rank | mask_edge_file;
    uo_bitboard mask = uo_bitboard_file[file] | uo_bitboard_rank[rank];
    mask &= ~mask_edge;
    mask ^= uo_square_bitboard(square);

    uo_magics_R[square].mask = mask;
    int bits = uo_popcount_64(mask);
    uo_magics_R[square].shift = 64 - bits;
    int blocker_board_count = 1 << bits;
    blocker_board_count_total += blocker_board_count;
    blocker_board_count_max = blocker_board_count > blocker_board_count_max ? blocker_board_count : blocker_board_count_max;

    // printf("%c%d - %d\n", 'a' + uo_square_file(square), 1 + uo_square_rank(i), i);
    // uo_bitboard_print(mask);
    // printf("\n");
  }

  // 2. Allocate memory for blocker and move boards (and some extra space for temporary needs)
  uo_bitboard *boards = malloc((blocker_board_count_total + blocker_board_count_max * 2) * sizeof boards);

  // 3. Generate blocker boards and move boards
  for (uo_square square = 0; square < 64; ++square)
  {
    uo_magics_R[square].moves = boards;
    uo_magic magic = uo_magics_R[square];
    uo_bitboard mask = magic.mask;
    uint8_t bits = 64 - magic.shift;
    uint16_t permutation_count = 1 << bits;
    boards += permutation_count;
    // alternating list of blockers and moves
    uo_bitboard *blockers_moves = boards;

    // printf("mask\n");
    // uo_bitboard_print(mask);
    // printf("\n");

    for (uint16_t permutation = 0; permutation < permutation_count; ++permutation)
    {
      // 3.1. Create blocker board by setting masked bits on based on permutation binary representation
      uo_bitboard blockers = uo_bitboard_permutation(mask, bits, permutation);
      uo_bitboard moves = uo_bitboard_moves_R(blockers, square);

      *blockers_moves++ = blockers;
      *blockers_moves++ = moves;

      // printf("permutation: %d, \n", permutation);
      // printf("blockers\n");
      // uo_bitboard_print(blockers);
      // printf("moves\n");
      // uo_bitboard_print(moves);
      // printf("\n");
    }

    uo_magic_init(&uo_magics_R[square], boards, square, known_magics_R[square]);
  }
}

void uo_moves_init()
{
  if (init)
    return;
  init = true;

  rand_seed = time(NULL);
  srand(rand_seed);

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
