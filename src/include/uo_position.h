#ifndef UO_POSITION_H
#define UO_POSITION_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_bitboard.h"
#include "uo_piece.h"
#include "uo_move.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

  typedef struct uo_position
  {
    uo_bitboard piece_color;

    uo_bitboard p;
    uo_bitboard n;
    uo_bitboard b;
    uo_bitboard r;
    uo_bitboard q;
    uo_bitboard k;

    uo_piece color_to_move;

    // K Q k q
    // 1 2 4 8
    uint8_t castling;

    uint8_t enpassant;
    uint8_t halfmoves;
    uint16_t fullmove;

  } uo_position;

  extern const size_t uo_position__piece_bitboard_offset[0x100];

#define uo_position_piece_bitboard(position, piece) (uo_bitboard *)((uint8_t *)(position) + uo_position__piece_bitboard_offset[piece])

  // see: https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation
  // example fen: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
  uo_position *uo_position_from_fen(uo_position *pos, char *fen);

  char *uo_position_to_diagram(uo_position *pos, char diagram[72]);

  static inline uo_piece uo_position_square_piece(uo_position *position, uo_square square)
  {
    uo_bitboard mask = uo_square_bitboard(square);
    uo_piece piece;

    if (mask & position->p) piece = uo_piece__P;
    else if (mask & position->n) piece = uo_piece__N;
    else if (mask & position->b) piece = uo_piece__B;
    else if (mask & position->r) piece = uo_piece__R;
    else if (mask & position->q) piece = uo_piece__Q;
    else if (mask & position->k) piece = uo_piece__K;
    else return 0;

    piece |= (mask & position->piece_color) ? uo_piece__black : uo_piece__white;
    return piece;
  }

  uo_move_ex uo_position_make_move(uo_position *position, uo_move move);
  void uo_position_unmake_move(uo_position *position, uo_move_ex move);

#ifdef __cplusplus
}
#endif

#endif
