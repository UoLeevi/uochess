#ifndef UO_POSITION_H
#define UO_POSITION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "uo_bitboard.h"

#include <inttypes.h>
#include <stdbool.h>

typedef struct uo_position
{
  uo_bitboard black_piece;

  uo_bitboard p;
  uo_bitboard n;
  uo_bitboard b;
  uo_bitboard r;
  uo_bitboard q;
  uo_bitboard k;

  bool white_to_move;

  // K Q k q
  // 1 2 4 8
  uint8_t castling;

  uint8_t enpassant;
  uint8_t halfmoves;
  uint16_t fullmove;

} uo_position;

// see: https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation
// example fen: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
uo_position *uo_position_from_fen(uo_position *pos, char *fen);

char *uo_position_to_diagram(uo_position *pos, char diagram[72]);

#ifdef __cplusplus
}
#endif

#endif