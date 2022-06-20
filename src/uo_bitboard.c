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
    [0 /* a1 */] = { 0x1, 1 },
    [1 /* b1 */] = { 0x429000004400000, 2 },
    [2 /* c1 */] = { 0x830000000028000, 2 },
    [3 /* d1 */] = { 0x980100130c0800, 2 },
    [4 /* e1 */] = { 0x405002080814501, 2 },
    [5 /* f1 */] = { 0x62840000000020, 2 },
    [6 /* g1 */] = { 0xacd8e010080006, 2 },
    [7 /* h1 */] = { 0x200000000000002, 1 },
    [8 /* a2 */] = { 0x6402002200004, 3 },
    [9 /* b2 */] = { 0xa00104400004030, 4 },
    [10 /* c2 */] = { 0xac00100404004404, 4 },
    [11 /* d2 */] = { 0x334220811604804, 4 },
    [12 /* e2 */] = { 0x8041041100008820, 4 },
    [13 /* f2 */] = { 0x1800060c8c000012, 4 },
    [14 /* g2 */] = { 0x911408002c1, 4 },
    [15 /* h2 */] = { 0x480c008500000401, 3 },
    [16 /* a3 */] = { 0xd00004489804008, 2 },
    [17 /* b3 */] = { 0x9080002202623021, 3 },
    [18 /* c3 */] = { 0x10003000220048, 3 },
    [19 /* d3 */] = { 0x20001900020a80, 3 },
    [20 /* e3 */] = { 0x186820040229108c, 3 },
    [21 /* f3 */] = { 0x80202900040080, 3 },
    [22 /* g3 */] = { 0x180100841000, 3 },
    [23 /* h3 */] = { 0x980800108000241, 2 },
    [24 /* a4 */] = { 0x202000004c609400, 2 },
    [25 /* b4 */] = { 0x200002c90c00420, 3 },
    [26 /* c4 */] = { 0x1b0210400080, 3 },
    [27 /* d4 */] = { 0x410009008000040, 3 },
    [28 /* e4 */] = { 0x402180012000802, 3 },
    [29 /* f4 */] = { 0x1402082210, 3 },
    [30 /* g4 */] = { 0x82200004010a0000, 3 },
    [31 /* h4 */] = { 0x14000582802000, 2 },
    [32 /* a5 */] = { 0x802002400000, 2 },
    [33 /* b5 */] = { 0x5000000809221000, 3 },
    [34 /* c5 */] = { 0x1200800040100100, 3 },
    [35 /* d5 */] = { 0x408040000080848, 3 },
    [36 /* e5 */] = { 0x1400108810090850, 3 },
    [37 /* f5 */] = { 0x4201800000048400, 3 },
    [38 /* g5 */] = { 0x1000192031, 3 },
    [39 /* h5 */] = { 0x1000082480010100, 2 },
    [40 /* a6 */] = { 0x64408208c00a100, 2 },
    [41 /* b6 */] = { 0x4000008900406480, 3 },
    [42 /* c6 */] = { 0x4001000002001140, 3 },
    [43 /* d6 */] = { 0x2030a9008202802, 3 },
    [44 /* e6 */] = { 0x10008015004090c, 3 },
    [45 /* f6 */] = { 0x4480040020018200, 3 },
    [46 /* g6 */] = { 0x1402000020001106, 3 },
    [47 /* h6 */] = { 0x200401000100303, 2 },
    [48 /* a7 */] = { 0x1840108020c140, 2 },
    [49 /* b7 */] = { 0x40008021200290, 3 },
    [50 /* c7 */] = { 0x800000100002010, 3 },
    [51 /* d7 */] = { 0x900008008029, 3 },
    [52 /* e7 */] = { 0x2000400080050004, 3 },
    [53 /* f7 */] = { 0x30408000000c002, 3 },
    [54 /* g7 */] = { 0x200c20084000015, 3 },
    [55 /* h7 */] = { 0x1000000600401081, 2 },
    [56 /* a8 */] = { 0x150c200213081, 0 },
    [57 /* b8 */] = { 0x20800101000000, 0 },
    [58 /* c8 */] = { 0x100000002082001, 0 },
    [59 /* d8 */] = { 0x2100142400008400, 0 },
    [60 /* e8 */] = { 0x28000d001302020, 0 },
    [61 /* f8 */] = { 0x140842a00040032, 0 },
    [62 /* g8 */] = { 0x806080000201, 0 },
    [63 /* h8 */] = { 0x18020002c01000, 0 }
    },
  [1 /* black */] = {
    // square      number               bits
    [0 /* a1 */] = { 0x404010000408, 0 },
    [1 /* b1 */] = { 0x2200080000201220, 0 },
    [2 /* c1 */] = { 0x2000010010000001, 0 },
    [3 /* d1 */] = { 0x8400000040000200, 0 },
    [4 /* e1 */] = { 0x144082d880001108, 0 },
    [5 /* f1 */] = { 0x1800500030004801, 0 },
    [6 /* g1 */] = { 0x9400030004000208, 0 },
    [7 /* h1 */] = { 0x4004080000801, 0 },
    [8 /* a2 */] = { 0x4000002180104018, 2 },
    [9 /* b2 */] = { 0x2082000008060822, 3 },
    [10 /* c2 */] = { 0x100010026800a000, 3 },
    [11 /* d2 */] = { 0x2800801004022200, 3 },
    [12 /* e2 */] = { 0x1400810120010100, 3 },
    [13 /* f2 */] = { 0x2206201200800100, 3 },
    [14 /* g2 */] = { 0x100c70200008012, 3 },
    [15 /* h2 */] = { 0x5040113000000c1, 2 },
    [16 /* a3 */] = { 0x2840001080a08001, 2 },
    [17 /* b3 */] = { 0x92012000205843, 3 },
    [18 /* c3 */] = { 0xcc10008800002488, 3 },
    [19 /* d3 */] = { 0x9001404000002, 3 },
    [20 /* e3 */] = { 0x2004000620800000, 3 },
    [21 /* f3 */] = { 0x2020000048100, 3 },
    [22 /* g3 */] = { 0x8005000480000000, 3 },
    [23 /* h3 */] = { 0x9902000a000012, 2 },
    [24 /* a4 */] = { 0x10a83012011804, 2 },
    [25 /* b4 */] = { 0x1202502100203, 3 },
    [26 /* c4 */] = { 0x2048240004000000, 3 },
    [27 /* d4 */] = { 0x240120548000180, 3 },
    [28 /* e4 */] = { 0x1000044080016204, 3 },
    [29 /* f4 */] = { 0x8090803000006, 3 },
    [30 /* g4 */] = { 0x10400080004, 3 },
    [31 /* h4 */] = { 0x10404660202, 2 },
    [32 /* a5 */] = { 0x2000004400060140, 2 },
    [33 /* b5 */] = { 0x14000e081020000, 3 },
    [34 /* c5 */] = { 0x204003001600110, 3 },
    [35 /* d5 */] = { 0x262008800080000, 3 },
    [36 /* e5 */] = { 0x200000140004281c, 3 },
    [37 /* f5 */] = { 0x200141205812e00, 3 },
    [38 /* g5 */] = { 0x101000101102400, 3 },
    [39 /* h5 */] = { 0x100a1210000a100, 2 },
    [40 /* a6 */] = { 0x4200108a0850000, 2 },
    [41 /* b6 */] = { 0x800141122000000, 3 },
    [42 /* c6 */] = { 0x800a0010000000, 3 },
    [43 /* d6 */] = { 0x228840909048000, 3 },
    [44 /* e6 */] = { 0x4000800024080480, 3 },
    [45 /* f6 */] = { 0x80d00022200090, 3 },
    [46 /* g6 */] = { 0x10004001000100, 3 },
    [47 /* h6 */] = { 0x8800028301002000, 2 },
    [48 /* a7 */] = { 0x801020083200080, 3 },
    [49 /* b7 */] = { 0x100010008210200, 4 },
    [50 /* c7 */] = { 0x100018420080045, 4 },
    [51 /* d7 */] = { 0x280041900c0020, 4 },
    [52 /* e7 */] = { 0x2101200208220c04, 4 },
    [53 /* f7 */] = { 0x400004010010, 4 },
    [54 /* g7 */] = { 0x42060ca032280, 4 },
    [55 /* h7 */] = { 0x1200010401028040, 3 },
    [56 /* a8 */] = { 0x400000000000104, 1 },
    [57 /* b8 */] = { 0x20400008a2800, 2 },
    [58 /* c8 */] = { 0xc909002843200, 2 },
    [59 /* d8 */] = { 0x220002602, 2 },
    [60 /* e8 */] = { 0x8900000018540, 2 },
    [61 /* f8 */] = { 0x440000000000931, 2 },
    [62 /* g8 */] = { 0x800001004020340, 2 },
    [63 /* h8 */] = { 0x21203000004804, 1 }
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
static uo_bitboard uo_attacks_P[2][64];

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

      uo_attacks_P[uo_piece__white][square] |= mask << 9;
      uo_attacks_P[uo_piece__black][square] |= mask >> 7;
    }

    for (uint8_t file = 1; file < 8; ++file)
    {
      uo_square square = (rank << 3) + file;
      uo_bitboard mask = uo_square_bitboard(square);

      uo_attacks_P[uo_piece__white][square] |= mask << 7;
      uo_attacks_P[uo_piece__black][square] |= mask >> 9;
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

static inline uo_bitboard uo_bitboard_gen_mask_P(uo_square square, uo_piece color)
{
  int rank = uo_square_rank(square);
  int file = uo_square_file(square);
  uo_bitboard mask = uo_square_bitboard(square) & ~uo_bitboard_rank[0] & ~uo_bitboard_rank[7];

  uo_bitboard push_one;
  uo_bitboard push_two;

  if (color == uo_piece__white)
  {
    uo_bitboard mask_rank_2nd = mask & uo_bitboard_rank[1];;
    push_one = mask << 8;
    push_two = mask_rank_2nd << 16;
  }
  else
  {
    uo_bitboard mask_rank_7th = mask & uo_bitboard_rank[6];;
    push_one = mask >> 8;
    push_two = mask_rank_7th >> 16;
  }

  return push_one | push_two | uo_attacks_P[color][square];
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

      //printf("%c%d - %d\n", 'a' + uo_square_file(square), 1 + uo_square_rank(square), square);
      //uo_bitboard_print(mask);
      //printf("\n");
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
  uo_bitboard *boards = malloc((blocker_board_count_total + blocker_board_count_max * 2) * sizeof * boards);

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
  uo_bitboard *boards = malloc((blocker_board_count_total + blocker_board_count_max * 2) * sizeof * boards);

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
  uo_attacks_P_init();
  uo_magics_B_init();
  uo_magics_R_init();
  uo_magics_P_init();
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
  uo_square shift_below = 63 - square;
  uint8_t diagonal = uo_square_diagonal[square];
  uint8_t antidiagonal = uo_square_antidiagonal[square];

  uo_bitboard mask = uo_square_bitboard(square);
  diagonal_attackers &= ~mask;
  blockers &= ~mask;

  uo_bitboard pins = 0;

  uo_bitboard antidiagonal_attackers = diagonal_attackers & uo_bitboard_antidiagonal[antidiagonal];

  if (antidiagonal_attackers)
  {
    uo_bitboard antidiagonal_blockers = blockers & uo_bitboard_antidiagonal[antidiagonal];

    uo_bitboard antidiagonal_attackers_below = (antidiagonal_attackers << shift_below) >> shift_below;
    uo_square square_antidiagonal_attacker_below = uo_msb(antidiagonal_attackers_below);

    if (square_antidiagonal_attacker_below != -1)
    {
      uo_bitboard antidiagonal_blockers_below = (antidiagonal_blockers << shift_below) >> shift_below;
      antidiagonal_blockers_below = (antidiagonal_blockers_below >> square_antidiagonal_attacker_below) << square_antidiagonal_attacker_below;
      if (uo_popcount(antidiagonal_blockers_below) == 2) pins |= antidiagonal_blockers_below & ~uo_square_bitboard(square_antidiagonal_attacker_below);
    }

    uo_bitboard antidiagonal_attackers_above = (antidiagonal_attackers >> square) << square;
    uo_square square_antidiagonal_attacker_above = uo_lsb(antidiagonal_attackers_above);

    if (square_antidiagonal_attacker_above != -1)
    {
      uo_bitboard antidiagonal_blockers_above = (antidiagonal_blockers >> square) << square;
      antidiagonal_blockers_above = (antidiagonal_blockers_above << (63 - square_antidiagonal_attacker_above)) >> (63 - square_antidiagonal_attacker_above);
      if (uo_popcount(antidiagonal_blockers_above) == 2) pins |= antidiagonal_blockers_above & ~uo_square_bitboard(square_antidiagonal_attacker_above);
    }
  }

  diagonal_attackers &= uo_bitboard_diagonal[diagonal];

  if (diagonal_attackers)
  {
    uo_bitboard diagonal_blockers = blockers & uo_bitboard_diagonal[diagonal];
    uo_bitboard diagonal_attackers_below = (diagonal_attackers << shift_below) >> shift_below;
    uo_square square_diagonal_attacker_below = uo_msb(diagonal_attackers_below);

    if (square_diagonal_attacker_below != -1)
    {
      uo_bitboard diagonal_blockers_below = (diagonal_blockers << shift_below) >> shift_below;
      diagonal_blockers_below = (diagonal_blockers_below >> square_diagonal_attacker_below) << square_diagonal_attacker_below;
      if (uo_popcount(diagonal_blockers_below) == 2) pins |= diagonal_blockers_below & ~uo_square_bitboard(square_diagonal_attacker_below);
    }

    uo_bitboard diagonal_attackers_above = (diagonal_attackers >> square) << square;
    uo_square square_diagonal_attacker_above = uo_lsb(diagonal_attackers_above);

    if (square_diagonal_attacker_above != -1)
    {
      uo_bitboard diagonal_blockers_above = (diagonal_blockers >> square) << square;
      diagonal_blockers_above = (diagonal_blockers_above << (63 - square_diagonal_attacker_above)) >> (63 - square_diagonal_attacker_above);
      if (uo_popcount(diagonal_blockers_above) == 2) pins |= diagonal_blockers_above & ~uo_square_bitboard(square_diagonal_attacker_above);
    }
  }

  return pins;
}

uo_bitboard uo_bitboard_pins_R(uo_square square, uo_bitboard blockers, uo_bitboard line_attackers)
{
  uo_square shift_below = 63 - square;
  uint8_t file = uo_square_file(square);
  uint8_t rank = uo_square_rank(square);

  uo_bitboard mask = uo_square_bitboard(square);
  line_attackers &= ~mask;
  blockers &= ~mask;

  uo_bitboard pins = 0;

  uo_bitboard rank_attackers = line_attackers & uo_bitboard_rank[rank];

  if (rank_attackers)
  {
    uo_bitboard rank_blockers = blockers & uo_bitboard_rank[rank];
    uo_bitboard rank_attackers_left = (rank_attackers << shift_below) >> shift_below;
    uo_square square_rank_attacker_left = uo_msb(rank_attackers_left);

    if (square_rank_attacker_left != -1)
    {
      uo_bitboard rank_blockers_left = (rank_blockers << shift_below) >> shift_below;
      rank_blockers_left = (rank_blockers_left >> square_rank_attacker_left) << square_rank_attacker_left;
      if (uo_popcount(rank_blockers_left) == 2) pins |= rank_blockers_left & ~uo_square_bitboard(square_rank_attacker_left);
    }

    uo_bitboard rank_attackers_right = (rank_attackers >> square) << square;
    uo_square square_rank_attacker_right = uo_lsb(rank_attackers_right);

    if (square_rank_attacker_right != -1)
    {
      uo_bitboard rank_blockers_right = (rank_blockers >> square) << square;
      rank_blockers_right = (rank_blockers_right << (63 - square_rank_attacker_right)) >> (63 - square_rank_attacker_right);
      if (uo_popcount(rank_blockers_right) == 2) pins |= rank_blockers_right & ~uo_square_bitboard(square_rank_attacker_right);
    }
  }

  uo_bitboard file_attackers = line_attackers & uo_bitboard_file[file];

  if (file_attackers)
  {
    uo_bitboard file_blockers = blockers & uo_bitboard_file[file];
    uo_bitboard file_attackers_below = (file_attackers << shift_below) >> shift_below;
    uo_square square_file_attacker_below = uo_msb(file_attackers_below);

    if (square_file_attacker_below != -1)
    {
      uo_bitboard file_blockers_below = (file_blockers << shift_below) >> shift_below;
      file_blockers_below = (file_blockers_below >> square_file_attacker_below) << square_file_attacker_below;
      if (uo_popcount(file_blockers_below) == 2) pins |= file_blockers_below & ~uo_square_bitboard(square_file_attacker_below);
    }

    uo_bitboard file_attackers_above = (file_attackers >> square) << square;
    uo_square square_file_attacker_above = uo_lsb(file_attackers_above);

    if (square_file_attacker_above != -1)
    {
      uo_bitboard file_blockers_above = (file_blockers >> square) << square;
      file_blockers_above = (file_blockers_above << (63 - square_file_attacker_above)) >> (63 - square_file_attacker_above);
      if (uo_popcount(file_blockers_above) == 2) pins |= file_blockers_above & ~uo_square_bitboard(square_file_attacker_above);
    }
  }

  return pins;
}

uo_bitboard uo_bitboard_moves_P(uo_square square, uint8_t color, uo_bitboard own, uo_bitboard enemy)
{
  uo_bitboard mask_file = uo_bitboard_file[uo_square_file(square)];
  uo_bitboard blockers_and_capture_targets = (enemy & ((mask_file << 1) | (mask_file >> 1))) | (mask_file & (own | enemy));
  uo_magic magic = uo_magics_P[color][square];
  int index = ((blockers_and_capture_targets & magic.mask) * magic.number) >> magic.shift;
  return magic.moves[index];
}

uo_bitboard uo_bitboard_attacks_N(uo_square square)
{
  return uo_moves_N[square];
}

uo_bitboard uo_bitboard_attacks_B(uo_square square, uo_bitboard blockers)
{
  uo_magic magic = uo_magics_B[square];
  int index = ((blockers & magic.mask) * magic.number) >> magic.shift;
  return magic.moves[index];
}

uo_bitboard uo_bitboard_attacks_R(uo_square square, uo_bitboard blockers)
{
  uo_magic magic = uo_magics_R[square];
  int index = ((blockers & magic.mask) * magic.number) >> magic.shift;
  return magic.moves[index];
}

uo_bitboard uo_bitboard_attacks_K(uo_square square)
{
  return uo_moves_K[square];
}

uo_bitboard uo_bitboard_attacks_P(uo_square square, uint8_t color)
{
  return uo_attacks_P[color][square];
}
