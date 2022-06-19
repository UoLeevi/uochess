#ifndef UO_PIECE_H
#define UO_PIECE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <inttypes.h>

  typedef uint8_t uo_piece;

  //                                               0b00000000
  //                                                    QQ
  //                                                   KRBNP
  extern const uo_piece uo_piece__P;   // = 0x02 - 0b00000010
  extern const uo_piece uo_piece__N;   // = 0x04 - 0b00000100
  extern const uo_piece uo_piece__B;   // = 0x08 - 0b00001000
  extern const uo_piece uo_piece__R;   // = 0x10 - 0b00010000
  extern const uo_piece uo_piece__Q;   // = 0x18 - 0b00011000
  extern const uo_piece uo_piece__K;   // = 0x20 - 0b00100000
  extern const uo_piece uo_piece__p;   // = 0x03 - 0b00000011
  extern const uo_piece uo_piece__n;   // = 0x05 - 0b00000101
  extern const uo_piece uo_piece__b;   // = 0x09 - 0b00001001
  extern const uo_piece uo_piece__r;   // = 0x11 - 0b00010001
  extern const uo_piece uo_piece__q;   // = 0x19 - 0b00011001
  extern const uo_piece uo_piece__k;   // = 0x21 - 0b00100001

  extern const uo_piece uo_piece__any; // = 0x3E - 0b00111110

  extern const uo_piece uo_piece__white; // = 0x00;
  extern const uo_piece uo_piece__black; // = 0x01;

  extern const char uo_piece_to_char[0x100];
  extern const uo_piece uo_piece_from_char[0x100];

#define uo_piece_color(piece) ((piece) & (uo_piece)0x1)

#ifdef __cplusplus
}
#endif

#endif
