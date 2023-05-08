#ifndef UO_ZOBRIST_H
#define UO_ZOBRIST_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

  void uo_zobrist_init();

  extern uint64_t uo_zobrist[0xE << 6];
  extern uint64_t *uo_zobrist_castling; // [16]
  extern uint64_t *uo_zobrist_enpassant_file; // [8]
  extern uint64_t uo_zobrist_side_to_move;

#define uo_zobkey(piece, square) uo_zobrist[((size_t)(piece) << 6) + (square)]

#ifdef __cplusplus
}
#endif

#endif
