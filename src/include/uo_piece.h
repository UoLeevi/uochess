#ifndef UO_PIECE_H
#define UO_PIECE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_def.h"

#include <stdint.h>

  typedef uint8_t uo_piece;

#define uo_piece__P ((uo_piece)0x2) // P 0b0010
#define uo_piece__N ((uo_piece)0x4) // N 0b0100
#define uo_piece__B ((uo_piece)0x6) // B 0b0110
#define uo_piece__R ((uo_piece)0x8) // R 0b1000
#define uo_piece__Q ((uo_piece)0xA) // Q 0b1010
#define uo_piece__K ((uo_piece)0xC) // K 0b1100
#define uo_piece__p ((uo_piece)0x3) // p 0b0011
#define uo_piece__n ((uo_piece)0x5) // n 0b0101
#define uo_piece__b ((uo_piece)0x7) // b 0b0111
#define uo_piece__r ((uo_piece)0x9) // r 0b1001
#define uo_piece__q ((uo_piece)0xB) // q 0b1011
#define uo_piece__k ((uo_piece)0xD) // k 0b1101

#define uo_piece_type(piece) ((piece) & (uo_piece)0xE)

  static inline char uo_piece_to_char(uo_piece piece)
  {
    switch (piece)
    {
      case uo_piece__P: return 'P';
      case uo_piece__N: return 'N';
      case uo_piece__B: return 'B';
      case uo_piece__R: return 'R';
      case uo_piece__Q: return 'Q';
      case uo_piece__K: return 'K';
      case uo_piece__p: return 'p';
      case uo_piece__n: return 'n';
      case uo_piece__b: return 'b';
      case uo_piece__r: return 'r';
      case uo_piece__q: return 'q';
      case uo_piece__k: return 'k';
      default: return 0;
    }
  };

  static inline uo_piece uo_piece_from_char(char chr)
  {
    switch (chr)
    {
      case 'P': return uo_piece__P;
      case 'N': return uo_piece__N;
      case 'B': return uo_piece__B;
      case 'R': return uo_piece__R;
      case 'Q': return uo_piece__Q;
      case 'K': return uo_piece__K;
      case 'p': return uo_piece__p;
      case 'n': return uo_piece__n;
      case 'b': return uo_piece__b;
      case 'r': return uo_piece__r;
      case 'q': return uo_piece__q;
      case 'k': return uo_piece__k;
      default: return 0;
    }
  };

  static inline int16_t up_piece_value(uo_piece piece)
  {
    switch (piece)
    {
      case uo_piece__P:
      case uo_piece__p:
        return 100;

      case uo_piece__n:
      case uo_piece__N:
        return 300;

      case uo_piece__B:
      case uo_piece__b:
        return 301;

      case uo_piece__R:
      case uo_piece__r:
        return 500;

      case uo_piece__Q:
      case uo_piece__q:
        return 1050;

      case uo_piece__K:
      case uo_piece__k:
        return UO_SCORE_MATE_IN_THRESHOLD;

      default: return 0;
    }
  };

#ifdef __cplusplus
}
#endif

#endif
