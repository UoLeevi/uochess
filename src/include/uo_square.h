#ifndef UO_SQUARE_H
#define UO_SQUARE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <inttypes.h>

  typedef uint8_t uo_square;
  typedef uint64_t uo_bitboard;

#define uo_square_bitboard(square) ((uo_bitboard)1 << (square))
#define uo_square_file(square) ((square)&7)
#define uo_square_rank(square) ((square) >> 3)
  extern const int uo_square_diagonal[64];
  extern const int uo_square_antidiagonal[64];

  enum uo_square_enum {
    uo_square__a1, uo_square__b1, uo_square__c1, uo_square__d1, uo_square__e1, uo_square__f1, uo_square__g1, uo_square__h1,
    uo_square__a2, uo_square__b2, uo_square__c2, uo_square__d2, uo_square__e2, uo_square__f2, uo_square__g2, uo_square__h2,
    uo_square__a3, uo_square__b3, uo_square__c3, uo_square__d3, uo_square__e3, uo_square__f3, uo_square__g3, uo_square__h3,
    uo_square__a4, uo_square__b4, uo_square__c4, uo_square__d4, uo_square__e4, uo_square__f4, uo_square__g4, uo_square__h4,
    uo_square__a5, uo_square__b5, uo_square__c5, uo_square__d5, uo_square__e5, uo_square__f5, uo_square__g5, uo_square__h5,
    uo_square__a6, uo_square__b6, uo_square__c6, uo_square__d6, uo_square__e6, uo_square__f6, uo_square__g6, uo_square__h6,
    uo_square__a7, uo_square__b7, uo_square__c7, uo_square__d7, uo_square__e7, uo_square__f7, uo_square__g7, uo_square__h7,
    uo_square__a8, uo_square__b8, uo_square__c8, uo_square__d8, uo_square__e8, uo_square__f8, uo_square__g8, uo_square__h8
  };

#ifdef __cplusplus
}
#endif

#endif
