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
#include "uo_util.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h> 
#include <string.h> 
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

  typedef struct uo_move_history
  {
    uint64_t key;
    uo_move move;
    uo_position_flags flags;
    uint8_t move_count;
    uint8_t tactical_move_count;
    bool moves_generated;
    uint8_t repetitions;
    struct
    {
      uo_move killers[2];
    } search;
  } uo_move_history;

  typedef struct uo_position
  {
    union {
      uo_bitboard bitboards[8];
      struct {
        uo_bitboard own;
        uo_bitboard enemy;
        uo_bitboard P;
        uo_bitboard N;
        uo_bitboard B;
        uo_bitboard R;
        uo_bitboard Q;
        uo_bitboard K;
      };
    };

    uo_piece board[64];

    uo_piece captures[30];

    uo_position_flags flags;
    uint16_t ply;
    uint16_t root_ply;

    uint64_t key;
    uo_piece *piece_captured;

    struct
    {
      uo_bitboard by_BQ;
      uo_bitboard by_RQ;
      uo_bitboard by_N;
      uo_bitboard by_P;
    } checks;

    struct
    {
      uo_bitboard by_BQ;
      uo_bitboard by_RQ;
    } pins;

    struct
    {
      bool checks_and_pins;
    } update_status;

    uo_move_history *stack;
    uo_move_history history[UO_MAX_PLY + 100];

    // Relative history heuristic
    // see: https://www.researchgate.net/publication/220962554_The_Relative_History_Heuristic
    uint32_t hhtable[2][64 * 64];
    uint32_t bftable[2][64 * 64];

    struct
    {
      uo_move moves[UO_MAX_PLY * UO_BRANCING_FACTOR];
      uo_move *head;
      int16_t move_scores[0x100];
    } movelist;
  } uo_position;

#pragma region uo_position_piece_bitboard

  //static_assert(offsetof(uo_position, white) / sizeof(uo_bitboard) == uo_white, "Unexpected field offset: uo_position.white");
  //static_assert(offsetof(uo_position, black) / sizeof(uo_bitboard) == uo_black, "Unexpected field offset: uo_position.black");
  static_assert(offsetof(uo_position, P) / sizeof(uo_bitboard) == (uo_piece__P >> 1) + 1, "Unexpected field offset: uo_position.P");
  static_assert(offsetof(uo_position, N) / sizeof(uo_bitboard) == (uo_piece__N >> 1) + 1, "Unexpected field offset: uo_position.N");
  static_assert(offsetof(uo_position, B) / sizeof(uo_bitboard) == (uo_piece__B >> 1) + 1, "Unexpected field offset: uo_position.B");
  static_assert(offsetof(uo_position, R) / sizeof(uo_bitboard) == (uo_piece__R >> 1) + 1, "Unexpected field offset: uo_position.R");
  static_assert(offsetof(uo_position, Q) / sizeof(uo_bitboard) == (uo_piece__Q >> 1) + 1, "Unexpected field offset: uo_position.Q");
  static_assert(offsetof(uo_position, K) / sizeof(uo_bitboard) == (uo_piece__K >> 1) + 1, "Unexpected field offset: uo_position.K");

  static inline uo_bitboard *uo_position_piece_bitboard(uo_position *position, uo_piece piece)
  {
    return position->bitboards + (1 + (piece >> 1));
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

  static inline void uo_position_reset_root(uo_position *position)
  {
    position->piece_captured = position->captures;
    position->movelist.head = position->movelist.moves;

    size_t relevant_history_count = position->stack - position->history;
    int rule50 = uo_position_flags_rule50(position->flags);
    relevant_history_count = rule50 > relevant_history_count ? relevant_history_count : rule50;

    if (relevant_history_count > 0)
    {
      memmove(position->history, position->stack - relevant_history_count, relevant_history_count * sizeof * position->stack);
    }

    position->stack = position->history + relevant_history_count;
    position->stack->moves_generated = false;
    position->root_ply += position->ply;
    position->ply = 0;
  }

  size_t uo_position_print_fen(uo_position *const position, char fen[90]);

  size_t uo_position_print_diagram(uo_position *const position, char diagram[663]);

  void uo_position_copy(uo_position *restrict dst, const uo_position *restrict src);

  void uo_position_make_move(uo_position *position, uo_move move);

  void uo_position_unmake_move(uo_position *position);

  void uo_position_make_null_move(uo_position *position);

  void uo_position_unmake_null_move(uo_position *position);

  static inline uo_move uo_position_previous_move(uo_position *position)
  {
    return position->ply > 0 ? position->stack[-1].move : 0;
  }

  size_t uo_position_generate_moves(uo_position *position);

  static inline bool uo_position_is_check_move(const uo_position *position, uo_move move)
  {
    uo_square square_from = uo_move_square_from(move);
    uo_square square_to = uo_move_square_to(move);
    const uo_piece *board = position->board;
    uo_piece piece = board[square_from];

    assert(uo_color(piece) == uo_color_own);

    uo_bitboard mask_enemy = position->enemy;
    uo_bitboard enemy_K = mask_enemy & position->K;

    // 1. Direct checks

    if (piece == uo_piece__P && (uo_bitboard_attacks_P(square_to, uo_color_own) & enemy_K)) return true;
    if (piece == uo_piece__N && (uo_bitboard_attacks_N(square_to) & enemy_K)) return true;

    uo_bitboard bitboard_from = uo_square_bitboard(square_from);
    uo_bitboard bitboard_to = uo_square_bitboard(square_to);
    uo_bitboard mask_own = uo_andn(bitboard_from, position->own);
    uo_bitboard occupied = mask_own | mask_enemy | bitboard_to;

    switch (piece)
    {
      case uo_piece__B:
        if (uo_bitboard_attacks_B(square_to, occupied) & enemy_K) return true;
        break;

      case uo_piece__R:
        if (uo_bitboard_attacks_R(square_to, occupied) & enemy_K) return true;
        break;

      case uo_piece__Q:
        if (uo_bitboard_attacks_Q(square_to, occupied) & enemy_K) return true;
        break;
    }

    // 2. Discovered checks

    uo_square square_enemy_K = uo_tzcnt(mask_enemy & position->K);

    uo_bitboard bitboard_file = uo_square_bitboard_file[square_enemy_K];
    if (bitboard_from & bitboard_file)
    {
      if (square_from < square_enemy_K)
      {
        for (int i = square_enemy_K - 8; i >= 0; i -= 8)
        {
          if (board[i] > 1)
          {
            return board[i] == uo_piece__R || board[i] == uo_piece__Q;
          }
        }
      }
      else
      {
        for (int i = square_enemy_K + 8; i < 64; i += 8)
        {
          if (board[i] > 1)
          {
            return board[i] == uo_piece__R || board[i] == uo_piece__Q;
          }
        }
      }

      return false;
    }

    uo_bitboard bitboard_rank = uo_square_bitboard_rank[square_enemy_K];
    if (bitboard_from & bitboard_rank)
    {
      if (square_from < square_enemy_K)
      {
        for (int i = square_enemy_K - 1; uo_square_file(i) != 7; --i)
        {
          if (board[i] > 1)
          {
            return board[i] == uo_piece__R || board[i] == uo_piece__Q;
          }
        }
      }
      else
      {
        for (int i = square_enemy_K + 1; uo_square_file(i) != 0; ++i)
        {
          if (board[i] > 1)
          {
            return board[i] == uo_piece__R || board[i] == uo_piece__Q;
          }
        }
      }

      return false;
    }

    uint8_t diagonal = uo_square_diagonal[diagonal];
    uo_bitboard bitboard_diagonal = uo_bitboard_diagonal[diagonal];
    if (bitboard_from & bitboard_diagonal)
    {
      if (square_from < square_enemy_K)
      {
        for (int i = square_enemy_K - 9; i >= 0 && uo_square_diagonal[i] == diagonal; i -= 9)
        {
          if (board[i] > 1)
          {
            return board[i] == uo_piece__B || board[i] == uo_piece__Q;
          }
        }
      }
      else
      {
        for (int i = square_enemy_K + 9; i < 64 && uo_square_diagonal[i] == diagonal; i += 9)
        {
          if (board[i] > 1)
          {
            return board[i] == uo_piece__B || board[i] == uo_piece__Q;
          }
        }
      }

      return false;
    }

    uint8_t antidiagonal = uo_square_antidiagonal[antidiagonal];
    uo_bitboard bitboard_antidiagonal = uo_bitboard_antidiagonal[antidiagonal];
    if (bitboard_from & bitboard_antidiagonal)
    {
      if (square_from < square_enemy_K)
      {
        for (int i = square_enemy_K - 7; i >= 0 && uo_square_antidiagonal[i] == antidiagonal; i -= 7)
        {
          if (board[i] > 1)
          {
            return board[i] == uo_piece__B || board[i] == uo_piece__Q;
          }
        }
      }
      else
      {
        for (int i = square_enemy_K + 7; i < 64 && uo_square_antidiagonal[i] == antidiagonal; i += 7)
        {
          if (board[i] > 1)
          {
            return board[i] == uo_piece__B || board[i] == uo_piece__Q;
          }
        }
      }

      return false;
    }

    return false;
  }

  static inline bool uo_position_is_rule50_draw(const uo_position *position)
  {
    return uo_position_flags_rule50(position->flags) >= 100;
  }

  static inline bool uo_position_is_repetition_draw(const uo_position *position)
  {
    return position->stack->repetitions == 3;
  }

  static inline bool uo_position_is_max_depth_reached(const uo_position *position)
  {
    return position->ply >= UO_MAX_PLY;
  }

  static inline void uo_position_update_checks_and_pins(uo_position *position)
  {
    uo_bitboard mask_own = position->own;
    uo_bitboard mask_enemy = position->enemy;
    uo_bitboard occupied = mask_own | mask_enemy;

    uo_bitboard enemy_P = mask_enemy & position->P;
    uo_bitboard enemy_N = mask_enemy & position->N;
    uo_bitboard enemy_BQ = mask_enemy & (position->B | position->Q);
    uo_bitboard enemy_RQ = mask_enemy & (position->R | position->Q);

    uo_bitboard own_K = mask_own & position->K;
    uo_square square_own_K = uo_tzcnt(own_K);

    position->checks.by_BQ = enemy_BQ & uo_bitboard_attacks_B(square_own_K, occupied);
    position->checks.by_RQ = enemy_RQ & uo_bitboard_attacks_R(square_own_K, occupied);
    position->checks.by_N = enemy_N & uo_bitboard_attacks_N(square_own_K);
    position->checks.by_P = enemy_P & uo_bitboard_attacks_P(square_own_K, uo_color_own);

    position->pins.by_BQ = uo_bitboard_pins_B(square_own_K, occupied, enemy_BQ);
    position->pins.by_RQ = uo_bitboard_pins_R(square_own_K, occupied, enemy_RQ);

    position->update_status.checks_and_pins = true;
  }

  static inline bool uo_position_is_check(uo_position *position)
  {
    if (!position->update_status.checks_and_pins)
    {
      uo_position_update_checks_and_pins(position);
    }

    return position->checks.by_P | position->checks.by_N | position->checks.by_BQ | position->checks.by_RQ;
  }

  static inline bool uo_position_is_null_move_allowed(uo_position *position)
  {
    return !uo_position_is_check(position)
      && uo_position_previous_move(position) != 0
      && uo_andn(position->P | position->K, position->own);
  }

  // see: https://www.chessprogramming.org/SEE_-_The_Swap_Algorithm
  static inline int16_t uo_position_move_sse(const uo_position *position, uo_move move)
  {
    uo_square square_from = uo_move_square_from(move);
    uo_square square_to = uo_move_square_to(move);
    const uo_piece *board = position->board;
    uo_piece piece = board[square_from];
    uo_piece piece_captured = board[square_to];

    assert(uo_color(piece) == uo_color_own);

    uo_bitboard bitboard_from = uo_square_bitboard(square_from);
    uo_bitboard bitboard_to = uo_square_bitboard(square_to);

    uo_bitboard mask_own = uo_andn(bitboard_from, position->own);
    uo_bitboard mask_enemy = uo_andn(bitboard_to, position->enemy);
    uo_bitboard occupied = mask_own | mask_enemy;

    uo_bitboard attacks_by_Q = mask_enemy & position->Q & uo_bitboard_attacks_Q(square_to, occupied);
    uo_bitboard attacks_by_R = mask_enemy & position->R & uo_bitboard_attacks_R(square_to, occupied);
    uo_bitboard attacks_by_B = mask_enemy & position->B & uo_bitboard_attacks_B(square_to, occupied);
    uo_bitboard attacks_by_N = mask_enemy & position->N & uo_bitboard_attacks_N(square_to);
    uo_bitboard attacks_by_P = mask_enemy & position->P & uo_bitboard_attacks_P(square_to, uo_color_own);
    uo_bitboard attacks_by_K = mask_enemy & position->K & uo_bitboard_attacks_K(square_to);

    uo_bitboard defended_by_Q = mask_own & position->Q & uo_bitboard_attacks_Q(square_to, occupied);
    uo_bitboard defended_by_R = mask_own & position->R & uo_bitboard_attacks_R(square_to, occupied);
    uo_bitboard defended_by_B = mask_own & position->B & uo_bitboard_attacks_B(square_to, occupied);
    uo_bitboard defended_by_N = mask_own & position->N & uo_bitboard_attacks_N(square_to);
    uo_bitboard defended_by_P = mask_own & position->P & uo_bitboard_attacks_P(square_to, uo_color_enemy);
    uo_bitboard defended_by_K = mask_own & position->K & uo_bitboard_attacks_K(square_to);

    int16_t gain[32];
    size_t depth = 0;

    gain[depth] = up_piece_value(piece_captured);

    while (true)
    {
      depth++;

      // speculative store, if defended
      gain[depth] = up_piece_value(piece) - gain[depth - 1];

      if (gain[depth - 1] > 0 && gain[depth] < 0)
      {
        // pruning does not influence the result
        break;
      }

      if (attacks_by_P)
      {
        square_from = uo_bitboard_next_square(&attacks_by_P);
        bitboard_from = uo_square_bitboard(square_from);
        piece = uo_piece__P;
        occupied ^= bitboard_from;

        attacks_by_Q = mask_enemy & position->Q & uo_bitboard_attacks_Q(square_to, occupied);
        attacks_by_B = mask_enemy & position->B & uo_bitboard_attacks_B(square_to, occupied);
        defended_by_Q = mask_own & position->Q & uo_bitboard_attacks_Q(square_to, occupied);
        defended_by_B = mask_own & position->B & uo_bitboard_attacks_B(square_to, occupied);
      }
      else if (attacks_by_N)
      {
        square_from = uo_bitboard_next_square(&attacks_by_N);
        bitboard_from = uo_square_bitboard(square_from);
        piece = uo_piece__N;
        occupied ^= bitboard_from;
      }
      else if (attacks_by_B)
      {
        square_from = uo_bitboard_next_square(&attacks_by_B);
        bitboard_from = uo_square_bitboard(square_from);
        piece = uo_piece__B;
        occupied ^= bitboard_from;

        attacks_by_Q = mask_enemy & position->Q & uo_bitboard_attacks_Q(square_to, occupied);
        attacks_by_B = mask_enemy & position->B & uo_bitboard_attacks_B(square_to, occupied);
        defended_by_Q = mask_own & position->Q & uo_bitboard_attacks_Q(square_to, occupied);
        defended_by_B = mask_own & position->B & uo_bitboard_attacks_B(square_to, occupied);
      }
      else if (attacks_by_R)
      {
        square_from = uo_bitboard_next_square(&attacks_by_R);
        bitboard_from = uo_square_bitboard(square_from);
        piece = uo_piece__R;
        occupied ^= bitboard_from;

        attacks_by_Q = mask_enemy & position->Q & uo_bitboard_attacks_Q(square_to, occupied);
        attacks_by_R = mask_enemy & position->R & uo_bitboard_attacks_R(square_to, occupied);
        defended_by_Q = mask_own & position->Q & uo_bitboard_attacks_Q(square_to, occupied);
        defended_by_R = mask_own & position->R & uo_bitboard_attacks_R(square_to, occupied);
      }
      else if (attacks_by_Q)
      {
        square_from = uo_bitboard_next_square(&attacks_by_Q);
        bitboard_from = uo_square_bitboard(square_from);
        piece = uo_piece__Q;
        occupied ^= bitboard_from;

        attacks_by_Q = mask_enemy & position->Q & uo_bitboard_attacks_Q(square_to, occupied);
        attacks_by_R = mask_enemy & position->R & uo_bitboard_attacks_R(square_to, occupied);
        attacks_by_B = mask_enemy & position->B & uo_bitboard_attacks_B(square_to, occupied);
        defended_by_Q = mask_own & position->Q & uo_bitboard_attacks_Q(square_to, occupied);
        defended_by_R = mask_own & position->R & uo_bitboard_attacks_R(square_to, occupied);
        defended_by_B = mask_own & position->B & uo_bitboard_attacks_B(square_to, occupied);
      }
      else if (attacks_by_K)
      {
        square_from = uo_bitboard_next_square(&attacks_by_K);
        bitboard_from = uo_square_bitboard(square_from);
        occupied ^= bitboard_from;
        piece = uo_piece__K;

        attacks_by_Q = mask_enemy & position->Q & uo_bitboard_attacks_Q(square_to, occupied);
        attacks_by_R = mask_enemy & position->R & uo_bitboard_attacks_R(square_to, occupied);
        attacks_by_B = mask_enemy & position->B & uo_bitboard_attacks_B(square_to, occupied);
        defended_by_Q = mask_own & position->Q & uo_bitboard_attacks_Q(square_to, occupied);
        defended_by_R = mask_own & position->R & uo_bitboard_attacks_R(square_to, occupied);
        defended_by_B = mask_own & position->B & uo_bitboard_attacks_B(square_to, occupied);
      }
      else
      {
        break;
      }

      uo_bitboard temp;

      temp = mask_own;
      mask_own = mask_enemy & occupied;
      mask_enemy = temp;

      temp = attacks_by_Q;
      attacks_by_Q = defended_by_Q;
      defended_by_Q = temp;

      temp = attacks_by_R;
      attacks_by_R = defended_by_R;
      defended_by_R = temp;

      temp = attacks_by_B;
      attacks_by_B = defended_by_B;
      defended_by_B = temp;

      temp = attacks_by_N;
      attacks_by_N = defended_by_N;
      defended_by_N = temp;

      temp = attacks_by_P;
      attacks_by_P = defended_by_P;
      defended_by_P = temp;

      temp = attacks_by_K;
      attacks_by_K = defended_by_K;
      defended_by_K = temp;
    }

    while (--depth)
    {
      gain[depth - 1] = -(gain[depth] > -gain[depth - 1] ? gain[depth] : -gain[depth - 1]);
    }

    return gain[0];
  }

  static inline bool uo_position_move_threatens_material(uo_position *position, uo_move move)
  {
    uo_square square_from = uo_move_square_from(move);
    uo_square square_to = uo_move_square_to(move);
    uo_piece *board = position->board;
    uo_piece piece = board[square_from];

    assert(uo_color(piece) == uo_color_own);

    bool square_defended_by_P = 0 != (uo_bitboard_attacks_P(square_to, uo_color_own) & position->enemy & position->P);
    if (square_defended_by_P)
    {
      return false;
    }

    uo_bitboard bitboard_from = uo_square_bitboard(square_from);
    uo_bitboard bitboard_to = uo_square_bitboard(square_to);

    uo_bitboard mask_own = uo_andn(bitboard_from, position->own);
    uo_bitboard mask_enemy = uo_andn(bitboard_to, position->enemy);

    uo_bitboard occupied = mask_own | mask_enemy;

    uo_bitboard enemy_P = mask_enemy & position->P;
    uo_bitboard defended_by_enemy_P = uo_bitboard_attacks_left_enemy_P(enemy_P) | uo_bitboard_attacks_right_enemy_P(enemy_P);
    uo_bitboard mask_undefended_by_P = mask_enemy & ~defended_by_enemy_P;

    switch (piece)
    {
      case uo_piece__P:
        return 0 != (uo_bitboard_attacks_P(square_to, uo_color_own) & mask_enemy & (position->N | position->B | position->R | position->Q));

      case uo_piece__N:
        return 0 != (uo_bitboard_attacks_N(square_to) & mask_enemy & (position->R | position->Q));

      case uo_piece__B:
        return 0 != (uo_bitboard_attacks_B(square_to, occupied) & mask_enemy & (position->R | position->Q));

      case uo_piece__R:
        return 0 != (uo_bitboard_attacks_R(square_to, occupied) & mask_enemy & position->Q);
    }

    return false;
  }

  bool uo_position_is_legal_move(uo_position *position, uo_move move);

  static inline bool uo_position_is_killer_move(const uo_position *position, uo_move move)
  {
    uo_move *killers = position->stack->search.killers;

    return !uo_move_is_capture(move) && (move == killers[0] || move == killers[1]);
  }

  static inline uint16_t uo_position_calculate_non_tactical_move_score(uo_position *position, uo_move move)
  {
    if (uo_position_is_killer_move(position, move))
    {
      return 1000;
    }

    // relative history heuristic
    uint8_t color = uo_color(position->flags);
    uint16_t index = move & 0xFFF;
    uint32_t hhscore = position->hhtable[color][index] << 4;
    uint32_t bfscore = position->bftable[color][index] + 1;
    return hhscore / bfscore;
  }

  static inline uint16_t uo_position_calculate_tactical_move_score(uo_position *position, uo_move move)
  {
    uo_square move_type = uo_move_get_type(move);

    if (move_type == uo_move_type__enpassant)
    {
      return 3000;
    }

    if (move_type & uo_move_type__promo)
    {
      return 3000;
    }

    if (move_type & uo_move_type__x)
    {
      uo_square square_from = uo_move_square_from(move);
      uo_square square_to = uo_move_square_to(move);
      uo_piece piece = position->board[square_from];
      uo_piece piece_captured = position->board[square_to];
      return 2000 + up_piece_value(piece_captured) - up_piece_value(piece) / 10;
    }

    return 0;
  }

  static inline int uo_position_partition_moves(uo_position *position, uo_move *movelist, int lo, int hi)
  {
    int16_t *move_scores = position->movelist.move_scores;
    int mid = (lo + hi) >> 1;
    uo_move temp_move;
    int16_t temp_score;

    if (move_scores[mid] > move_scores[lo])
    {
      temp_move = movelist[mid];
      movelist[mid] = movelist[lo];
      movelist[lo] = temp_move;

      temp_score = move_scores[mid];
      move_scores[mid] = move_scores[lo];
      move_scores[lo] = temp_score;
    }

    if (move_scores[hi] > move_scores[lo])
    {
      temp_move = movelist[hi];
      movelist[hi] = movelist[lo];
      movelist[lo] = temp_move;

      temp_score = move_scores[hi];
      move_scores[hi] = move_scores[lo];
      move_scores[lo] = temp_score;
    }

    if (move_scores[mid] > move_scores[hi])
    {
      temp_move = movelist[mid];
      movelist[mid] = movelist[hi];
      movelist[hi] = temp_move;

      temp_score = move_scores[mid];
      move_scores[mid] = move_scores[hi];
      move_scores[hi] = temp_score;
    }

    int8_t pivot = move_scores[hi];

    for (int j = lo; j < hi - 1; ++j)
    {
      if (move_scores[j] > pivot)
      {

        temp_move = movelist[lo];
        movelist[lo] = movelist[j];
        movelist[j] = temp_move;

        temp_score = move_scores[lo];
        move_scores[lo] = move_scores[j];
        move_scores[j] = temp_score;

        ++lo;
      }
    }

    temp_move = movelist[lo];
    movelist[lo] = movelist[hi];
    movelist[hi] = temp_move;

    temp_score = move_scores[lo];
    move_scores[lo] = move_scores[hi];
    move_scores[hi] = temp_score;

    return lo;
  }

  // Limited quicksort which prioritizes ordering beginning of movelist
  static inline void uo_position_quicksort_moves(uo_position *position, uo_move *movelist, int lo, int hi, int depth)
  {
    if (lo >= hi || depth <= 0)
    {
      return;
    }

    int p = uo_position_partition_moves(position, movelist, lo, hi);
    uo_position_quicksort_moves(position, movelist, lo, p - 1, depth - 1);
    uo_position_quicksort_moves(position, movelist, p + 1, hi, depth - 2);
  }

  static inline void uo_position_sort_tactical_moves(uo_position *position)
  {
    uo_move *movelist = position->movelist.head;
    size_t tactical_move_count = position->stack->tactical_move_count;
    size_t i = 0;

    while (i < tactical_move_count)
    {
      uo_move move = movelist[i];
      uint16_t score = uo_position_calculate_tactical_move_score(position, move);
      position->movelist.move_scores[i] = score;
      ++i;
    }

    const size_t quicksort_depth = 4; // arbitrary
    uo_position_quicksort_moves(position, movelist, 0, tactical_move_count - 1, quicksort_depth);
  }

  static inline void uo_position_sort_non_tactical_moves(uo_position *position)
  {
    uo_move *movelist = position->movelist.head;
    size_t tactical_move_count = position->stack->tactical_move_count;
    size_t move_count = position->stack->move_count;
    size_t i = tactical_move_count;

    while (i < move_count)
    {
      uo_move move = movelist[i];
      uint16_t score = uo_position_calculate_tactical_move_score(position, move);
      position->movelist.move_scores[i] = score;
      ++i;
    }

    const size_t quicksort_depth = 4; // arbitrary
    uo_position_quicksort_moves(position, movelist, tactical_move_count, move_count - 1, quicksort_depth);
  }

  static inline void uo_position_sort_moves(uo_position *position, uo_move bestmove)
  {
    uo_move *movelist = position->movelist.head;
    size_t move_count = position->stack->move_count;
    uo_position_sort_tactical_moves(position);
    uo_position_sort_non_tactical_moves(position);

    // set bestmove as first
    if (bestmove && movelist[0] != bestmove)
    {
      for (size_t i = 1; i < move_count; ++i)
      {
        if (movelist[i] == bestmove)
        {
          movelist[i] = movelist[0];
          movelist[0] = bestmove;
          break;
        }
      }
    }
  }

  static inline bool uo_position_is_quiescent(uo_position *position)
  {
    if (!position->stack->moves_generated)
    {
      uo_position_generate_moves(position);
    }

    if (uo_position_is_check(position))
    {
      return true;
    }

    size_t move_count = position->stack->move_count;

    for (size_t i = 0; i < move_count; ++i)
    {
      uo_move move = position->movelist.head[i];

      bool is_tacktical_move = uo_move_is_promotion(move)
        || (uo_move_is_capture(move) && uo_position_move_sse(position, move) >= 0);

      if (is_tacktical_move)
      {
        return false;
      }
    }

    return true;
  }

  uo_move uo_position_parse_move(uo_position *const position, char str[5]);

  uo_move uo_position_parse_png_move(uo_position *position, char *png);

  size_t uo_position_print_move(uo_position *const position, uo_move move, char str[6]);

  size_t uo_position_perft(uo_position *position, size_t depth);

#ifdef __cplusplus
}
#endif

#endif
