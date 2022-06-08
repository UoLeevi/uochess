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
    a1, b1, c1, d1, e1, f1, g1, h1,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8
  };

#ifdef __cplusplus
}
#endif

#endif