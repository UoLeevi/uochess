#include "uo_bitboard.h"
#include "uo_util.h"
#include "uo_piece.h"
#include "uo_util.h"

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


#ifndef UO_REGENERATE_MAGICS
#define UO_REGENERATE_MAGICS 0
#endif // !UO_REGENERATE_MAGICS

#pragma region known_magics

const uint64_t known_magics_P[2][64][2] = {
  [0 /* white */] = {
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
    },
  [1 /* black */] = {
    // square      number               bits
    [0 /* a1 */] = {1, 1},
    [1 /* b1 */] = {1, 1},
    [2 /* c1 */] = {1, 1},
    [3 /* d1 */] = {1, 1},
    [4 /* e1 */] = {1, 1},
    [5 /* f1 */] = {1, 1},
    [6 /* g1 */] = {1, 1},
    [7 /* h1 */] = {1, 1},
    [8 /* a2 */] = {0xe080018000006001, 2},
    [9 /* b2 */] = {0x4800001000000210, 3},
    [10 /* c2 */] = {0x1003008040180400, 3},
    [11 /* d2 */] = {0x840002000000220, 3},
    [12 /* e2 */] = {0x2400000080473100, 3},
    [13 /* f2 */] = {0xa200081028314004, 3},
    [14 /* g2 */] = {0x102000020000000, 3},
    [15 /* h2 */] = {0x4500009010080000, 2},
    [16 /* a3 */] = {0xa000602240008c, 2},
    [17 /* b3 */] = {0x22082000a00826, 3},
    [18 /* c3 */] = {0x1110800000800028, 3},
    [19 /* d3 */] = {0x4840020004a040, 3},
    [20 /* e3 */] = {0xa0c075004004000, 3},
    [21 /* f3 */] = {0xa2020ca214000484, 3},
    [22 /* g3 */] = {0x9001000000000041, 3},
    [23 /* h3 */] = {0x41204001020081, 2},
    [24 /* a4 */] = {0x4402001001014, 2},
    [25 /* b4 */] = {0x242804000000, 3},
    [26 /* c4 */] = {0x4200900000808004, 3},
    [27 /* d4 */] = {0x304583301006c00, 3},
    [28 /* e4 */] = {0x2020041008000804, 3},
    [29 /* f4 */] = {0x21000150030, 3},
    [30 /* g4 */] = {0x300410000883008, 3},
    [31 /* h4 */] = {0x1500110400000001, 2},
    [32 /* a5 */] = {0x91004080410000, 2},
    [33 /* b5 */] = {0x401802001206000, 3},
    [34 /* c5 */] = {0x5080000000, 3},
    [35 /* d5 */] = {0x41040800000040, 3},
    [36 /* e5 */] = {0x800402000002, 3},
    [37 /* f5 */] = {0x9000203200800040, 3},
    [38 /* g5 */] = {0x200404100000102, 3},
    [39 /* h5 */] = {0x48208201022b0060, 2},
    [40 /* a6 */] = {0x410001270844008, 2},
    [41 /* b6 */] = {0x20d000048200400, 3},
    [42 /* c6 */] = {0x1c0065485800, 3},
    [43 /* d6 */] = {0x4000100808001008, 3},
    [44 /* e6 */] = {0x1a00044008020, 3},
    [45 /* f6 */] = {0xa011000109010100, 3},
    [46 /* g6 */] = {0x8000008001000082, 3},
    [47 /* h6 */] = {0x2300a6800080, 2},
    [48 /* a7 */] = {0x4040008088208024, 3},
    [49 /* b7 */] = {0x1000088a04008, 4},
    [50 /* c7 */] = {0x60c9040004100900, 4},
    [51 /* d7 */] = {0x6480400010041091, 4},
    [52 /* e7 */] = {0x2102002008024000, 4},
    [53 /* f7 */] = {0x880010011820804, 4},
    [54 /* g7 */] = {0x100480a122008604, 4},
    [55 /* h7 */] = {0x400032021009000, 3},
    [56 /* a8 */] = {1, 1},
    [57 /* b8 */] = {1, 1},
    [58 /* c8 */] = {1, 1},
    [59 /* d8 */] = {1, 1},
    [60 /* e8 */] = {1, 1},
    [61 /* f8 */] = {1, 1},
    [62 /* g8 */] = {1, 1},
    [63 /* h8 */] = {1, 1}
    }
};

const uint64_t known_magics_B[64][2] = {
  // square      number               bits
  [0 /* a1 */] = { 0x5002021009060088, 6 },
  [1 /* b1 */] = { 0x2105202104000, 5 },
  [2 /* c1 */] = { 0x22322092020010a0, 5 },
  [3 /* d1 */] = { 0xa21424008004a004, 5 },
  [4 /* e1 */] = { 0x84242042009000, 5 },
  [5 /* f1 */] = { 0x1041100804010020, 5 },
  [6 /* g1 */] = { 0x204020104220000, 5 },
  [7 /* h1 */] = { 0x8100104402084304, 6 },
  [8 /* a2 */] = { 0x2003401004008488, 5 },
  [9 /* b2 */] = { 0x6820111002044141, 5 },
  [10 /* c2 */] = { 0x490610040280a400, 5 },
  [11 /* d2 */] = { 0x8282240420000, 5 },
  [12 /* e2 */] = { 0x41420010100, 5 },
  [13 /* f2 */] = { 0x820806082000, 5 },
  [14 /* g2 */] = { 0x4000040501086084, 5 },
  [15 /* h2 */] = { 0x4008810401210808, 5 },
  [16 /* a3 */] = { 0x4100d00a080104, 5 },
  [17 /* b3 */] = { 0x220001062120840, 5 },
  [18 /* c3 */] = { 0x40040002040c0008, 7 },
  [19 /* d3 */] = { 0x40018064008c0, 7 },
  [20 /* e3 */] = { 0x24000201a24048, 7 },
  [21 /* f3 */] = { 0x4902410600562004, 7 },
  [22 /* g3 */] = { 0x2080518020610, 5 },
  [23 /* h3 */] = { 0x4022080905009210, 5 },
  [24 /* a4 */] = { 0x56200010200201, 5 },
  [25 /* b4 */] = { 0x2020880501000b0, 5 },
  [26 /* c4 */] = { 0x10085002080020c0, 7 },
  [27 /* d4 */] = { 0x830140020440008, 9 },
  [28 /* e4 */] = { 0x41080413014003, 9 },
  [29 /* f4 */] = { 0x10b050002004901, 7 },
  [30 /* g4 */] = { 0x4402040122010104, 5 },
  [31 /* h4 */] = { 0xc012006042110118, 5 },
  [32 /* a5 */] = { 0x548400008f040, 5 },
  [33 /* b5 */] = { 0x4422020240503040, 5 },
  [34 /* c5 */] = { 0x1044c40210500020, 7 },
  [35 /* d5 */] = { 0x2008020080080082, 9 },
  [36 /* e5 */] = { 0x4008208040100, 9 },
  [37 /* f5 */] = { 0x8010008200882208, 7 },
  [38 /* g5 */] = { 0x320818204081052a, 5 },
  [39 /* h5 */] = { 0x402020200424140, 5 },
  [40 /* a6 */] = { 0x9101010020500, 5 },
  [41 /* b6 */] = { 0x841209010600408, 5 },
  [42 /* c6 */] = { 0x8018e8040c2800, 7 },
  [43 /* d6 */] = { 0x102019012800, 7 },
  [44 /* e6 */] = { 0x10410124004200, 7 },
  [45 /* f6 */] = { 0x20689000400080, 7 },
  [46 /* g6 */] = { 0x802101030301a210, 5 },
  [47 /* h6 */] = { 0x848510102080220, 5 },
  [48 /* a7 */] = { 0x580848029090000a, 5 },
  [49 /* b7 */] = { 0x4010404c2080000, 5 },
  [50 /* c7 */] = { 0x80006004a080908, 5 },
  [51 /* d7 */] = { 0x8160080a420a0400, 5 },
  [52 /* e7 */] = { 0x4180352405040100, 5 },
  [53 /* f7 */] = { 0x2040494080a0020, 5 },
  [54 /* g7 */] = { 0xc0810040d840820, 5 },
  [55 /* h7 */] = { 0x9100400404000, 5 },
  [56 /* a8 */] = { 0x1010a10808010808, 6 },
  [57 /* b8 */] = { 0x384a084208050880, 5 },
  [58 /* c8 */] = { 0x2800100120841040, 5 },
  [59 /* d8 */] = { 0x8a0101420200, 5 },
  [60 /* e8 */] = { 0x80090240104104, 5 },
  [61 /* f8 */] = { 0x204010010640, 5 },
  [62 /* g8 */] = { 0x818860b42c0050, 5 },
  [63 /* h8 */] = { 0xc20820400408200, 6 }, };

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

#pragma endregion

typedef struct uo_magic
{
  uo_bitboard *moves;
  uo_bitboard mask;
  uint64_t number;
  uint8_t shift;
} uo_magic;

static uo_bitboard uo_moves_K[64];
static uo_bitboard uo_moves_N[64];

static uo_magic uo_magics_P[2][64];
static uo_magic uo_magics_B[64];
static uo_magic uo_magics_R[64];

static bool init;
static int rand_seed;



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

bool uo_bitboard_next_square(uo_bitboard bitboard, uo_square *square /* -1 for first */)
{
  uo_square s = *square;
  if (s == 63) return false;
  bitboard >>= s + 1;
  uint8_t next = uo_ffs(bitboard);
  if (next == 0) return false;
  *square = s + next;
  return true;
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

static inline uo_bitboard uo_bitboard_gen_mask_P(uo_square square, uo_piece color)
{
  int rank = uo_square_rank(square);
  int file = uo_square_file(square);
  uo_bitboard mask = uo_square_bitboard(square) & ~uo_bitboard_rank[0] & ~uo_bitboard_rank[7];
  uo_bitboard mask_not_file_a = mask & ~uo_bitboard_file[0];
  uo_bitboard mask_not_file_h = mask & ~uo_bitboard_file[7];

  uo_bitboard push_one;
  uo_bitboard push_two;
  uo_bitboard capture_left;
  uo_bitboard capture_right;

  if (color == uo_piece__white)
  {
    uo_bitboard mask_rank_2nd = mask & uo_bitboard_rank[1];;
    push_one = mask << 8;
    push_two = mask_rank_2nd << 16;
    capture_left = mask_not_file_a << 7;
    capture_right = mask_not_file_h << 9;
  }
  else
  {
    uo_bitboard mask_rank_7th = mask & uo_bitboard_rank[6];;
    push_one = mask >> 8;
    push_two = mask_rank_7th >> 16;
    capture_left = mask_not_file_a >> 9;
    capture_right = mask_not_file_h >> 7;
  }

  mask = push_one | push_two | capture_left | capture_right;

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

static inline uo_bitboard uo_bitboard_gen_moves_P(uo_bitboard blockers, uo_square square, uo_piece color)
{
  int rank = uo_square_rank(square);
  int file = uo_square_file(square);
  uo_bitboard mask = uo_square_bitboard(square);
  uo_bitboard moves = 0;

  if (color == uo_piece__white)
  {
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
  }
  else {
    // push one
    if (rank > 0 && rank < 7)
    {
      moves |= (mask >> 8) & ~blockers;

      // push two
      if (moves && rank == 6)
      {
        moves |= (mask >> 16) & ~blockers;
      }

      // capture left
      if (file > 0)
      {
        moves |= (mask >> 9) & blockers;
      }

      // capture right
      if (file < 7)
      {
        moves |= (mask >> 7) & blockers;
      }
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

static inline void uo_magic_init(uo_magic *magic, uo_bitboard *blockers_moves /* alternating list of blockers and moves*/, uo_square square, const uint64_t known_magic[2])
{
  if (known_magic && known_magic[0])
  {
    magic->number = known_magic[0];
    magic->shift = 64 - known_magic[1];
  }

  uint8_t shift = magic->shift;
  uint8_t bits = 64 - shift;
  uint64_t number = magic->number ? magic->number : (uo_rand() & uo_rand() & uo_rand());
  uint16_t count = 1 << bits;

  while (true)
  {
    memset(magic->moves, 0, count * sizeof * magic->moves);

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
        number = uo_rand() & uo_rand() & uo_rand();

        goto try_next_number;
      }
    }

    if (!known_magic || !known_magic[0])
    {
      // suitable magic number was found
      printf("suitable magic number was found\n");
      printf("square: %d: { 0x%" PRIx64 ", %d },\n", square, number, bits);
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
    // white
    {
      uo_bitboard mask = uo_bitboard_gen_mask_P(square, uo_piece__white);

      uo_magics_P[uo_piece__white][square].mask = mask;
      int bits = uo_popcount(mask);
      uo_magics_P[uo_piece__white][square].shift = 64 - bits;
      int blocker_board_count = 1 << bits;
      blocker_board_count_total += blocker_board_count;
      blocker_board_count_max = blocker_board_count > blocker_board_count_max ? blocker_board_count : blocker_board_count_max;

      // printf("%c%d - %d\n", 'a' + uo_square_file(square), 1 + uo_square_rank(i), i);
      // uo_bitboard_print(mask);
      // printf("\n");
    }

    // black
    {
      uo_bitboard mask = uo_bitboard_gen_mask_P(square, uo_piece__black);

      uo_magics_P[uo_piece__black][square].mask = mask;
      int bits = uo_popcount(mask);
      uo_magics_P[uo_piece__black][square].shift = 64 - bits;
      int blocker_board_count = 1 << bits;
      blocker_board_count_total += blocker_board_count;
      blocker_board_count_max = blocker_board_count > blocker_board_count_max ? blocker_board_count : blocker_board_count_max;

      // printf("%c%d - %d\n", 'a' + uo_square_file(square), 1 + uo_square_rank(i), i);
      // uo_bitboard_print(mask);
      // printf("\n");
    }
  }

  // 2. Allocate memory for blocker and move boards (and some extra space for temporary needs)
  uo_bitboard *boards = malloc((blocker_board_count_total + blocker_board_count_max * 2) * sizeof boards);

  // 3. Generate blocker boards and move boards
  for (uo_square square = 0; square < 64; ++square)
  {
    // white
    {
      uo_magics_P[uo_piece__white][square].moves = boards;
      uo_magic magic = uo_magics_P[uo_piece__white][square];
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
        uo_bitboard moves = uo_bitboard_gen_moves_P(blockers, square, uo_piece__white);

        *blockers_moves++ = blockers;
        *blockers_moves++ = moves;

        // printf("permutation: %d, \n", permutation);
        // printf("blockers\n");
        // uo_bitboard_print(blockers);
        // printf("moves\n");
        // uo_bitboard_print(moves);
        // printf("\n");
      }

      uo_magic_init(&uo_magics_P[uo_piece__white][square], boards, square, UO_REGENERATE_MAGICS ? NULL : known_magics_P[uo_piece__white][square]);
    }

    // black
    {
      uo_magics_P[uo_piece__black][square].moves = boards;
      uo_magic magic = uo_magics_P[uo_piece__black][square];
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
        uo_bitboard moves = uo_bitboard_gen_moves_P(blockers, square, uo_piece__black);

        *blockers_moves++ = blockers;
        *blockers_moves++ = moves;

        // printf("permutation: %d, \n", permutation);
        // printf("blockers\n");
        // uo_bitboard_print(blockers);
        // printf("moves\n");
        // uo_bitboard_print(moves);
        // printf("\n");
      }

      uo_magic_init(&uo_magics_P[uo_piece__black][square], boards, square, UO_REGENERATE_MAGICS ? NULL : known_magics_P[uo_piece__black][square]);
    }
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
  uo_bitboard *boards = malloc((blocker_board_count_total + blocker_board_count_max * 2) * sizeof boards);

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
    uo_bitboard *blockers_moves = boards;

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

  rand_seed = time(NULL);
  uo_rand_init(rand_seed);

  uo_moves_K_init();
  uo_moves_N_init();
  uo_magics_B_init();
  uo_magics_R_init();
  uo_magics_P_init();
}

uo_bitboard uo_bitboard_moves(uo_square square, uo_piece piece, uo_bitboard blockers)
{
  uo_bitboard moves = 0;

  if (piece & uo_piece__P)
  {
    uo_magic magic = uo_magics_P[uo_piece_color(piece)][square];
    int index = ((blockers & magic.mask) * magic.number) >> magic.shift;
    moves |= magic.moves[index];
  }

  if (piece & uo_piece__B)
  {
    uo_magic magic = uo_magics_B[square];
    int index = ((blockers & magic.mask) * magic.number) >> magic.shift;
    moves |= magic.moves[index];
  }

  if (piece & uo_piece__R)
  {
    uo_magic magic = uo_magics_R[square];
    int index = ((blockers & magic.mask) * magic.number) >> magic.shift;
    moves |= magic.moves[index];
  }

  if (piece & uo_piece__N)
  {
    moves |= uo_moves_N[square];
  }

  if (piece & uo_piece__K)
  {
    moves |= uo_moves_K[square];
  }

  return moves;
}

uo_bitboard uo_bitboard_pins(uo_square square, uo_bitboard blockers, uo_bitboard diagonal_attackers, uo_bitboard line_attackers)
{
  const uint8_t file = uo_square_file(square);
  const uint8_t rank = uo_square_rank(square);

  uo_bitboard pins = 0;
  uo_square pin = 0;

  // diagonal pin below

  for (int i = square - 9; i >= 9; i -= 9)
  {
    if (uo_square_bitboard(i) & blockers)
    {
      pin = i;

      for (int j = i - 9; j >= 0; j -= 9)
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

  for (int i = square + 9; i < 55; i += 9)
  {
    if (uo_square_bitboard(i) & blockers)
    {
      pin = i;

      for (int j = i + 9; j < 64; j += 9)
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

  for (int i = square - 7; i >= 9; i -= 7)
  {
    if (uo_square_bitboard(i) & blockers)
    {
      pin = i;

      for (int j = i - 7; j >= 0; j -= 7)
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

  for (int i = square + 7; i < 55; i += 7)
  {
    if (uo_square_bitboard(i) & blockers)
    {
      pin = i;

      for (int j = i + 7; j < 64; j += 7)
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
        if (uo_square_bitboard(j) & diagonal_attackers)
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
        if (uo_square_bitboard(j) & diagonal_attackers)
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

  for (int i = square - 8; i > file; --i)
  {
    if (uo_square_bitboard(i) & blockers)
    {
      pin = i;

      for (int j = i - 8; j >= file; --j)
      {
        if (uo_square_bitboard(j) & diagonal_attackers)
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

  for (int i = square + 8; i < 56; ++i)
  {
    if (uo_square_bitboard(i) & blockers)
    {
      pin = i;

      for (int j = i + 8; j < 64; ++j)
      {
        if (uo_square_bitboard(j) & diagonal_attackers)
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
