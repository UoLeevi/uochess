#ifndef UO_MOVE_H
#define UO_MOVE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_square.h"
#include "uo_piece.h"
#include "uo_position.h"

#include <inttypes.h>

  // see: https://www.chessprogramming.org/Encoding_Moves#From-To_Based
  typedef uint16_t uo_move;

  typedef int8_t uo_move_type;
#define uo_move_type__illegal ((uo_move_type)-1)
#define uo_move_type__quiet ((uo_move_type)0)
#define uo_move_type__P_double_push ((uo_move_type)1)
#define uo_move_type__OO ((uo_move_type)2)
#define uo_move_type__OOO ((uo_move_type)3)
#define uo_move_type__x ((uo_move_type)4)
#define uo_move_type__enpassant ((uo_move_type)5)
#define uo_move_type__promo ((uo_move_type)8)
#define uo_move_type__promo_N ((uo_move_type)8)
#define uo_move_type__promo_B ((uo_move_type)9)
#define uo_move_type__promo_R ((uo_move_type)10)
#define uo_move_type__promo_Q ((uo_move_type)11)
#define uo_move_type__promo_Nx ((uo_move_type)12)
#define uo_move_type__promo_Bx ((uo_move_type)13)
#define uo_move_type__promo_Rx ((uo_move_type)14)
#define uo_move_type__promo_Qx ((uo_move_type)15)

  static inline uo_square uo_move_square_from(uo_move move)
  {
    return move & 0x3F;
  }

  static inline uo_square uo_move_square_to(uo_move move)
  {
    return uo_move_square_from(move >> 6);
  }

  static inline uo_move_type uo_move_get_type(uo_move move)
  {
    return uo_move_square_from(move >> 6);
  }

  static inline uo_move uo_move_encode(uo_square from, uo_square to, uo_move_type type)
  {
    return from | (to << 6) | (type << 12);
  }

  // see: https://www.chessprogramming.org/Encoding_Moves#Extended_Move_Structure
  typedef uint64_t uo_move_ex;

                                     // bits:        16                      16              8
  static inline uo_move_ex uo_move_ex_encode(uo_move move, uo_position_flags flags, uo_piece captured)
  {
    return move | (flags << 16) | (captured << 32);
  }

  static inline uo_position_flags uo_move_ex_flags(uo_move move)
  {
    return (move >> 16) && 0xFFFF;
  }

  static inline uo_piece uo_move_ex_piece_captured(uo_move_ex move)
  {
    return (move >> 32) && 0xFF;
  }

#ifdef __cplusplus
}
#endif

#endif