#include "uo_moves.h"
#include "uo_piece.h"
#include "uo_util.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

int rand_seed;

#ifndef UO_REGENERATE_MAGICS
#define UO_REGENERATE_MAGICS 0
#endif // !UO_REGENERATE_MAGICS

const uint64_t known_magics_P[64][2] = {
  // square      number               bits
  [0 /* a1 */] = {1, 1},
  [1 /* b1 */] = {1, 1},
  [2 /* c1 */] = {1, 1},
  [3 /* d1 */] = {1, 1},
  [4 /* e1 */] = {1, 1},
  [5 /* f1 */] = {1, 1},
  [6 /* g1 */] = {1, 1},
  [7 /* h1 */] = {1, 1},
  [8 /* a2 */] = {0x2000208208042021, 3},
  [9 /* b2 */] = {0x8000210c10040000, 4},
  [10 /* c2 */] = {0x4a300440034800, 4},
  [11 /* d2 */] = {0x20080410c0000200, 4},
  [12 /* e2 */] = {0x100861820000000, 4},
  [13 /* f2 */] = {0x8088420092c01006, 4},
  [14 /* g2 */] = {0xa5008200010111, 4},
  [15 /* h2 */] = {0x2800050640004080, 3},
  [16 /* a3 */] = {0xc20c200180801, 2},
  [17 /* b3 */] = {0x1050020a1000208, 3},
  [18 /* c3 */] = {0x140c480822a018, 3},
  [19 /* d3 */] = {0x200002800020000, 3},
  [20 /* e3 */] = {0x1000002408002901, 3},
  [21 /* f3 */] = {0x1002082210820128, 3},
  [22 /* g3 */] = {0x40108040008, 3},
  [23 /* h3 */] = {0x41098041000d0081, 2},
  [24 /* a4 */] = {0x2400001854804000, 2},
  [25 /* b4 */] = {0x100000249113080, 3},
  [26 /* c4 */] = {0xd8110254000, 3},
  [27 /* d4 */] = {0x1402008024082820, 3},
  [28 /* e4 */] = {0x88a000584201003, 3},
  [29 /* f4 */] = {0x824020300c810800, 3},
  [30 /* g4 */] = {0x1100100, 3},
  [31 /* h4 */] = {0x441022701080004, 2},
  [32 /* a5 */] = {0x804800480410000, 2},
  [33 /* b5 */] = {0x14242022000d, 3},
  [34 /* c5 */] = {0x40041510100040, 3},
  [35 /* d5 */] = {0x1a004c0000121056, 3},
  [36 /* e5 */] = {0x8210034044000, 3},
  [37 /* f5 */] = {0x2100460220400, 3},
  [38 /* g5 */] = {0x240400102010006, 3},
  [39 /* h5 */] = {0x5000800400111400, 2},
  [40 /* a6 */] = {0x80080104002, 2},
  [41 /* b6 */] = {0x44030a10226001, 3},
  [42 /* c6 */] = {0x4c00880821021004, 3},
  [43 /* d6 */] = {0x80021010002c4910, 3},
  [44 /* e6 */] = {0x410410408, 3},
  [45 /* f6 */] = {0x704008800000641, 3},
  [46 /* g6 */] = {0x1000020008000105, 3},
  [47 /* h6 */] = {0x1440001601100, 2},
  [48 /* a7 */] = {0x22200002d0008047, 2},
  [49 /* b7 */] = {0x20a280000400220, 3},
  [50 /* c7 */] = {0x880000000020024, 3},
  [51 /* d7 */] = {0x3228200080048048, 3},
  [52 /* e7 */] = {0x840000003800014, 3},
  [53 /* f7 */] = {0x400400000620202, 3},
  [54 /* g7 */] = {0x2122400008841, 3},
  [55 /* h7 */] = {0x1608000002000001, 2},
  [56 /* a8 */] = {1, 1},
  [57 /* b8 */] = {1, 1},
  [58 /* c8 */] = {1, 1},
  [59 /* d8 */] = {1, 1},
  [60 /* e8 */] = {1, 1},
  [61 /* f8 */] = {1, 1},
  [62 /* g8 */] = {1, 1},
  [63 /* h8 */] = {1, 1}
};

const uint64_t known_magics_B[64][2] = {
  // square      number               bits
  [0 /* a1 */] = {0x8a1f201b33986ffd, 6},
  [1 /* b1 */] = {0x34895eac01c601fe, 5},
  [2 /* c1 */] = {0x66105c07cacbb2f3, 5},
  [3 /* d1 */] = {0xd7b444008513da98, 5},
  [4 /* e1 */] = {0xcb241ca01af60730, 5},
  [5 /* f1 */] = {0x8f4a699f7e205224, 5},
  [6 /* g1 */] = {0xee69d72144762abd, 5},
  [7 /* h1 */] = {0x73c3d873d077ffe8, 6},
  [8 /* a2 */] = {0x77ffef1bc64415f1, 5},
  [9 /* b2 */] = {0x8b793aeec4dc03f5, 5},
  [10 /* c2 */] = {0x1027281241420701, 5},
  [11 /* d2 */] = {0x8608f808e502d0ea, 5},
  [12 /* e2 */] = {0x715ada0a10492061, 5},
  [13 /* f2 */] = {0x5ae041c77cfd6f46, 5},
  [14 /* g2 */] = {0x9387140219103871, 5},
  [15 /* h2 */] = {0x8067c5f6cedeff19, 5},
  [16 /* a3 */] = {0x481067c182262c37, 5},
  [17 /* b3 */] = {0xb1641c2907984ff8, 5},
  [18 /* c3 */] = {0x8ea84d5010820170, 7},
  [19 /* d3 */] = {0x4aa8021c04223a4d, 7},
  [20 /* e3 */] = {0xbdf2027c0211308a, 7},
  [21 /* f3 */] = {0x3c900c6010e8e1e, 7},
  [22 /* g3 */] = {0xa4ddc54eef11beb3, 5},
  [23 /* h3 */] = {0x57166358c73c3fbe, 5},
  [24 /* a4 */] = {0x5ae0114e94ac380c, 5},
  [25 /* b4 */] = {0xc7dfbd5f3a080911, 5},
  [26 /* c4 */] = {0x84cc010ff0050723, 7},
  [27 /* d4 */] = {0x44080080620040, 9},
  [28 /* e4 */] = {0x202840020802001, 9},
  [29 /* f4 */] = {0x4030010400804100, 7},
  [30 /* g4 */] = {0x801c028440421000, 5},
  [31 /* h4 */] = {0x1c90820202220200, 5},
  [32 /* a5 */] = {0x2804b0400500, 5},
  [33 /* b5 */] = {0x4c01680200a00400, 5},
  [34 /* c5 */] = {0x40102408100, 7},
  [35 /* d5 */] = {0x4c005100004040, 9},
  [36 /* e5 */] = {0x848010040100802, 9},
  [37 /* f5 */] = {0x1004080020121004, 7},
  [38 /* g5 */] = {0x408808144088a200, 5},
  [39 /* h5 */] = {0x88801202103008c, 5},
  [40 /* a6 */] = {0x2800820000b80, 5},
  [41 /* b6 */] = {0x1800002111042000, 5},
  [42 /* c6 */] = {0x2310000010080880, 7},
  [43 /* d6 */] = {0x8020a21801000180, 7},
  [44 /* e6 */] = {0x441000400080, 7},
  [45 /* f6 */] = {0x2200041000480, 7},
  [46 /* g6 */] = {0x2088008100008d, 5},
  [47 /* h6 */] = {0x1044008482100300, 5},
  [48 /* a7 */] = {0x550, 5},
  [49 /* b7 */] = {0x6206, 5},
  [50 /* c7 */] = {0x103800a080600405, 5},
  [51 /* d7 */] = {0x1205000040022000, 5},
  [52 /* e7 */] = {0x91820b4098484102, 5},
  [53 /* f7 */] = {0x2c02e02404080060, 5},
  [54 /* g7 */] = {0x20142004840182a0, 5},
  [55 /* h7 */] = {0xc0204140400c80a, 5},
  [56 /* a8 */] = {0x7850, 6},
  [57 /* b8 */] = {0x5a5f, 5},
  [58 /* c8 */] = {0x4c0f, 5},
  [59 /* d8 */] = {0x901004180082000, 5},
  [60 /* e8 */] = {0x2000980222020400, 5},
  [61 /* f8 */] = {0x8008041420114401, 5},
  [62 /* g8 */] = {0x901b0204080206, 5},
  [63 /* h8 */] = {0x524040806140010, 6}
};

const uint64_t known_magics_R[64][2] = {
  //square      number               bits
 [0 /* a1 */] = {0x177801395e43fffa, 12},
 [1 /* b1 */] = {0x8400020009007c4,  11},
 [2 /* c1 */] = {0x80100020008008, 11},
 [3 /* d1 */] = {0x1100100028050021, 11},
 [4 /* e1 */] = {0x8180028008000c00, 11},
 [5 /* f1 */] = {0x2300010026040048, 11},
 [6 /* g1 */] = {0x1000c0082001700, 11},
 [7 /* h1 */] = {0x8880008010214300, 12},
 [8 /* a2 */] = {0x8080800040008024, 11},
 [9 /* b2 */] = {0x401008402000, 10},
 [10 /* c2 */] = {0x11002005110040, 10},
 [11 /* d2 */] = {0x80a1002104085000, 10},
 [12 /* e2 */] = {0x80800c00800802, 10},
 [13 /* f2 */] = {0x1001618840100, 10},
 [14 /* g2 */] = {0x2002802008441, 10},
 [15 /* h2 */] = {0x8088000c9000080, 11},
 [16 /* a3 */] = {0x82c0808000400028, 11},
 [17 /* b3 */] = {0x4020008020804000, 10},
 [18 /* c3 */] = {0x20600280100084a0, 10},
 [19 /* d3 */] = {0x2004808010002800, 10},
 [20 /* e3 */] = {0x8008009040080, 10},
 [21 /* f3 */] = {0x2040808014000200, 10},
 [22 /* g3 */] = {0x40008490230, 10},
 [23 /* h3 */] = {0x244020008609401, 11},
 [24 /* a4 */] = {0x800400080008020, 11},
 [25 /* b4 */] = {0x210004040026000, 10},
 [26 /* c4 */] = {0x1000120200408020, 10},
 [27 /* d4 */] = {0x1008100100490020, 10},
 [28 /* e4 */] = {0x25080080040081, 10},
 [29 /* f4 */] = {0x3022000200181005, 10},
 [30 /* g4 */] = {0x8430001000c0200, 10},
 [31 /* h4 */] = {0x1c001020000846c, 11},
 [32 /* a5 */] = {0xc00320c001800080, 11},
 [33 /* b5 */] = {0x802000804001, 10},
 [34 /* c5 */] = {0x200200249001100, 10},
 [35 /* d5 */] = {0x4208100280800800, 10},
 [36 /* e5 */] = {0x4082804800804400, 10},
 [37 /* f5 */] = {0x2006003006000805, 10},
 [38 /* g5 */] = {0x4200220804000110, 10},
 [39 /* h5 */] = {0x2246e402000081, 11},
 [40 /* a6 */] = {0x2040014020808000, 11},
 [41 /* b6 */] = {0x440003008006002, 10},
 [42 /* c6 */] = {0x8088624200820012, 10},
 [43 /* d6 */] = {0x28301a010050008, 10},
 [44 /* e6 */] = {0x8004080091010004, 10},
 [45 /* f6 */] = {0x81200191c820010, 10},
 [46 /* g6 */] = {0x401000200010004, 10},
 [47 /* h6 */] = {0x10400e8820001, 11},
 [48 /* a7 */] = {0x1002c00080012080, 11},
 [49 /* b7 */] = {0x41440822a050200, 10},
 [50 /* c7 */] = {0x20080a1144200, 10},
 [51 /* d7 */] = {0xc10001084180080, 10},
 [52 /* e7 */] = {0x407080181040080, 10},
 [53 /* f7 */] = {0x4190a040042801, 10},
 [54 /* g7 */] = {0x10830022400, 10},
 [55 /* h7 */] = {0x8100004c00830a00, 11},
 [56 /* a8 */] = {0x88800141122101, 12},
 [57 /* b8 */] = {0x329020885100c001, 11},
 [58 /* c8 */] = {0x1002008030203942, 11},
 [59 /* d8 */] = {0x800190144300021, 11},
 [60 /* e8 */] = {0x80e001020840802, 11},
 [61 /* f8 */] = {0x8742001008294402, 11},
 [62 /* g8 */] = {0x4100069c020001, 11},
 [63 /* h8 */] = {0x1032080c406, 12}
};

static bool init;

uo_bitboard uo_moves_K[64];
uo_bitboard uo_moves_N[64];

typedef struct uo_magic
{
  uo_bitboard* moves;
  uo_bitboard mask;
  uint64_t number;
  uint8_t shift;
} uo_magic;

uo_magic uo_magics_P[64];
uo_magic uo_magics_B[64];
uo_magic uo_magics_R[64];

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

static inline uo_bitboard uo_bitboard_gen_mask_P(uo_square square)
{
  int rank = uo_square_rank(square);
  int file = uo_square_file(square);
  uo_bitboard mask = uo_square_bitboard(square) & ~uo_bitboard_rank[0] & ~uo_bitboard_rank[7];
  uo_bitboard mask_rank_2nd = mask & uo_bitboard_rank[1];
  uo_bitboard mask_not_file_a = mask & ~uo_bitboard_file[0];
  uo_bitboard mask_not_file_h = mask & ~uo_bitboard_file[7];
  uo_bitboard one = mask << 8;
  uo_bitboard two = mask_rank_2nd << 16;
  uo_bitboard left = mask_not_file_a << 7;
  uo_bitboard right = mask_not_file_h << 9;

  mask = one | two | left | right;

  return mask;
}

static inline uo_bitboard uo_bitboard_gen_mask_B(uo_square square)
{
  int rank = uo_square_rank(square);
  int file = uo_square_file(square);
  uo_bitboard mask_edge_rank = (uo_bitboard_rank[0] | uo_bitboard_rank[7]) & ~uo_bitboard_rank[rank];
  uo_bitboard mask_edge_file = (uo_bitboard_file[0] | uo_bitboard_file[7]) & ~uo_bitboard_file[file];
  uo_bitboard mask_edge = mask_edge_rank | mask_edge_file;
  int diagonal = uo_square_diagonal[square];
  int antidiagonal = uo_square_antidiagonal[square];
  uo_bitboard mask = uo_bitboard_diagonal[diagonal] | uo_bitboard_antidiagonal[antidiagonal];
  mask &= ~mask_edge;
  mask ^= uo_square_bitboard(square);
  return mask;
}

static inline uo_bitboard uo_bitboard_gen_mask_R(uo_square square)
{
  int rank = uo_square_rank(square);
  int file = uo_square_file(square);
  uo_bitboard mask_edge_rank = (uo_bitboard_rank[0] | uo_bitboard_rank[7]) & ~uo_bitboard_rank[rank];
  uo_bitboard mask_edge_file = (uo_bitboard_file[0] | uo_bitboard_file[7]) & ~uo_bitboard_file[file];
  uo_bitboard mask_edge = mask_edge_rank | mask_edge_file;
  uo_bitboard mask = uo_bitboard_file[file] | uo_bitboard_rank[rank];
  mask &= ~mask_edge;
  mask ^= uo_square_bitboard(square);
  return mask;
}

static inline uo_bitboard uo_bitboard_gen_moves_P(uo_bitboard blockers, uo_square square)
{
  int rank = uo_square_rank(square);
  int file = uo_square_file(square);
  uo_bitboard mask = uo_square_bitboard(square);
  uo_bitboard moves = 0;

  // push one
  if (rank > 0 && rank < 7)
  {
    moves |= (mask << 8) & ~blockers;

    // push two
    if (moves && rank == 1)
    {
      moves |= (mask << 16) & ~blockers;
    }

    // capture left
    if (file > 0)
    {
      moves |= (mask << 7) & blockers;
    }

    // capture right
    if (file < 7)
    {
      moves |= (mask << 9) & blockers;
    }
  }

  return moves;
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
  for (size_t i = 1; (i < 8 - file) && (i < 8 - rank); i++)
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

static inline void uo_magic_init(uo_magic* magic, uo_bitboard* blockers_moves /* alternating list of blockers and moves*/, uo_square square, const uint64_t known_magic[2])
{
  if (known_magic && known_magic[0])
  {
    magic->number = known_magic[0];
    magic->shift = 64 - known_magic[1];
  }

  uint16_t count = 1 << uo_popcount(magic->mask);
  uint8_t shift = magic->shift;
  uint8_t bits = 64 - shift;
  uint64_t number = magic->number ? magic->number : (uo_rand() & uo_rand() & uo_rand());
  uint16_t record = 0;

  while (true)
  {
    memset(magic->moves, 0, count * sizeof * magic->moves);

    uo_bitboard* ptr = blockers_moves;

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
        //if (i > record) {
        //    record = i;
        //    printf("record %d/%d\n", record, count);
        //}

        number = uo_rand() & uo_rand() & uo_rand();

        goto try_next_number;
      }
    }

    if (!known_magic || !known_magic[0])
    {
      // suitable magic number was found
      printf("suitable magic number was found\n");
      printf("rand_seed: %d\n", rand_seed);
      printf("square: %d\n", square);
      printf("bits: %d\n", bits);
      printf("number: 0x%" PRIx64 "\n", number);
      printf("\n");
      magic->number = number;
    }

    return;
  try_next_number:;
  }
}

void uo_magics_P_init(void)
{
  // 1. Generate blocker masks

  int blocker_board_count_total = 0;
  int blocker_board_count_max = 0;

  for (uo_square square = 0; square < 64; ++square)
  {
    uo_bitboard mask = uo_bitboard_gen_mask_P(square);

    uo_magics_P[square].mask = mask;
    int bits = uo_popcount(mask);
    uo_magics_P[square].shift = 64 - bits;
    int blocker_board_count = 1 << bits;
    blocker_board_count_total += blocker_board_count;
    blocker_board_count_max = blocker_board_count > blocker_board_count_max ? blocker_board_count : blocker_board_count_max;

    // printf("%c%d - %d\n", 'a' + uo_square_file(square), 1 + uo_square_rank(i), i);
    // uo_bitboard_print(mask);
    // printf("\n");
  }

  // 2. Allocate memory for blocker and move boards (and some extra space for temporary needs)
  uo_bitboard* boards = malloc((blocker_board_count_total + blocker_board_count_max * 2) * sizeof boards);

  // 3. Generate blocker boards and move boards
  for (uo_square square = 0; square < 64; ++square)
  {
    uo_magics_P[square].moves = boards;
    uo_magic magic = uo_magics_P[square];
    uo_bitboard mask = magic.mask;
    uint8_t bits = 64 - magic.shift;
    uint16_t permutation_count = 1 << bits;
    boards += permutation_count;
    // alternating list of blockers and moves
    uo_bitboard* blockers_moves = boards;

    // printf("mask\n");
    // uo_bitboard_print(mask);
    // printf("\n");

    for (uint16_t permutation = 0; permutation < permutation_count; ++permutation)
    {
      // 3.1. Create blocker board by setting masked bits on based on permutation binary representation
      uo_bitboard blockers = uo_bitboard_permutation(mask, bits, permutation);
      uo_bitboard moves = uo_bitboard_gen_moves_P(blockers, square);

      *blockers_moves++ = blockers;
      *blockers_moves++ = moves;

      // printf("permutation: %d, \n", permutation);
      // printf("blockers\n");
      // uo_bitboard_print(blockers);
      // printf("moves\n");
      // uo_bitboard_print(moves);
      // printf("\n");
    }

    uo_magic_init(&uo_magics_P[square], boards, square, UO_REGENERATE_MAGICS ? NULL : known_magics_P[square]);
  }
}

void uo_magics_B_init(void)
{
  // 1. Generate blocker masks

  int blocker_board_count_total = 0;
  int blocker_board_count_max = 0;

  for (uo_square square = 0; square < 64; ++square)
  {
    uo_bitboard mask = uo_bitboard_gen_mask_B(square);

    uo_magics_B[square].mask = mask;
    int bits = uo_popcount(mask);
    uo_magics_B[square].shift = 64 - bits;
    int blocker_board_count = 1 << bits;
    blocker_board_count_total += blocker_board_count;
    blocker_board_count_max = blocker_board_count > blocker_board_count_max ? blocker_board_count : blocker_board_count_max;

    // printf("%c%d - %d\n", 'a' + uo_square_file(square), 1 + uo_square_rank(i), i);
    // uo_bitboard_print(mask);
    // printf("\n");
  }

  // 2. Allocate memory for blocker and move boards (and some extra space for temporary needs)
  uo_bitboard* boards = malloc((blocker_board_count_total + blocker_board_count_max * 2) * sizeof boards);

  // 3. Generate blocker boards and move boards
  for (uo_square square = 0; square < 64; ++square)
  {
    uo_magics_B[square].moves = boards;
    uo_magic magic = uo_magics_B[square];
    uo_bitboard mask = magic.mask;
    uint8_t bits = 64 - magic.shift;
    uint16_t permutation_count = 1 << bits;
    boards += permutation_count;
    // alternating list of blockers and moves
    uo_bitboard* blockers_moves = boards;

    // printf("mask\n");
    // uo_bitboard_print(mask);
    // printf("\n");

    for (uint16_t permutation = 0; permutation < permutation_count; ++permutation)
    {
      // 3.1. Create blocker board by setting masked bits on based on permutation binary representation
      uo_bitboard blockers = uo_bitboard_permutation(mask, bits, permutation);
      uo_bitboard moves = uo_bitboard_gen_moves_B(blockers, square);

      *blockers_moves++ = blockers;
      *blockers_moves++ = moves;

      //printf("permutation: %d, \n", permutation);
      //printf("blockers\n");
      //uo_bitboard_print(blockers);
      //printf("moves\n");
      //uo_bitboard_print(moves);
      //printf("\n");
    }

    uo_magic_init(&uo_magics_B[square], boards, square, UO_REGENERATE_MAGICS ? NULL : known_magics_B[square]);
  }
}

void uo_magics_R_init(void)
{
  // 1. Generate blocker masks

  int blocker_board_count_total = 0;
  int blocker_board_count_max = 0;

  for (uo_square square = 0; square < 64; ++square)
  {
    uo_bitboard mask = uo_bitboard_gen_mask_R(square);

    uo_magics_R[square].mask = mask;
    int bits = uo_popcount(mask);
    uo_magics_R[square].shift = 64 - bits;
    int blocker_board_count = 1 << bits;
    blocker_board_count_total += blocker_board_count;
    blocker_board_count_max = blocker_board_count > blocker_board_count_max ? blocker_board_count : blocker_board_count_max;

    // printf("%c%d - %d\n", 'a' + uo_square_file(square), 1 + uo_square_rank(i), i);
    // uo_bitboard_print(mask);
    // printf("\n");
  }

  // 2. Allocate memory for blocker and move boards (and some extra space for temporary needs)
  uo_bitboard* boards = malloc((blocker_board_count_total + blocker_board_count_max * 2) * sizeof boards);

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
    uo_bitboard* blockers_moves = boards;

    // printf("mask\n");
    // uo_bitboard_print(mask);
    // printf("\n");

    for (uint16_t permutation = 0; permutation < permutation_count; ++permutation)
    {
      // 3.1. Create blocker board by setting masked bits on based on permutation binary representation
      uo_bitboard blockers = uo_bitboard_permutation(mask, bits, permutation);
      uo_bitboard moves = uo_bitboard_gen_moves_R(blockers, square);

      *blockers_moves++ = blockers;
      *blockers_moves++ = moves;

      // printf("permutation: %d, \n", permutation);
      // printf("blockers\n");
      // uo_bitboard_print(blockers);
      // printf("moves\n");
      // uo_bitboard_print(moves);
      // printf("\n");
    }

    uo_magic_init(&uo_magics_R[square], boards, square, UO_REGENERATE_MAGICS ? NULL : known_magics_R[square]);
  }
}

bool uo_position_is_check(uo_position* pos, bool white)
{
  uo_bitboard occupied = pos->p | pos->n | pos->b | pos->r | pos->q | pos->k;
  uo_bitboard mask_own = occupied & pos->black_piece;
  uo_bitboard mask_enemy = occupied & ~mask_own;
  uo_bitboard temp;

  if (white)
  {
    temp = mask_own;
    mask_own = mask_enemy;
    mask_enemy = temp;
  }

  uo_bitboard k = pos->k & mask_own;
  uo_square i;

  i = -1;
  uo_bitboard k_enemy = pos->k & mask_enemy;
  while (uo_bitboard_next_square(k_enemy, &i))
  {
    if (k & uo_moves_K[i])
    {
      return true;
    }
  }

  i = -1;
  uo_bitboard n_enemy = pos->n & mask_enemy;
  while (uo_bitboard_next_square(n_enemy, &i))
  {
    if (k & uo_moves_N[i])
    {
      return true;
    }
  }

  i = -1;
  uo_bitboard b_enemy = pos->b & mask_enemy;
  while (uo_bitboard_next_square(b_enemy, &i))
  {
    uo_magic magic = uo_magics_B[i];
    int index = ((occupied & magic.mask) * magic.number) >> magic.shift;
    uo_bitboard moves = magic.moves[index];

    if (k & moves)
    {
      return true;
    }
  }

  i = -1;
  uo_bitboard r_enemy = pos->r & mask_enemy;
  while (uo_bitboard_next_square(r_enemy, &i))
  {
    uo_magic magic = uo_magics_R[i];
    int index = ((occupied & magic.mask) * magic.number) >> magic.shift;
    uo_bitboard moves = magic.moves[index];

    if (k & moves)
    {
      return true;
    }
  }

  i = -1;
  uo_bitboard q_enemy = pos->q & mask_enemy;
  while (uo_bitboard_next_square(q_enemy, &i))
  {
    {
      uo_magic magic = uo_magics_R[i];
      int index = ((occupied & magic.mask) * magic.number) >> magic.shift;
      uo_bitboard moves = magic.moves[index];

      if (k & moves)
      {
        return true;
      }
    }
    {
      uo_magic magic = uo_magics_B[i];
      int index = ((occupied & magic.mask) * magic.number) >> magic.shift;
      uo_bitboard moves = magic.moves[index];

      if (k & moves)
      {
        return true;
      }
    }
  }

  i = -1;
  uo_bitboard p_enemy = pos->p & mask_enemy;

  while (uo_bitboard_next_square(p_enemy, &i))
  {
    uo_bitboard p = uo_square_bitboard(i);
    uo_bitboard moves = white ? ((p >> 7) | (p >> 9)) : ((p << 7) | (p << 9));

    if (k & moves)
    {
      return true;
    }
  }

  return false;
}

uo_bitboard uo_position_moves(uo_position* pos, uo_square square, uo_position nodes[27 /* maximum number of legal moves for a single piece (queen) */])
{
  uo_bitboard mask = uo_square_bitboard(square);
  uo_bitboard occupied = pos->p | pos->n | pos->b | pos->r | pos->q | pos->k;
  uo_bitboard mask_own = occupied & pos->black_piece;
  uo_bitboard mask_enemy = occupied & ~mask_own;
  uo_bitboard temp;

  if (pos->white_to_move)
  {
    temp = mask_own;
    mask_own = mask_enemy;
    mask_enemy = temp;
  }

  mask &= mask_own;

  if (!mask) return 0;

  if (pos->p & mask)
  {
    uo_magic magic = uo_magics_P[square];
    int index = ((occupied & magic.mask) * magic.number) >> magic.shift;
    uo_bitboard moves = magic.moves[index] & ~mask_own;

    //printf("\nsquare: %d\n", square);
    //printf("\noccupied\n");
    //uo_bitboard_print(occupied);
    //printf("\nmask_own\n");
    //uo_bitboard_print(mask_own);
    //printf("\nmask_enemy\n");
    //uo_bitboard_print(mask_enemy);
    //printf("\nblocker mask\n");
    //uo_bitboard_print(magic.mask);
    //printf("\nmask\n");
    //uo_bitboard_print(mask);
    //printf("\nmoves\n");
    //uo_bitboard_print(moves);

    uo_bitboard enpassant = uo_square_bitboard(pos->enpassant);
    enpassant &= (mask << 9) | (mask << 7);
    moves |= enpassant;

    uo_square i = -1;

    while (uo_bitboard_next_square(moves, &i))
    {
      uo_bitboard move = uo_square_bitboard(i);

      *nodes = *pos;
      nodes->p &= ~mask;
      nodes->p |= move;
      nodes->n &= ~move;
      nodes->b &= ~move;
      nodes->r &= ~move;
      nodes->q &= ~move;

      if (pos->white_to_move)
      {
        nodes->black_piece &= ~move;
      }
      else
      {
        nodes->black_piece |= move;
      }

      // en passant
      if (move == (mask << 16))
      {
        nodes->enpassant = square + 8;
        nodes->p &= ~uo_square_bitboard(nodes->enpassant);
      }

      nodes->halfmoves = 0;
      ++nodes->fullmove;
      nodes->white_to_move = !pos->white_to_move;

      if (uo_position_is_check(nodes, pos->white_to_move))
      {
        moves ^= move;
      }
      else
      {
        ++nodes;
      }
    }

    return moves;
  }

  if (pos->n & mask)
  {
    uo_bitboard moves = uo_moves_N[square] & ~mask_own;

    uo_square i = -1;

    while (uo_bitboard_next_square(moves, &i))
    {
      uo_bitboard move = uo_square_bitboard(i);

      *nodes = *pos;
      nodes->n &= ~mask;
      nodes->n |= move;
      nodes->p &= ~move;
      nodes->b &= ~move;
      nodes->r &= ~move;
      nodes->q &= ~move;

      if (pos->white_to_move)
      {
        nodes->black_piece &= ~move;
      }
      else
      {
        nodes->black_piece |= move;
      }

      nodes->enpassant = 0;
      nodes->halfmoves = 0;
      ++nodes->fullmove;
      nodes->white_to_move = !pos->white_to_move;

      if (uo_position_is_check(nodes, pos->white_to_move))
      {
        moves ^= move;
      }
      else
      {
        ++nodes;
      }
    }

    return moves;
  }

  if (pos->b & mask)
  {
    uo_magic magic = uo_magics_B[square];
    int index = ((occupied & magic.mask) * magic.number) >> magic.shift;
    uo_bitboard moves = magic.moves[index] & ~mask_own;

    uo_square i = -1;

    while (uo_bitboard_next_square(moves, &i))
    {
      uo_bitboard move = uo_square_bitboard(i);

      *nodes = *pos;
      nodes->b &= ~mask;
      nodes->b |= move;
      nodes->p &= ~move;
      nodes->n &= ~move;
      nodes->r &= ~move;
      nodes->q &= ~move;

      if (pos->white_to_move)
      {
        nodes->black_piece &= ~move;
      }
      else
      {
        nodes->black_piece |= move;
      }

      nodes->enpassant = 0;
      nodes->halfmoves = 0;
      ++nodes->fullmove;
      nodes->white_to_move = !pos->white_to_move;

      if (uo_position_is_check(nodes, pos->white_to_move))
      {
        moves ^= move;
      }
      else
      {
        ++nodes;
      }
    }

    return moves;
  }

  if (pos->r & mask)
  {
    uo_magic magic = uo_magics_R[square];
    int index = ((occupied & magic.mask) * magic.number) >> magic.shift;
    uo_bitboard moves = magic.moves[index] & ~mask_own;

    uo_square i = -1;

    while (uo_bitboard_next_square(moves, &i))
    {
      uo_bitboard move = uo_square_bitboard(i);

      *nodes = *pos;
      nodes->r &= ~mask;
      nodes->r |= move;
      nodes->p &= ~move;
      nodes->n &= ~move;
      nodes->b &= ~move;
      nodes->q &= ~move;

      if (pos->white_to_move)
      {
        nodes->black_piece &= ~move;
      }
      else
      {
        nodes->black_piece |= move;
      }

      nodes->enpassant = 0;
      nodes->halfmoves = 0;
      ++nodes->fullmove;
      nodes->white_to_move = !pos->white_to_move;

      if (uo_position_is_check(nodes, pos->white_to_move))
      {
        moves ^= move;
      }
      else
      {
        ++nodes;
      }
    }

    if (pos->k & mask)
    {
      uo_bitboard moves = uo_moves_K[square] & ~mask_own;

      uo_square i = -1;

      while (uo_bitboard_next_square(moves, &i))
      {
        uo_bitboard move = uo_square_bitboard(i);

        *nodes = *pos;
        nodes->k &= ~mask;
        nodes->k |= move;
        nodes->p &= ~move;
        nodes->n &= ~move;
        nodes->b &= ~move;
        nodes->r &= ~move;
        nodes->q &= ~move;

        if (pos->white_to_move)
        {
          nodes->black_piece &= ~move;
        }
        else
        {
          nodes->black_piece |= move;
        }

        nodes->enpassant = 0;
        nodes->halfmoves = 0;
        ++nodes->fullmove;
        nodes->white_to_move = !pos->white_to_move;

        if (uo_position_is_check(nodes, pos->white_to_move))
        {
          moves ^= move;
        }
        else
        {
          ++nodes;
        }
      }

      return moves;
    }


    return moves;
  }

  if (pos->q & mask)
  {
    uo_bitboard moves = 0;

    {
      uo_magic magic = uo_magics_B[square];
      int index = ((occupied & magic.mask) * magic.number) >> magic.shift;
      moves |= magic.moves[index] & ~mask_own;
    }
    {
      uo_magic magic = uo_magics_R[square];
      int index = ((occupied & magic.mask) * magic.number) >> magic.shift;
      moves |= magic.moves[index] & ~mask_own;
    }

    uo_square i = -1;

    while (uo_bitboard_next_square(moves, &i))
    {
      uo_bitboard move = uo_square_bitboard(i);

      *nodes = *pos;
      nodes->r &= ~mask;
      nodes->r |= move;
      nodes->p &= ~move;
      nodes->n &= ~move;
      nodes->b &= ~move;
      nodes->q &= ~move;

      if (pos->white_to_move)
      {
        nodes->black_piece &= ~move;
      }
      else
      {
        nodes->black_piece |= move;
      }

      nodes->enpassant = 0;
      nodes->halfmoves = 0;
      ++nodes->fullmove;
      nodes->white_to_move = !pos->white_to_move;

      if (uo_position_is_check(nodes, pos->white_to_move))
      {
        moves ^= move;
      }
      else
      {
        ++nodes;
      }
    }

    return moves;
  }

  if (pos->k & mask)
  {
    uo_bitboard moves = uo_moves_K[square] & ~mask_own;

    uo_square i = -1;

    while (uo_bitboard_next_square(moves, &i))
    {
      uo_bitboard move = uo_square_bitboard(i);

      *nodes = *pos;
      nodes->k &= ~mask;
      nodes->k |= move;
      nodes->p &= ~move;
      nodes->n &= ~move;
      nodes->b &= ~move;
      nodes->r &= ~move;
      nodes->q &= ~move;

      if (pos->white_to_move)
      {
        nodes->black_piece &= ~move;
      }
      else
      {
        nodes->black_piece |= move;
      }

      nodes->enpassant = 0;
      nodes->halfmoves = 0;
      ++nodes->fullmove;
      nodes->white_to_move = !pos->white_to_move;

      if (uo_position_is_check(nodes, pos->white_to_move))
      {
        moves ^= move;
      }
      else
      {
        ++nodes;
      }
    }

    return moves;
  }

  return 0;
}

typedef struct uo_movelist
{
  uo_position* position;
  uo_square square;
  uo_bitboard moves;
  uo_position* nodes;

} uo_movelist;

void uo_moves_init()
{
  if (init)
    return;
  init = true;

  rand_seed = time(NULL);
  uo_rand_init(rand_seed);

  uo_moves_K_init();
  uo_moves_N_init();
  uo_magics_B_init();
  uo_magics_R_init();
  uo_magics_P_init();
}

bool uo_test_moves()
{
  bool passed = true;

  return passed;
}
