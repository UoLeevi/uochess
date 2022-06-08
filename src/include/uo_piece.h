#ifndef UO_PIECE_H
#define UO_PIECE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <inttypes.h>

  typedef uint8_t uo_piece;

  //                                               0b00000000
  //                                                 b PNBRQK
  extern const uo_piece uo_piece__P; // = 0xa0 - P 0b10100000
  extern const uo_piece uo_piece__N; // = 0x90 - N 0b10010000
  extern const uo_piece uo_piece__B; // = 0x88 - B 0b10001000
  extern const uo_piece uo_piece__R; // = 0x84 - R 0b10000100
  extern const uo_piece uo_piece__Q; // = 0x82 - Q 0b10000010
  extern const uo_piece uo_piece__K; // = 0x81 - K 0b10000001
  extern const uo_piece uo_piece__p; // = 0x20 - p 0b00100000
  extern const uo_piece uo_piece__n; // = 0x10 - n 0b00010000
  extern const uo_piece uo_piece__b; // = 0x08 - b 0b00001000
  extern const uo_piece uo_piece__r; // = 0x04 - r 0b00000100
  extern const uo_piece uo_piece__q; // = 0x02 - q 0b00000010
  extern const uo_piece uo_piece__k; // = 0x01 - k 0b00000001

  extern const char uo_piece_to_char[0x100];
  extern const uo_piece uo_piece_from_char[0x100];

#ifdef __cplusplus
}
#endif

#endif