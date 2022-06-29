#ifndef UO_POSITION_H
#define UO_POSITION_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_bitboard.h"
#include "uo_piece.h"
#include "uo_move.h"
#include "uo_def.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

  typedef uint16_t uo_position_flags;
  /*
    color_to_move : 1
      0 w
      1 b
    halfmoves: 7
      1-100
    enpassant_file : 4
      1-8
    castling: 4
      1 K
      2 Q
      4 k
      8 q
  */

  typedef struct uo_position_state
  {
    uo_position_flags flags;
    uo_move move;
    uo_piece piece_captured;
  } uo_position_state;

  typedef struct uo_position
  {
    uo_bitboard piece_color;

    uo_bitboard P;
    uo_bitboard N;
    uo_bitboard B;
    uo_bitboard R;
    uo_bitboard Q;
    uo_bitboard K;

    uo_piece board[64];

    uo_position_flags flags;
    uint16_t fullmove;

    uo_square checkers[2];

    uint64_t key;

    uo_position_state state[UO_MAX_PLY];
    uo_position_state *stack;
  } uo_position;

#pragma region uo_position_flags__functions

  static inline uint8_t uo_position_flags_color_to_move(uo_position_flags flags)
  {
    return flags & 0x0001;
  }

  static inline uint8_t uo_position_flags_halfmoves(uo_position_flags flags)
  {
    return (flags & 0x00FE) >> 1;
  }

  static inline uint8_t uo_position_flags_enpassant_file(uo_position_flags flags)
  {
    return (flags & 0x0F00) >> 8;
  }

  static inline uint8_t uo_position_flags_castling(uo_position_flags flags)
  {
    return (flags & 0xF000) >> 12;
  }

  static inline uint8_t uo_position_flags_castling_white(uo_position_flags flags)
  {
    return (flags & 0x3000) >> 12;
  }

  static inline uint8_t uo_position_flags_castling_black(uo_position_flags flags)
  {
    return (flags & 0xC000) >> 14;
  }

  static inline bool uo_position_flags_castling_K(uo_position_flags flags)
  {
    return (flags & 0x1000) >> 12;
  }

  static inline bool uo_position_flags_castling_Q(uo_position_flags flags)
  {
    return (flags & 0x2000) >> 13;
  }

  static inline bool uo_position_flags_castling_k(uo_position_flags flags)
  {
    return (flags & 0x4000) >> 14;
  }

  static inline bool uo_position_flags_castling_q(uo_position_flags flags)
  {
    return (flags & 0x8000) >> 15;
  }

  static inline uo_position_flags uo_position_flags_update_color_to_move(uo_position_flags flags, uint8_t color_to_move)
  {
    return (flags & 0xFFFE) | (uo_position_flags)color_to_move;
  }

  static inline uo_position_flags uo_position_flags_update_halfmoves(uo_position_flags flags, uint8_t halfmoves)
  {
    return (flags & 0xFF01) | ((uo_position_flags)halfmoves << 1);
  }

  static inline uo_position_flags uo_position_flags_update_enpassant_file(uo_position_flags flags, uint8_t enpassant_file)
  {
    return (flags & 0xF0FF) | ((uo_position_flags)enpassant_file << 8);
  }

  static inline uo_position_flags uo_position_flags_update_castling(uo_position_flags flags, uint8_t castling)
  {
    return (flags & 0x0FFF) | ((uo_position_flags)castling << 12);
  }

  static inline uo_position_flags uo_position_flags_update_castling_white(uo_position_flags flags, uint8_t castling_white)
  {
    return (flags & 0xCFFF) | ((uo_position_flags)castling_white << 12);
  }

  static inline uo_position_flags uo_position_flags_update_castling_black(uo_position_flags flags, uint8_t castling_black)
  {
    return (flags & 0x3FFF) | ((uo_position_flags)castling_black << 14);
  }

  static inline uo_position_flags uo_position_flags_update_castling_K(uo_position_flags flags, bool castling_K)
  {
    return (flags & 0xEFFF) | ((uo_position_flags)castling_K << 12);
  }

  static inline uo_position_flags uo_position_flags_update_castling_Q(uo_position_flags flags, bool castling_Q)
  {
    return (flags & 0xDFFF) | ((uo_position_flags)castling_Q << 13);
  }

  static inline uo_position_flags uo_position_flags_update_castling_k(uo_position_flags flags, bool castling_k)
  {
    return (flags & 0xBFFF) | ((uo_position_flags)castling_k << 14);
  }

  static inline uo_position_flags uo_position_flags_update_castling_q(uo_position_flags flags, bool castling_q)
  {
    return (flags & 0x7FFF) | ((uo_position_flags)castling_q << 15);
  }

#pragma endregion

  // see: https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation
  // example fen: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
  uo_position *uo_position_from_fen(uo_position *position, char *fen);

  size_t uo_position_print_fen(uo_position *position, char fen[90]);

  size_t uo_position_print_diagram(uo_position *position, char diagram[663]);

  void uo_position_make_move(uo_position *position, uo_move move);

  void uo_position_unmake_move(uo_position *position);

  size_t uo_position_get_moves(uo_position *position, uo_move *movelist);

  bool uo_position_is_check(uo_position *position);

  bool uo_position_is_legal_move(uo_position *position, uo_move move);

  uo_move uo_position_parse_move(uo_position *position, char str[5]);

#ifdef __cplusplus
}
#endif

#endif
