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
#include <assert.h>

  typedef uint16_t uo_position_flags;
  /*
    color_to_move : 1
      0 w
      1 b
    rule50: 7
      1-100
    enpassant_file : 4
      1-8
    castling: 4
      1 K - h1
      2 Q - a1
      4 k - h8
      8 q - a8
  */

  typedef struct uo_position
  {
    uo_bitboard own;
    uo_bitboard enemy;
    uo_bitboard P;
    uo_bitboard p;
    uo_bitboard N;
    uo_bitboard n;
    uo_bitboard B;
    uo_bitboard b;
    uo_bitboard R;
    uo_bitboard r;
    uo_bitboard Q;
    uo_bitboard q;
    uo_bitboard K;
    uo_bitboard k;

    uo_piece board[64];
    //uint8_t board_xor;

    uo_position_flags flags;
    uint16_t ply;

    uo_square checkers[2];

    uint64_t key;

    struct
    {
      uo_piece *piece_captured;
      uo_move *move;
      uo_position_flags *flags;
      uo_piece captures[30];
      uo_move moves[UO_MAX_PLY];
      uo_position_flags prev_flags[UO_MAX_PLY];
    } history;
  } uo_position;

#pragma region uo_position_piece_bitboard

  //static_assert(offsetof(uo_position, white) / sizeof(uo_bitboard) == uo_white, "Unexpected field offset: uo_position.white");
  //static_assert(offsetof(uo_position, black) / sizeof(uo_bitboard) == uo_black, "Unexpected field offset: uo_position.black");
  static_assert(offsetof(uo_position, P) / sizeof(uo_bitboard) == uo_piece__P, "Unexpected field offset: uo_position.P");
  static_assert(offsetof(uo_position, p) / sizeof(uo_bitboard) == uo_piece__p, "Unexpected field offset: uo_position.p");
  static_assert(offsetof(uo_position, N) / sizeof(uo_bitboard) == uo_piece__N, "Unexpected field offset: uo_position.N");
  static_assert(offsetof(uo_position, n) / sizeof(uo_bitboard) == uo_piece__n, "Unexpected field offset: uo_position.n");
  static_assert(offsetof(uo_position, B) / sizeof(uo_bitboard) == uo_piece__B, "Unexpected field offset: uo_position.B");
  static_assert(offsetof(uo_position, b) / sizeof(uo_bitboard) == uo_piece__b, "Unexpected field offset: uo_position.b");
  static_assert(offsetof(uo_position, R) / sizeof(uo_bitboard) == uo_piece__R, "Unexpected field offset: uo_position.R");
  static_assert(offsetof(uo_position, r) / sizeof(uo_bitboard) == uo_piece__r, "Unexpected field offset: uo_position.r");
  static_assert(offsetof(uo_position, Q) / sizeof(uo_bitboard) == uo_piece__Q, "Unexpected field offset: uo_position.Q");
  static_assert(offsetof(uo_position, q) / sizeof(uo_bitboard) == uo_piece__q, "Unexpected field offset: uo_position.q");
  static_assert(offsetof(uo_position, K) / sizeof(uo_bitboard) == uo_piece__K, "Unexpected field offset: uo_position.K");
  static_assert(offsetof(uo_position, k) / sizeof(uo_bitboard) == uo_piece__k, "Unexpected field offset: uo_position.k");

  static inline uo_bitboard *uo_position_piece_bitboard(uo_position *position, uo_piece piece)
  {
    return (uo_bitboard *)position + piece;
  };

#pragma endregion

#pragma region uo_position_flags__functions

  static inline uint8_t uo_position_flags_color_to_move(uo_position_flags flags)
  {
    return flags & 0x0001;
  }

  static inline uint8_t uo_position_flags_rule50(uo_position_flags flags)
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

  static inline bool uo_position_flags_castling_OO(uo_position_flags flags)
  {
    return (flags & 0x1000) >> 12;
  }

  static inline bool uo_position_flags_castling_OOO(uo_position_flags flags)
  {
    return (flags & 0x2000) >> 13;
  }

  static inline bool uo_position_flags_castling_enemy_OO(uo_position_flags flags)
  {
    return (flags & 0x4000) >> 14;
  }

  static inline bool uo_position_flags_castling_enemy_OOO(uo_position_flags flags)
  {
    return (flags & 0x8000) >> 15;
  }

  static inline uo_position_flags uo_position_flags_update_color_to_move(uo_position_flags flags, uint8_t color_to_move)
  {
    return (flags & 0xFFFE) | (uo_position_flags)color_to_move;
  }

  static inline uo_position_flags uo_position_flags_update_rule50(uo_position_flags flags, uint8_t rule50)
  {
    return (flags & 0xFF01) | ((uo_position_flags)rule50 << 1);
  }

  static inline uo_position_flags uo_position_flags_update_enpassant_file(uo_position_flags flags, uint8_t enpassant_file)
  {
    return (flags & 0xF0FF) | ((uo_position_flags)enpassant_file << 8);
  }

  static inline uo_position_flags uo_position_flags_update_castling(uo_position_flags flags, uint8_t castling)
  {
    return (flags & 0x0FFF) | ((uo_position_flags)castling << 12);
  }

  static inline uo_position_flags uo_position_flags_update_castling_own(uo_position_flags flags, uint8_t castling_white)
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

  static inline uo_position_flags uo_position_flags_flip_castling(uo_position_flags flags)
  {
    uint8_t castling = uo_position_flags_castling(flags);
    castling = (castling << 2) | (castling >> 2);
    return uo_position_flags_update_castling(flags, castling);
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

  bool uo_position_is_check_move(uo_position *position, uo_move move);

  bool uo_position_is_check(uo_position *position);

  bool uo_position_is_legal_move(uo_position *position, uo_move move);

  uo_move uo_position_parse_move(uo_position *position, char str[5]);

  uo_move uo_position_parse_png_move(uo_position *position, char *png);

  size_t uo_position_print_move(uo_position *position, uo_move move, char str[6]);

#ifdef __cplusplus
}
#endif

#endif
