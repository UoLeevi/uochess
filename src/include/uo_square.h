#ifndef UO_SQUARE_H
#define UO_SQUARE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <inttypes.h>

  typedef int8_t uo_square;
  typedef uint64_t uo_bitboard;

#define uo_square_bitboard(square) ((uo_bitboard)1 << (square))
#define uo_square_file(square) ((square)&7)
#define uo_square_rank(square) ((square) >> 3)
  extern const int uo_square_diagonal[64];
  extern const int uo_square_antidiagonal[64];


#define uo_square__a1  0
#define uo_square__b1  1
#define uo_square__c1  2
#define uo_square__d1  3
#define uo_square__e1  4
#define uo_square__f1  5
#define uo_square__g1  6
#define uo_square__h1  7

#define uo_square__a2  8
#define uo_square__b2  9
#define uo_square__c2 10
#define uo_square__d2 11
#define uo_square__e2 12
#define uo_square__f2 13
#define uo_square__g2 14
#define uo_square__h2 15

#define uo_square__a3 16
#define uo_square__b3 17
#define uo_square__c3 18
#define uo_square__d3 19
#define uo_square__e3 20
#define uo_square__f3 21
#define uo_square__g3 22
#define uo_square__h3 23

#define uo_square__a4 24
#define uo_square__b4 25
#define uo_square__c4 26
#define uo_square__d4 27
#define uo_square__e4 28
#define uo_square__f4 29
#define uo_square__g4 30
#define uo_square__h4 31

#define uo_square__a5 32
#define uo_square__b5 33
#define uo_square__c5 34
#define uo_square__d5 35
#define uo_square__e5 36
#define uo_square__f5 37
#define uo_square__g5 38
#define uo_square__h5 39

#define uo_square__a6 40
#define uo_square__b6 41
#define uo_square__c6 42
#define uo_square__d6 43
#define uo_square__e6 44
#define uo_square__f6 45
#define uo_square__g6 46
#define uo_square__h6 47

#define uo_square__a7 48
#define uo_square__b7 49
#define uo_square__c7 50
#define uo_square__d7 51
#define uo_square__e7 52
#define uo_square__f7 53
#define uo_square__g7 54
#define uo_square__h7 55

#define uo_square__a8 56
#define uo_square__b8 57
#define uo_square__c8 58
#define uo_square__d8 59
#define uo_square__e8 60
#define uo_square__f8 61
#define uo_square__g8 62
#define uo_square__h8 63

  size_t uo_squares_between(uo_square from, uo_square to, uo_square between[6]);

#ifdef __cplusplus
}
#endif

#endif
