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
#include "uo_zobrist.h"

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
      0-100
    enpassant_file : 4
      0-8
    castling: 4
      1 K - h1
      2 Q - a1
      4 k - h8
      8 q - a8
  */

#define uo_move_history__checks_none uo_bitboard_all
#define uo_move_history__checks_unknown 0

#define uo_move_score_skip INT16_MIN

  typedef struct uo_move_cache
  {
    uint64_t key;
    uo_move move;
    uo_bitboard checks;
    int16_t see;

    struct
    {
      bool checks;
      bool see;
    } is_cached;
  } uo_move_cache;

  typedef struct uo_move_history
  {
    uint64_t key;
    uo_move move;
    uo_bitboard checks;
    int16_t static_eval;
    uo_position_flags flags;
    uint8_t move_count;
    uint8_t tactical_move_count;
    uint8_t skipped_move_count;
    bool moves_generated;
    bool moves_sorted;
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
    uint16_t material;

    uo_position_flags flags;
    uint16_t ply;
    uint16_t root_ply;

    uint64_t key;
    uo_piece *piece_captured;

    struct
    {
      bool updated;
      uo_bitboard by_BQ;
      uo_bitboard by_RQ;
    } pins;

    uo_move_history *stack;
    uo_move_history history[UO_MAX_PLY + 100];

    // Relative history heuristic
    // see: https://www.researchgate.net/publication/220962554_The_Relative_History_Heuristic
    uint32_t hhtable[2 * 6 * 64];
    uint32_t bftable[2 * 6 * 64];

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
  uo_position *uo_position_from_fen(uo_position *position, const char *fen);

  static inline uo_move_cache *uo_move_cache_get(uo_move_cache move_cache[0x1000], const uo_position *position, uo_move move)
  {
    if (!move_cache) return false;
    move_cache += (move ^ position->key) & 0xFFF;

    if (move_cache->key != position->key || move_cache->move != move)
    {
      *move_cache = (uo_move_cache){
        .key = position->key,
        .move = move
      };
    }

    return move_cache;
  }

  static inline void uo_position_reset_root(uo_position *position)
  {
    // Reset stack of captures
    position->piece_captured = position->captures;

    // Reset move list stack
    position->movelist.head = position->movelist.moves;

    // To maintain the current move history on the stack, it is important to keep the move history up to the last irreversible move.
    // However, since we need to reference the static evaluation of the past two moves, we should ensure that at least four plies are retained.
    int rule50 = uo_position_flags_rule50(position->flags);
    size_t relevant_history_count = uo_min(uo_max(rule50, 4), position->stack - position->history);

    // Get the stack pointer to the beginning of the relevant history
    uo_move_history *stack = &position->history[relevant_history_count];

    if (stack != position->stack)
    {
      // Copy the relevant history to the beginning of the history stack
      memmove(position->history, position->stack - relevant_history_count, relevant_history_count * sizeof * stack);

      // Save the current checks
      uo_bitboard checks = position->stack->checks;

      // Clear the stack
      memset(stack, 0, (position->stack - stack) * sizeof * stack);

      // Set the stack pointer to the beginning of the relevant history
      position->stack = stack;

      // Restore the current checks and other fields on the stack
      stack->checks = checks;
    }

    // Reset the position key and flags on the stack
    stack->key = position->key;
    stack->flags = position->flags;

    // Reset the ply counter
    position->root_ply += position->ply;
    position->ply = 0;
  }

  size_t uo_position_print_fen(uo_position *position, char fen[90]);

  size_t uo_position_print_diagram(uo_position *position, char diagram[663]);

  void uo_position_copy(uo_position *restrict dst, const uo_position *restrict src);

  void uo_position_make_move(uo_position *position, uo_move move);

  void uo_position_unmake_move(uo_position *position);

  void uo_position_make_null_move(uo_position *position);

  void uo_position_unmake_null_move(uo_position *position);

  static inline uo_move uo_position_previous_move(const uo_position *position)
  {
    return position->ply > 0 ? position->stack[-1].move : 0;
  }

  size_t uo_position_generate_moves(uo_position *position);

  static inline uint8_t uo_position_material_percentage(const uo_position *position)
  {
    return (uint32_t)100 * (uint32_t)position->material / (uint32_t)uo_score_material_max;
  }

  static inline uo_bitboard uo_position_move_checks(const uo_position *position, uo_move move, uo_move_cache *move_cache)
  {
    move_cache = uo_move_cache_get(move_cache, position, move);

    if (move_cache && move_cache->is_cached.checks)
    {
      return move_cache->checks;
    }

    uo_square square_from = uo_move_square_from(move);
    uo_square square_to = uo_move_square_to(move);
    const uo_piece *board = position->board;
    uo_piece piece = board[square_from];

    assert(uo_color(piece) == uo_color_own);

    uo_bitboard bitboard_from = uo_square_bitboard(square_from);
    uo_bitboard bitboard_to = uo_square_bitboard(square_to);
    uo_bitboard mask_own = uo_andn(bitboard_from, position->own);
    uo_bitboard mask_enemy = position->enemy;
    uo_bitboard own_BQ = mask_own & (position->B | position->Q);
    uo_bitboard own_RQ = mask_own & (position->R | position->Q);
    uo_bitboard occupied = mask_own | mask_enemy | bitboard_to;

    uo_square square_enemy_K = uo_tzcnt(mask_enemy & position->K);

    uo_bitboard checks = 0;

    switch (piece)
    {
      case uo_piece__P:
        if (uo_move_is_enpassant(move))
        {
          occupied = uo_andn(uo_square_bitboard(square_to - 8), occupied);
          checks |= uo_bitboard_attacks_P(square_enemy_K, uo_color_enemy) & bitboard_to;
        }
        else if (uo_move_is_promotion(move))
        {
          switch (uo_move_get_type(move) & uo_move_type__promo_Q)
          {
            case uo_move_type__promo_N:
              checks |= uo_bitboard_attacks_N(square_enemy_K) & bitboard_to;
              break;

            case uo_move_type__promo_B:
              own_BQ |= bitboard_to;
              break;

            case uo_move_type__promo_R:
              own_RQ |= bitboard_to;
              break;

            case uo_move_type__promo_Q:
              own_BQ |= bitboard_to;
              own_RQ |= bitboard_to;
              break;
          }
        }
        else
        {
          checks |= uo_bitboard_attacks_P(square_enemy_K, uo_color_enemy) & bitboard_to;
        }

        break;

      case uo_piece__N:
        checks |= uo_bitboard_attacks_N(square_enemy_K) & bitboard_to;
        break;

      case uo_piece__B:
        own_BQ |= bitboard_to;
        break;

      case uo_piece__R:
        own_RQ |= bitboard_to;
        break;

      case uo_piece__Q:
        own_BQ |= bitboard_to;
        own_RQ |= bitboard_to;
        break;

      case uo_piece__K:
        switch (uo_move_get_type(move))
        {
          case uo_move_type__OO:
            own_RQ |= uo_square_bitboard(uo_square__f1);
            break;

          case uo_move_type__OOO:
            own_RQ |= uo_square_bitboard(uo_square__d1);
            break;
        }
    }

    checks |= uo_bitboard_attacks_B(square_enemy_K, occupied) & own_BQ;
    checks |= uo_bitboard_attacks_R(square_enemy_K, occupied) & own_RQ;

    if (move_cache)
    {
      move_cache->is_cached.checks = true;
      move_cache->checks = checks;
    }

    return checks;
  }

  static inline void uo_position_update_next_move_checks(uo_position *position, uo_bitboard checks)
  {
    position->stack[1].checks = checks ? uo_bswap(checks) : uo_move_history__checks_none;
  }

  static inline bool uo_position_is_rule50_draw(const uo_position *position)
  {
    return uo_position_flags_rule50(position->flags) >= 100;
  }

  static inline uint8_t uo_position_repetition_count(const uo_position *position)
  {
    return position->stack->repetitions;
  }

  static inline bool uo_position_is_max_depth_reached(const uo_position *position)
  {
    return position->ply >= UO_MAX_PLY;
  }

  static inline void uo_position_update_pins(uo_position *position)
  {
    uo_bitboard mask_own = position->own;
    uo_bitboard mask_enemy = position->enemy;
    uo_bitboard occupied = mask_own | mask_enemy;

    uo_bitboard enemy_BQ = mask_enemy & (position->B | position->Q);
    uo_bitboard enemy_RQ = mask_enemy & (position->R | position->Q);

    uo_bitboard own_K = mask_own & position->K;
    uo_square square_own_K = uo_tzcnt(own_K);

    position->pins.updated = true;
    position->pins.by_BQ = uo_bitboard_pins_B(square_own_K, occupied, enemy_BQ);
    position->pins.by_RQ = uo_bitboard_pins_R(square_own_K, occupied, enemy_RQ);
  }

  static inline bool uo_position_is_check(uo_position *position)
  {
    if (position->stack->checks == uo_move_history__checks_unknown)
    {
      uo_bitboard mask_own = position->own;
      uo_bitboard mask_enemy = position->enemy;
      uo_bitboard occupied = mask_own | mask_enemy;
      uo_bitboard empty = ~occupied;

      uo_bitboard own_K = mask_own & position->K;
      uo_square square_own_K = uo_tzcnt(own_K);

      uo_bitboard enemy_P = mask_enemy & position->P;
      uo_bitboard enemy_N = mask_enemy & position->N;
      uo_bitboard enemy_B = mask_enemy & position->B;
      uo_bitboard enemy_R = mask_enemy & position->R;
      uo_bitboard enemy_Q = mask_enemy & position->Q;
      uo_bitboard enemy_K = mask_enemy & position->K;
      uo_bitboard enemy_BQ = enemy_B | enemy_Q;
      uo_bitboard enemy_RQ = enemy_R | enemy_Q;

      uo_bitboard checks = (enemy_BQ & uo_bitboard_attacks_B(square_own_K, occupied))
        | (enemy_RQ & uo_bitboard_attacks_R(square_own_K, occupied))
        | (enemy_N & uo_bitboard_attacks_N(square_own_K))
        | (enemy_P & uo_bitboard_attacks_P(square_own_K, uo_color_own));

      position->stack->checks = checks ? checks : uo_move_history__checks_none;
    }

    return position->stack->checks != uo_move_history__checks_none;
  }

  static inline uo_bitboard uo_position_checks(const uo_position *position)
  {
    return position->stack->checks != uo_move_history__checks_none ? position->stack->checks : 0;
  }

  static inline uo_bitboard uo_position_attacks_own_NBRQ(const uo_position *position)
  {
    uo_bitboard attacks = 0;
    uo_bitboard mask_own = position->own;
    uo_bitboard occupied = mask_own | position->enemy;

    uo_bitboard own_N = mask_own & position->N;
    while (own_N)
    {
      uo_square square = uo_bitboard_next_square(&own_N);
      attacks |= uo_bitboard_attacks_N(square);
    }

    uo_bitboard own_RQ = mask_own & (position->R | position->Q);
    while (own_RQ)
    {
      uo_square square = uo_bitboard_next_square(&own_RQ);
      attacks |= uo_bitboard_attacks_R(square, occupied);
    }

    uo_bitboard own_BQ = mask_own & (position->B | position->Q);
    while (own_BQ)
    {
      uo_square square = uo_bitboard_next_square(&own_BQ);
      attacks |= uo_bitboard_attacks_B(square, occupied);
    }

    return attacks;
  }

  static inline uo_bitboard uo_position_attacks_enemy_NBRQ(const uo_position *position)
  {
    uo_bitboard attacks = 0;
    uo_bitboard mask_enemy = position->enemy;
    uo_bitboard occupied = mask_enemy | position->own;

    uo_bitboard enemy_N = mask_enemy & position->N;
    while (enemy_N)
    {
      uo_square square = uo_bitboard_next_square(&enemy_N);
      attacks |= uo_bitboard_attacks_N(square);
    }

    uo_bitboard enemy_RQ = mask_enemy & (position->R | position->Q);
    while (enemy_RQ)
    {
      uo_square square = uo_bitboard_next_square(&enemy_RQ);
      attacks |= uo_bitboard_attacks_R(square, occupied);
    }

    uo_bitboard enemy_BQ = mask_enemy & (position->B | position->Q);
    while (enemy_BQ)
    {
      uo_square square = uo_bitboard_next_square(&enemy_BQ);
      attacks |= uo_bitboard_attacks_B(square, occupied);
    }

    return attacks;
  }

  static inline bool uo_position_is_null_move_allowed(uo_position *position)
  {
    return !uo_position_is_check(position)
      && uo_position_previous_move(position) != 0
      && uo_popcnt(position->own | position->enemy) > 6;
  }

  // see: https://www.chessprogramming.org/SEE_-_The_Swap_Algorithm
  static inline int16_t uo_position_move_see(const uo_position *position, uo_move move, uo_move_cache *move_cache)
  {
    move_cache = uo_move_cache_get(move_cache, position, move);

    if (move_cache && move_cache->is_cached.see)
    {
      return move_cache->see;
    }

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

    gain[depth] = uo_piece_value(piece_captured);

    while (true)
    {
      depth++;

      // speculative store, if defended
      gain[depth] = uo_piece_value(piece) - gain[depth - 1];

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

    if (move_cache)
    {
      move_cache->is_cached.see = true;
      move_cache->see = gain[0];
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

  // TODO: Write tests for this function
  static inline bool uo_position_is_legal_move(uo_position *position, uo_move move)
  {
    uo_piece *board = position->board;
    uo_square square_from = uo_move_square_from(move);
    uo_piece piece = board[square_from];

    if (piece <= 1) return false;
    if (uo_color(piece) != uo_color_own) return false;

    if (position->stack->moves_generated)
    {
      size_t move_count = position->stack->move_count;

      for (size_t i = 0; i < move_count; ++i)
      {
        if (move == position->movelist.head[i])
        {
          return true;
        }
      }

      return false;
    }
    else
    {
      uo_move_history *stack = position->stack;
      uo_square square_to = uo_move_square_to(move);
      uo_bitboard bitboard_to = uo_square_bitboard(square_to);
      uo_bitboard bitboard_from = uo_square_bitboard(square_from);

      uo_bitboard mask_own = position->own;
      uo_bitboard mask_enemy = position->enemy;
      uo_bitboard occupied = mask_own | mask_enemy;
      uo_bitboard empty = ~occupied;

      uo_bitboard own_K = mask_own & position->K;
      uo_bitboard enemy_P = mask_enemy & position->P;
      uo_bitboard enemy_N = mask_enemy & position->N;
      uo_bitboard enemy_B = mask_enemy & position->B;
      uo_bitboard enemy_R = mask_enemy & position->R;
      uo_bitboard enemy_Q = mask_enemy & position->Q;
      uo_bitboard enemy_K = mask_enemy & position->K;
      uo_bitboard enemy_BQ = enemy_B | enemy_Q;
      uo_bitboard enemy_RQ = enemy_R | enemy_Q;

      uo_bitboard attacks_enemy_P = uo_bitboard_attacks_enemy_P(enemy_P);

      uo_square square_own_K = uo_tzcnt(own_K);

      if (stack->checks == uo_move_history__checks_unknown)
      {
        uo_bitboard checks = (enemy_BQ & uo_bitboard_attacks_B(square_own_K, occupied))
          | (enemy_RQ & uo_bitboard_attacks_R(square_own_K, occupied))
          | (enemy_N & uo_bitboard_attacks_N(square_own_K))
          | (enemy_P & uo_bitboard_attacks_P(square_own_K, uo_color_own));

        stack->checks = checks ? checks : uo_move_history__checks_none;
      }

      uo_bitboard moves_K = uo_bitboard_moves_K(square_own_K, mask_own, mask_enemy);
      moves_K = uo_andn(attacks_enemy_P, moves_K);

      if (piece == uo_piece__K)
      {
        if (moves_K & bitboard_to)
        {
          uo_bitboard occupied_after_move_by_K = uo_andn(own_K, occupied);
          uo_bitboard enemy_checks_BQ = enemy_BQ & uo_bitboard_attacks_B(square_to, occupied_after_move_by_K);
          uo_bitboard enemy_checks_RQ = enemy_RQ & uo_bitboard_attacks_R(square_to, occupied_after_move_by_K);
          uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(square_to);
          uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(square_to);
          uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_BQ | enemy_checks_RQ | enemy_checks_K;

          return !enemy_checks;
        }
        else if (uo_position_is_check(position))
        {
          return false;
        }

        // Castling moves

        if (square_to == uo_square__g1
          && uo_position_flags_castling_OO(position->flags)
          && !(occupied & (uo_square_bitboard(uo_square__f1) | uo_square_bitboard(uo_square__g1))))
        {
          // Kingside castling
          uo_bitboard enemy_checks_BQ = enemy_BQ & uo_bitboard_attacks_B(uo_square__f1, occupied);
          uo_bitboard enemy_checks_RQ = enemy_RQ & uo_bitboard_attacks_R(uo_square__f1, occupied);
          uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(uo_square__f1);
          uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(uo_square__f1, uo_color_own);
          uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(uo_square__f1);
          uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_BQ | enemy_checks_RQ | enemy_checks_P | enemy_checks_K;

          if (!enemy_checks)
          {
            uo_bitboard enemy_checks_BQ = enemy_BQ & uo_bitboard_attacks_B(uo_square__g1, occupied);
            uo_bitboard enemy_checks_RQ = enemy_RQ & uo_bitboard_attacks_R(uo_square__g1, occupied);
            uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(uo_square__g1);
            uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(uo_square__g1, uo_color_own);
            uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(uo_square__g1);
            uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_BQ | enemy_checks_RQ | enemy_checks_P | enemy_checks_K;

            return !enemy_checks;
          }

          return true;
        }

        if (square_to == uo_square__c1
          && uo_position_flags_castling_OOO(position->flags)
          && !(occupied & (uo_square_bitboard(uo_square__d1) | uo_square_bitboard(uo_square__c1) | uo_square_bitboard(uo_square__b1))))
        {
          // Queenside castling
          uo_bitboard enemy_checks_BQ = enemy_BQ & uo_bitboard_attacks_B(uo_square__d1, occupied);
          uo_bitboard enemy_checks_RQ = enemy_RQ & uo_bitboard_attacks_R(uo_square__d1, occupied);
          uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(uo_square__d1);
          uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(uo_square__d1, uo_color_own);
          uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(uo_square__d1);
          uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_BQ | enemy_checks_RQ | enemy_checks_P | enemy_checks_K;

          if (!enemy_checks)
          {
            uo_bitboard enemy_checks_BQ = enemy_BQ & uo_bitboard_attacks_B(uo_square__c1, occupied);
            uo_bitboard enemy_checks_RQ = enemy_RQ & uo_bitboard_attacks_R(uo_square__c1, occupied);
            uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(uo_square__c1);
            uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(uo_square__c1, uo_color_own);
            uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(uo_square__c1);
            uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_BQ | enemy_checks_RQ | enemy_checks_P | enemy_checks_K;

            return !enemy_checks;
          }

          return true;
        }

        return false;
      }

      if (!position->pins.updated)
      {
        position->pins.by_BQ = uo_bitboard_pins_B(square_own_K, occupied, enemy_BQ);
        position->pins.by_RQ = uo_bitboard_pins_R(square_own_K, occupied, enemy_RQ);
        position->pins.updated = true;
      }

      uo_bitboard pins_to_own_K_by_BQ = position->pins.by_BQ;
      uo_bitboard pins_to_own_K_by_RQ = position->pins.by_RQ;
      uo_bitboard pins_to_own_K = position->pins.by_BQ | position->pins.by_RQ;


      if (uo_position_is_check(position))
      { // Position is a check

        uo_bitboard enemy_checks = position->stack->checks;

        // Double check, only king moves are legal
        if (uo_popcnt(enemy_checks) == 2) return false;

        uo_square square_enemy_checker = uo_tzcnt(enemy_checks);

        // Let's first consider captures

        uo_bitboard attack_checker_diagonals = uo_bitboard_attacks_B(square_enemy_checker, occupied);
        uo_bitboard attack_checker_lines = uo_bitboard_attacks_R(square_enemy_checker, occupied);

        switch (piece)
        {
          case uo_piece__P: {
            // Captures by pawns
            uo_bitboard attack_checker_P = uo_andn(pins_to_own_K, bitboard_from & uo_bitboard_attacks_P(square_enemy_checker, uo_black));
            if (attack_checker_P & bitboard_to) return true;

            // Let's also check for en passant captures
            uo_bitboard checks_by_P = enemy_checks & enemy_P;
            uint8_t enpassant_file = uo_position_flags_enpassant_file(position->flags);

            if (checks_by_P && enpassant_file == uo_square_file(square_to))
            {
              uo_bitboard non_pinned_P = uo_andn(pins_to_own_K, bitboard_from);

              if (enpassant_file > 1 && (non_pinned_P & (checks_by_P >> 1)))
              {
                return true;
              }

              if (enpassant_file < 8 && (non_pinned_P & (checks_by_P << 1)))
              {
                return true;
              }
            }

            break;
          }
          case uo_piece__N: {
            // Captures by knights
            uo_bitboard capture_N = uo_bitboard_attacks_N(square_own_K) & attack_checker_diagonals;
            if (capture_N & bitboard_to) return true;
            break;
          }
          case uo_piece__B: {
            // Captures by bishops
            uo_bitboard attack_checker_B = uo_andn(pins_to_own_K, bitboard_from & attack_checker_diagonals);
            if (attack_checker_B & bitboard_to) return true;
            break;
          }
          case uo_piece__R: {
            // Captures by rooks
            uo_bitboard attack_checker_R = uo_andn(pins_to_own_K, bitboard_from & attack_checker_lines);
            if (attack_checker_R & bitboard_to) return true;
            break;
          }
          case uo_piece__Q: {
            // Captures by queens
            uo_bitboard attack_checker_Q = uo_andn(pins_to_own_K, bitboard_from & (attack_checker_diagonals | attack_checker_lines));
            if (attack_checker_Q & bitboard_to) return true;
            break;
          }
        }

        // All possible captures considered. Next let's consider blocking moves

        uint8_t file_enemy_checker = uo_square_file(square_enemy_checker);
        uint8_t rank_enemy_checker = uo_square_rank(square_enemy_checker);

        uo_square squares_between[6];
        size_t i = uo_squares_between(square_own_K, square_enemy_checker, squares_between);

        // TODO: replace with checkmask
        while (i--)
        {
          uo_square square_between = squares_between[i];
          if (square_between != square_to) continue;

          uo_bitboard bitboard_square_between = uo_square_bitboard(square_between);

          uo_bitboard block_diagonals = uo_bitboard_moves_B(square_between, mask_enemy, mask_own);
          uo_bitboard block_lines = uo_bitboard_moves_R(square_between, mask_enemy, mask_own);

          switch (piece)
          {
            case uo_piece__P: {
              uo_bitboard block_P = bitboard_from & uo_square_bitboard(square_between - 8);

              if ((bitboard_square_between & uo_bitboard_rank_fourth) && (uo_bitboard_double_push_P(uo_square_bitboard(square_between - 16), empty)))
              {
                block_P |= bitboard_from & uo_square_bitboard(square_between - 16);
              }

              block_P = uo_andn(pins_to_own_K, block_P);
              return block_P != 0;
            }

            case uo_piece__N: {
              uo_bitboard block_N = uo_andn(pins_to_own_K, bitboard_from & uo_bitboard_moves_N(square_between, mask_enemy, mask_own));
              return block_N != 0;
            }

            case uo_piece__B: {
              uo_bitboard block_B = uo_andn(pins_to_own_K, bitboard_from & block_diagonals);
              return block_B != 0;
            }

            case uo_piece__R: {
              uo_bitboard block_R = uo_andn(pins_to_own_K, bitboard_from & block_lines);
              return block_R != 0;
            }

            case uo_piece__Q: {
              uo_bitboard block_Q = uo_andn(pins_to_own_K, bitboard_from & (block_diagonals | block_lines));
              return block_Q != 0;
            }
          }

          return false;
        }
      }

      if (pins_to_own_K & bitboard_from)
      { // Piece is pinned to the king
        switch (piece)
        {
          case uo_piece__P: {
            // Moves for pinned pawns
            uo_bitboard pinned_file_P = bitboard_from & pins_to_own_K_by_RQ & uo_square_bitboard_file[square_own_K];

            // Single pawn push
            uo_bitboard pinned_single_push_P = uo_bitboard_single_push_P(pinned_file_P, empty);
            if (pinned_single_push_P & bitboard_to) return true;

            // Double pawn push
            uo_bitboard pinned_double_push_P = uo_bitboard_double_push_P(pinned_file_P, empty);
            if (pinned_double_push_P & bitboard_to) return true;

            // Captures by pinned pawns
            uo_bitboard pinned_diag_P = bitboard_from & pins_to_own_K_by_BQ;

            // Captures to right
            uo_bitboard pinned_captures_right_P = uo_bitboard_captures_right_P(pinned_diag_P, mask_enemy) & pins_to_own_K_by_BQ;
            if (pinned_captures_right_P & bitboard_to) return true;

            // Captures to left
            uo_bitboard pinned_captures_left_P = uo_bitboard_captures_left_P(pinned_diag_P, mask_enemy) & pins_to_own_K_by_BQ;
            if (pinned_captures_left_P & bitboard_to) return true;

            uint8_t enpassant_file = uo_position_flags_enpassant_file(position->flags);
            if (!enpassant_file) return false;

            uo_bitboard bitboard_enpassant_file = uo_bitboard_file[enpassant_file - 1];

            if ((bitboard_enpassant_file & bitboard_to)
              && (bitboard_enpassant_file & bitboard_from) == 0
              && (uo_bitboard_rank_fifth & bitboard_from)
              && (bitboard_to & pins_to_own_K_by_BQ))
            {
              return true;
            }

            return false;
          }

          case uo_piece__N:
            return false;

          case uo_piece__B:
            return (uo_bitboard_moves_B(square_from, mask_own, mask_enemy) & pins_to_own_K_by_BQ & bitboard_to) != 0;

          case uo_piece__R:
            return (uo_bitboard_moves_R(square_from, mask_own, mask_enemy) & pins_to_own_K_by_RQ & bitboard_to) != 0;

          case uo_piece__Q:
            return ((uo_bitboard_moves_B(square_from, mask_own, mask_enemy) & pins_to_own_K_by_BQ)
              | (uo_bitboard_moves_R(square_from, mask_own, mask_enemy) & pins_to_own_K_by_RQ) & bitboard_to) != 0;
        }
      }

      // Position is not a check and piece is not pinned to the king
      switch (piece)
      {
        case uo_piece__P: {
          if (uo_bitboard_single_push_P(bitboard_from, empty) & bitboard_to) return true;
          if (uo_bitboard_double_push_P(bitboard_from, empty) & bitboard_to) return true;
          if (uo_bitboard_captures_right_P(bitboard_from, mask_enemy) & bitboard_to) return true;
          if (uo_bitboard_captures_left_P(bitboard_from, mask_enemy) & bitboard_to) return true;

          uint8_t enpassant_file = uo_position_flags_enpassant_file(position->flags);
          if (!enpassant_file) return false;

          uo_bitboard bitboard_enpassant_file = uo_bitboard_file[enpassant_file - 1];

          if ((bitboard_enpassant_file & bitboard_to)
            && (bitboard_enpassant_file & bitboard_from) == 0
            && (uo_bitboard_rank_fifth & bitboard_from))
          {
            return true;
          }

          return false;
        }

        case uo_piece__N: {
          uo_bitboard moves_N = uo_bitboard_moves_N(square_from, mask_own, mask_enemy);
          return (moves_N & bitboard_to) != 0;
        }

        case uo_piece__B: {
          uo_bitboard moves_B = uo_bitboard_moves_B(square_from, mask_own, mask_enemy);
          return (moves_B & bitboard_to) != 0;
        }

        case uo_piece__R: {
          uo_bitboard moves_R = uo_bitboard_moves_R(square_from, mask_own, mask_enemy);
          return (moves_R & bitboard_to) != 0;
        }

        case uo_piece__Q: {
          uo_bitboard moves_Q = uo_bitboard_moves_Q(square_from, mask_own, mask_enemy);
          return (moves_Q & bitboard_to) != 0;
        }
      }
    }

    return false;
  }

  static inline bool uo_position_is_killer_move(const uo_position *position, uo_move move)
  {
    uo_move *killers = position->stack->search.killers;

    return !uo_move_is_capture(move) && (move == killers[0] || move == killers[1]);
  }

  static inline size_t uo_position_move_history_heuristic_index(const uo_position *position, uo_move move)
  {
    uo_square square_from = uo_move_square_from(move);
    uo_piece piece = position->board[square_from];
    size_t piece_type = (uo_piece_type(piece) >> 1) - 1;
    size_t square_to = uo_move_square_to(move);
    size_t color = uo_color(position->flags);
    size_t index = color * 6 * 64 + piece_type * 64 + square_to;
    assert(index < (sizeof position->bftable / sizeof * position->bftable));
    return index;
  }

  static inline int16_t uo_position_calculate_non_tactical_move_score(uo_position *position, uo_move move, uo_move_cache *move_cache)
  {
    int16_t move_score = 0;

    int16_t see = uo_position_move_see(position, move, move_cache);
    move_score += see;

    bool is_killer_move = uo_position_is_killer_move(position, move);
    move_score += is_killer_move * 1000;

    // relative history heuristic
    size_t index = uo_position_move_history_heuristic_index(position, move);
    uint32_t hhscore = position->hhtable[index] << 4;
    uint32_t bfscore = position->bftable[index] + 1;

    move_score += hhscore / bfscore;

    return move_score;
  }

  static inline int16_t uo_position_calculate_tactical_move_score(uo_position *position, uo_move move, uo_move_cache *move_cache)
  {
    int16_t move_score = 0;

    uo_bitboard checks = uo_position_move_checks(position, move, move_cache);
    move_score += uo_popcnt(checks) * 100;

    int16_t see = uo_position_move_see(position, move, move_cache);
    move_score += see;

    uo_square move_type = uo_move_get_type(move);

    if (move_type == uo_move_type__enpassant)
    {
      move_score += 2050;
    }
    else if (move_type & uo_move_type__promo)
    {
      move_score += 2500 + move_type;
    }
    else if (move_type & uo_move_type__x)
    {
      move_score += 2000;
    }

    return move_score;
  }

  static inline int uo_position_partition_moves(uo_position *position, uo_move *movelist, int lo, int hi)
  {
    int16_t *move_scores = position->movelist.move_scores + (movelist - position->movelist.head);
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

  static inline size_t uo_position_sort_skipped_moves(uo_position *position, uo_move *movelist, int lo, int hi)
  {
    int16_t *move_scores = position->movelist.move_scores + (movelist - position->movelist.head);
    size_t skipped_move_count = 0;
    uo_move temp_move = 0;
    uo_move *tail = position->movelist.head + position->stack->move_count;
    uo_move *head = movelist + lo;

    for (int i = lo; i <= hi; ++i)
    {
      if (move_scores[i] == uo_move_score_skip)
      {
        tail[skipped_move_count++] = movelist[i];
      }
      else
      {
        *head++ = movelist[i];
      }
    }

    if (skipped_move_count)
    {
      position->stack->skipped_move_count = skipped_move_count;
      memmove(head, tail, skipped_move_count * sizeof(uo_move));
    }

    return skipped_move_count;
  }

  static inline void uo_position_quicksort_moves(uo_position *position, uo_move *movelist, int lo, int hi)
  {
    if (lo >= hi)
    {
      return;
    }

    hi = uo_position_partition_moves(position, movelist, lo, hi);
    int p = uo_position_partition_moves(position, movelist, lo, hi);
    uo_position_quicksort_moves(position, movelist, lo, p - 1);
    uo_position_quicksort_moves(position, movelist, p + 1, hi);
  }

  static inline void uo_position_sort_tactical_moves(uo_position *position, uo_move_cache *move_cache)
  {
    if (position->stack->moves_sorted) return;

    uo_move *movelist = position->movelist.head;
    size_t tactical_move_count = position->stack->tactical_move_count;
    size_t i = 0;

    while (i < tactical_move_count)
    {
      uo_move move = movelist[i];
      int16_t score = uo_position_calculate_tactical_move_score(position, move, move_cache);
      position->movelist.move_scores[i++] = score;
    }

    uo_position_quicksort_moves(position, movelist, 0, tactical_move_count - 1);
  }

  static inline void uo_position_sort_non_tactical_moves(uo_position *position, uo_move_cache *move_cache)
  {
    if (position->stack->moves_sorted) return;

    uo_move *movelist = position->movelist.head;
    size_t tactical_move_count = position->stack->tactical_move_count;
    size_t move_count = position->stack->move_count;
    size_t i = tactical_move_count;

    while (i < move_count)
    {
      uo_move move = movelist[i];
      int16_t score = uo_position_calculate_non_tactical_move_score(position, move, move_cache);
      position->movelist.move_scores[i++] = score;
    }

    uo_position_quicksort_moves(position, movelist, tactical_move_count, move_count - 1);
  }

  static inline void uo_position_sort_moves(uo_position *position, uo_move bestmove, uo_move_cache *move_cache)
  {
    if (position->stack->moves_sorted) return;

    uo_move *movelist = position->movelist.head;
    size_t move_count = position->stack->move_count;
    uo_position_sort_tactical_moves(position, move_cache);
    uo_position_sort_non_tactical_moves(position, move_cache);
    move_count -= uo_position_sort_skipped_moves(position, movelist, 0, move_count - 1);

    // set bestmove as first and shift other moves by one position
    if (bestmove && movelist[0] != bestmove)
    {
      for (size_t i = 1; i < move_count; ++i)
      {
        if (movelist[i] == bestmove)
        {
          while (--i)
          {
            movelist[i + 1] = movelist[i];
          }

          movelist[0] = bestmove;
          break;
        }
      }
    }

    position->stack->moves_sorted = true;
  }

  static inline bool uo_position_is_quiescent(uo_position *position)
  {
    if (!position->stack->moves_generated)
    {
      uo_position_generate_moves(position);
    }

    if (uo_position_is_check(position))
    {
      return false;
    }

    size_t move_count = position->stack->move_count;

    for (size_t i = 0; i < move_count; ++i)
    {
      uo_move move = position->movelist.head[i];

      bool is_tactical_move = uo_move_is_promotion(move)
        || (uo_move_is_capture(move) && uo_position_move_see(position, move, NULL) >= 0);

      if (is_tactical_move)
      {
        return false;
      }
    }

    return true;
  }

  static inline void uo_position_update_killers(uo_position *position, uo_move move)
  {
    if (!uo_move_is_capture(move) && position->stack->search.killers[0] != move)
    {
      position->stack->search.killers[1] = position->stack->search.killers[0];
      position->stack->search.killers[0] = move;
    }
  }

  static inline void uo_position_update_history_heuristic(uo_position *position, uo_move move, size_t depth)
  {
    size_t index = uo_position_move_history_heuristic_index(position, move);
    position->hhtable[index] += 1 << depth;
  }

  static inline void uo_position_update_butterfly_heuristic(uo_position *position, uo_move move)
  {
    size_t index = uo_position_move_history_heuristic_index(position, move);
    ++position->bftable[index];
  }

  uo_move uo_position_parse_move(const uo_position *position, char str[5]);

  uo_move uo_position_parse_pgn_move(uo_position *position, char *png);

  size_t uo_position_print_move(const uo_position *position, uo_move move, char str[6]);

  size_t uo_position_perft(uo_position *position, size_t depth);

  uo_position *uo_position_randomize(uo_position *position, const char *pieces /* e.g. KQRPPvKRRBNP */);

#ifdef __cplusplus
  }
#endif

#endif
