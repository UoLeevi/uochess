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
#include "uo_evaluation.h"
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
    int16_t see_ubound;
    int16_t see_lbound;
    int16_t static_eval;
    uo_evaluation_info eval_info;

    struct
    {
      bool checks;
      bool see;
      bool see_ubound;
      bool see_lbound;
      bool static_eval;
      bool eval_info;
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
    uo_evaluation_info eval_info;
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

  static inline uo_move_cache *uo_move_cache_get(uo_move_cache move_cache[0x1000], uint64_t key, uo_move move)
  {
    if (!move_cache) return NULL;
    uo_move_cache *entry = &move_cache[(move ^ key) & 0xFFF];

    if (entry->key != key || entry->move != move)
    {
      *entry = (uo_move_cache){
        .key = key,
        .move = move
      };
    }

    return entry;
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

  static inline uint64_t uo_position_move_key(const uo_position *position, uo_move move, uo_position_flags *flags)
  {
    uo_square square_from = uo_move_square_from(move);
    uo_square square_to = uo_move_square_to(move);
    const uo_piece *board = position->board;
    uo_piece piece = board[square_from];
    uo_piece piece_captured = board[square_to];
    uint8_t color = uo_color(position->flags);
    uint8_t flip_if_black = color == uo_black ? 56 : 0;

    uint64_t key = position->key;
    uo_position_flags flags_next = position->flags ^ 1;

    uint8_t enpassant_file = uo_position_flags_enpassant_file(flags_next);
    flags_next = uo_position_flags_update_enpassant_file(flags_next, 0);

    uint8_t rule50 = uo_position_flags_rule50(flags_next);
    flags_next = uo_position_flags_update_rule50(flags_next, rule50 + 1);

    uo_piece piece_colored = piece ^ color;
    key ^= uo_zobkey(piece_colored, square_from ^ flip_if_black)
      ^ uo_zobkey(piece_colored, square_to ^ flip_if_black);

    switch (piece)
    {
      case uo_piece__P:
        flags_next = uo_position_flags_update_rule50(flags_next, 0);
        break;

      case uo_piece__K:
        flags_next = uo_position_flags_update_castling_own(flags_next, 0);
        break;

      case uo_piece__R:
        switch (square_from)
        {
          case uo_square__a1:
            flags_next = uo_position_flags_update_castling_Q(flags_next, false);
            break;

          case uo_square__h1:
            flags_next = uo_position_flags_update_castling_K(flags_next, false);
            break;
        }
    }

    switch (uo_move_get_type(move))
    {
      case uo_move_type__P_double_push: {
        // en passant file

        uo_bitboard bitboard_to = uo_square_bitboard(square_to);
        uo_bitboard mask_enemy = position->enemy;
        uo_bitboard enemy_P = mask_enemy & position->P;

        uo_bitboard adjecent_enemy_pawn = uo_square_bitboard_adjecent_files[square_to] & uo_bitboard_rank_fourth & enemy_P;
        if (!adjecent_enemy_pawn) break;

        uo_bitboard enemy_K = mask_enemy & position->K;
        uo_square square_enemy_K = uo_tzcnt(enemy_K);
        uint8_t rank_enemy_K = uo_square_rank(square_enemy_K);

        if (rank_enemy_K == 3 && uo_popcnt(adjecent_enemy_pawn) == 1)
        {
          uo_bitboard mask_own = position->own;
          uo_bitboard own_R = mask_own & position->R;
          uo_bitboard own_Q = mask_own & position->Q;
          uo_bitboard bitboard_rank_enemy_K = uo_bitboard_rank[rank_enemy_K];
          uo_bitboard occupied = uo_andn(uo_square_bitboard_file[square_to], position->own | position->enemy);
          uo_bitboard rank_pins_to_enemy_K = uo_bitboard_pins_R(square_enemy_K, occupied, own_R | own_Q) & bitboard_rank_enemy_K;

          if (!(rank_pins_to_enemy_K & adjecent_enemy_pawn))
          {
            uint8_t enpassant_file = uo_square_file(square_to);
            flags_next = uo_position_flags_update_enpassant_file(flags_next, enpassant_file + 1);
            key ^= uo_zobrist_enpassant_file[enpassant_file];
          }
        }
        else
        {
          uint8_t enpassant_file = uo_square_file(square_to);
          flags_next = uo_position_flags_update_enpassant_file(flags_next, enpassant_file + 1);
          key ^= uo_zobrist_enpassant_file[enpassant_file];
        }

        break;
      }

      case uo_move_type__enpassant: {
        uo_piece piece_captured_colored = uo_piece__p ^ color;
        uo_square square_piece_captured = square_to - 8;
        key ^= uo_zobkey(piece_captured_colored, square_piece_captured ^ flip_if_black);
        break;
      }

      case uo_move_type__OO: {
        uo_piece piece_R_colored = uo_piece__R ^ color;
        key ^= uo_zobkey(piece_R_colored, uo_square__h1 ^ flip_if_black)
          ^ uo_zobkey(piece_R_colored, uo_square__f1 ^ flip_if_black);
        break;
      }

      case uo_move_type__OOO: {
        uo_piece piece_R_colored = uo_piece__R ^ color;
        key ^= uo_zobkey(piece_R_colored, uo_square__a1 ^ flip_if_black)
          ^ uo_zobkey(piece_R_colored, uo_square__d1 ^ flip_if_black);

        break;
      }

      case uo_move_type__promo_N:
      case uo_move_type__promo_Nx: {
        uo_piece piece_promo_colored = uo_piece__N ^ color;
        key ^= uo_zobkey(piece_colored, square_to ^ flip_if_black)
          ^ uo_zobkey(piece_promo_colored, square_to ^ flip_if_black);
        break;
      }

      case uo_move_type__promo_B:
      case uo_move_type__promo_Bx: {
        uo_piece piece_promo_colored = uo_piece__B ^ color;
        key ^= uo_zobkey(piece_colored, square_to ^ flip_if_black)
          ^ uo_zobkey(piece_promo_colored, square_to ^ flip_if_black);
        break;
      }

      case uo_move_type__promo_R:
      case uo_move_type__promo_Rx: {
        uo_piece piece_promo_colored = uo_piece__R ^ color;
        key ^= uo_zobkey(piece_colored, square_to ^ flip_if_black)
          ^ uo_zobkey(piece_promo_colored, square_to ^ flip_if_black);
        break;
      }

      case uo_move_type__promo_Q:
      case uo_move_type__promo_Qx: {
        uo_piece piece_promo_colored = uo_piece__Q ^ color;
        key ^= uo_zobkey(piece_colored, square_to ^ flip_if_black)
          ^ uo_zobkey(piece_promo_colored, square_to ^ flip_if_black);
        break;
      }
    }

    if (piece_captured > 1)
    {
      flags_next = uo_position_flags_update_rule50(flags_next, 0);

      uo_piece piece_captured_colored = piece_captured ^ color;
      key ^= uo_zobkey(piece_captured_colored, square_to ^ flip_if_black);

      if (piece_captured == uo_piece__r)
      {
        switch (square_to)
        {
          case uo_square__a8:
            flags_next = uo_position_flags_update_castling_q(flags_next, false);
            break;

          case uo_square__h8:
            flags_next = uo_position_flags_update_castling_k(flags_next, false);
            break;
        }
      }
    }

    uint8_t enpassant_file_prev = uo_position_flags_enpassant_file(position->flags);
    if (enpassant_file_prev)
    {
      key ^= uo_zobrist_enpassant_file[enpassant_file_prev - 1];
    }

    flags_next = uo_position_flags_flip_castling(flags_next);
    uint8_t castling_prev = uo_position_flags_castling(position->flags);
    uint8_t castling_next = uo_position_flags_castling(flags_next);
    key ^= uo_zobrist_castling[castling_prev] ^ uo_zobrist_castling[castling_next];

    key ^= uo_zobrist_side_to_move;

    if (flags) *flags = flags_next;

    return key;
  }

  void uo_position_make_move(uo_position *position, uo_move move, uint64_t key, uo_position_flags flags);

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

  static inline int16_t uo_position_evaluate(const uo_position *position, uo_evaluation_info *info)
  {
    int16_t score = uo_score_tempo;

    // opening & middle game
    int32_t score_mg = 0;

    // endgame
    int32_t score_eg = 0;

    uo_bitboard mask_own = position->own;
    uo_bitboard mask_enemy = position->enemy;
    uo_bitboard occupied = mask_own | mask_enemy;
    uo_bitboard empty = ~occupied;

    uo_bitboard own_P = mask_own & position->P;
    uo_bitboard own_N = mask_own & position->N;
    uo_bitboard own_B = mask_own & position->B;
    uo_bitboard own_R = mask_own & position->R;
    uo_bitboard own_Q = mask_own & position->Q;
    uo_bitboard own_K = mask_own & position->K;

    uo_bitboard enemy_P = mask_enemy & position->P;
    uo_bitboard enemy_N = mask_enemy & position->N;
    uo_bitboard enemy_B = mask_enemy & position->B;
    uo_bitboard enemy_R = mask_enemy & position->R;
    uo_bitboard enemy_Q = mask_enemy & position->Q;
    uo_bitboard enemy_K = mask_enemy & position->K;

    uo_square square_own_K = uo_tzcnt(own_K);
    uo_bitboard attacks_own_K = uo_bitboard_attacks_K(square_own_K);
    uo_bitboard zone_own_K = attacks_own_K | own_K | attacks_own_K << 8;
    int attack_units_own = 0;

    uo_square square_enemy_K = uo_tzcnt(enemy_K);
    uo_bitboard attacks_enemy_K = uo_bitboard_attacks_K(square_enemy_K);
    uo_bitboard zone_enemy_K = attacks_enemy_K | enemy_K | attacks_enemy_K >> 8;
    int attack_units_enemy = 0;

    uo_bitboard pushes_own_P = uo_bitboard_single_push_P(own_P, empty) | uo_bitboard_double_push_P(own_P, empty);
    uo_bitboard pushes_enemy_P = uo_bitboard_single_push_enemy_P(enemy_P, empty) | uo_bitboard_double_push_enemy_P(enemy_P, empty);

    uo_bitboard attacks_left_own_P = uo_bitboard_attacks_left_P(own_P);
    uo_bitboard attacks_right_own_P = uo_bitboard_attacks_right_P(own_P);
    uo_bitboard attacks_own_P = attacks_left_own_P | attacks_right_own_P;

    uo_bitboard attacks_left_enemy_P = uo_bitboard_attacks_left_enemy_P(enemy_P);
    uo_bitboard attacks_right_enemy_P = uo_bitboard_attacks_right_enemy_P(enemy_P);
    uo_bitboard attacks_enemy_P = attacks_left_enemy_P | attacks_right_enemy_P;

    uo_bitboard potential_attacks_own_P = attacks_own_P | uo_bitboard_attacks_left_P(pushes_own_P) | uo_bitboard_attacks_right_P(pushes_own_P);
    uo_bitboard potential_attacks_enemy_P = attacks_enemy_P | uo_bitboard_attacks_left_enemy_P(pushes_enemy_P) | uo_bitboard_attacks_right_enemy_P(pushes_enemy_P);

    int16_t castling_right_own_OO = uo_position_flags_castling_OO(position->flags);
    int16_t castling_right_own_OOO = uo_position_flags_castling_OOO(position->flags);
    int16_t castling_right_enemy_OO = uo_position_flags_castling_enemy_OO(position->flags);
    int16_t castling_right_enemy_OOO = uo_position_flags_castling_enemy_OOO(position->flags);

    // pawns

    int count_own_P = uo_popcnt(own_P);
    score += (uo_score_P + uo_score_extra_pawn) * count_own_P;
    int count_enemy_P = uo_popcnt(enemy_P);
    score -= (uo_score_P + uo_score_extra_pawn) * count_enemy_P;

    // pawn mobility

    int mobility_own_P = uo_popcnt(pushes_own_P) +
      uo_popcnt(attacks_left_own_P & mask_enemy) +
      uo_popcnt(attacks_right_own_P & mask_enemy);
    score += !mobility_own_P * uo_score_zero_mobility_P;
    score += uo_score_mul_ln(uo_score_mobility_P, mobility_own_P * mobility_own_P);

    int mobility_enemy_P = uo_popcnt(pushes_enemy_P) +
      uo_popcnt(attacks_left_enemy_P & mask_own) +
      uo_popcnt(attacks_right_enemy_P & mask_own);
    score -= !mobility_enemy_P * uo_score_zero_mobility_P;
    score -= uo_score_mul_ln(uo_score_mobility_P, mobility_enemy_P * mobility_enemy_P);

    uo_bitboard temp;

    // bitboards marking attacked squares by pieces of the same type
    uo_bitboard attacks_total_own_N = 0;
    uo_bitboard attacks_total_enemy_N = 0;
    uo_bitboard attacks_total_own_B = 0;
    uo_bitboard attacks_total_enemy_B = 0;
    uo_bitboard attacks_total_own_R = 0;
    uo_bitboard attacks_total_enemy_R = 0;
    uo_bitboard attacks_total_own_Q = 0;
    uo_bitboard attacks_total_enemy_Q = 0;

    uo_bitboard attacks_lower_value_own = attacks_own_P;
    uo_bitboard attacks_lower_value_enemy = attacks_enemy_P;

    uo_bitboard supported_contact_check_by_R = 0;
    uo_bitboard supported_contact_check_by_Q = 0;

    // knights
    temp = own_N;
    score += uo_score_N_square_attackable_by_P * (int16_t)uo_popcnt(own_N & potential_attacks_enemy_P);
    while (temp)
    {
      uo_square square_own_N = uo_bitboard_next_square(&temp);
      uo_bitboard attacks_own_N = uo_bitboard_attacks_N(square_own_N);
      attacks_total_own_N |= attacks_own_N;

      int mobility_own_N = uo_popcnt(uo_andn(mask_own | attacks_lower_value_enemy, attacks_own_N));
      score += !mobility_own_N * uo_score_zero_mobility_N;
      score += uo_score_mul_ln(uo_score_mobility_N, mobility_own_N * mobility_own_N);

      score += uo_score_N + uo_score_extra_piece;

      score_mg += uo_score_adjust_piece_square_table_score(mg_table_N[square_own_N]);
      score_eg += uo_score_adjust_piece_square_table_score(eg_table_N[square_own_N]);

      attack_units_own += uo_popcnt(attacks_own_N & zone_enemy_K) * uo_attack_unit_N;
    }

    temp = enemy_N;
    score -= uo_score_N_square_attackable_by_P * (int16_t)uo_popcnt(enemy_N & potential_attacks_own_P);
    while (temp)
    {
      uo_square square_enemy_N = uo_bitboard_next_square(&temp);
      uo_bitboard attacks_enemy_N = uo_bitboard_attacks_N(square_enemy_N);
      attacks_total_enemy_N |= attacks_enemy_N;

      int mobility_enemy_N = uo_popcnt(uo_andn(mask_enemy | attacks_lower_value_own, attacks_enemy_N));
      score -= !mobility_enemy_N * uo_score_zero_mobility_N;
      score -= uo_score_mul_ln(uo_score_mobility_N, mobility_enemy_N * mobility_enemy_N);

      score -= uo_score_N + uo_score_extra_piece;

      score_mg -= uo_score_adjust_piece_square_table_score(mg_table_N[square_enemy_N ^ 56]);
      score_eg -= uo_score_adjust_piece_square_table_score(eg_table_N[square_enemy_N ^ 56]);

      attack_units_enemy += uo_popcnt(attacks_enemy_N & zone_own_K) * uo_attack_unit_N;
    }
    attacks_lower_value_own |= attacks_total_own_N;
    attacks_lower_value_enemy |= attacks_total_enemy_N;


    // bishops
    temp = own_B;
    score += uo_score_B_square_attackable_by_P * (int16_t)uo_popcnt(own_B & potential_attacks_enemy_P);
    while (temp)
    {
      uo_square square_own_B = uo_bitboard_next_square(&temp);
      uo_bitboard attacks_own_B = uo_bitboard_attacks_B(square_own_B, occupied);
      attacks_total_own_B |= attacks_own_B;

      int mobility_own_B = uo_popcnt(uo_andn(mask_own | attacks_lower_value_enemy, attacks_own_B));
      score += !mobility_own_B * uo_score_zero_mobility_B;
      score += uo_score_mul_ln(uo_score_mobility_B, mobility_own_B * mobility_own_B);

      score += uo_score_B + uo_score_extra_piece;

      score_mg += uo_score_adjust_piece_square_table_score(mg_table_B[square_own_B]);
      score_eg += uo_score_adjust_piece_square_table_score(eg_table_B[square_own_B]);

      attack_units_own += uo_popcnt(attacks_own_B & zone_enemy_K) * uo_attack_unit_B;
    }
    temp = enemy_B;
    score -= uo_score_B_square_attackable_by_P * (int16_t)uo_popcnt(enemy_B & potential_attacks_own_P);
    while (temp)
    {
      uo_square square_enemy_B = uo_bitboard_next_square(&temp);
      uo_bitboard attacks_enemy_B = uo_bitboard_attacks_B(square_enemy_B, occupied);
      attacks_total_enemy_B |= attacks_enemy_B;

      int mobility_enemy_B = uo_popcnt(uo_andn(mask_enemy | attacks_lower_value_own, attacks_enemy_B));
      score -= !mobility_enemy_B * uo_score_zero_mobility_B;
      score -= uo_score_mul_ln(uo_score_mobility_B, mobility_enemy_B * mobility_enemy_B);

      score -= uo_score_B + uo_score_extra_piece;

      score_mg -= uo_score_adjust_piece_square_table_score(mg_table_B[square_enemy_B ^ 56]);
      score_eg -= uo_score_adjust_piece_square_table_score(eg_table_B[square_enemy_B ^ 56]);

      attack_units_enemy += uo_popcnt(attacks_enemy_B & zone_own_K) * uo_attack_unit_B;
    }
    attacks_lower_value_own |= attacks_total_own_B;
    attacks_lower_value_enemy |= attacks_total_enemy_B;

    // rooks
    temp = own_R;
    score += uo_score_R_square_attackable_by_P * (int16_t)uo_popcnt(own_R & potential_attacks_enemy_P);
    while (temp)
    {
      uo_square square_own_R = uo_bitboard_next_square(&temp);
      uo_bitboard attacks_own_R = uo_bitboard_attacks_R(square_own_R, occupied);

      attacks_total_own_R |= attacks_own_R;

      int mobility_own_R = uo_popcnt(uo_andn(mask_own | attacks_lower_value_enemy, attacks_own_R));
      score += !mobility_own_R * uo_score_zero_mobility_R;
      score += uo_score_mul_ln(uo_score_mobility_R, mobility_own_R * mobility_own_R);

      score += uo_score_R + uo_score_extra_piece;

      score_mg += uo_score_adjust_piece_square_table_score(mg_table_R[square_own_R]);
      score_eg += uo_score_adjust_piece_square_table_score(eg_table_R[square_own_R]);

      // connected rooks
      if (attacks_own_R & own_R) score += uo_score_connected_rooks / 2;

      attack_units_own += uo_popcnt(attacks_own_R & zone_enemy_K) * uo_attack_unit_R;
    }
    temp = enemy_R;
    score -= uo_score_R_square_attackable_by_P * (int16_t)uo_popcnt(enemy_R & potential_attacks_own_P);
    while (temp)
    {
      uo_square square_enemy_R = uo_bitboard_next_square(&temp);
      uo_bitboard attacks_enemy_R = uo_bitboard_attacks_R(square_enemy_R, occupied);
      attacks_total_enemy_R |= attacks_enemy_R;

      int mobility_enemy_R = uo_popcnt(uo_andn(mask_enemy | attacks_lower_value_own, attacks_enemy_R));
      score -= !mobility_enemy_R * uo_score_zero_mobility_R;
      score -= uo_score_mul_ln(uo_score_mobility_R, mobility_enemy_R * mobility_enemy_R);

      score -= uo_score_R + uo_score_extra_piece;

      score_mg -= uo_score_adjust_piece_square_table_score(mg_table_R[square_enemy_R ^ 56]);
      score_eg -= uo_score_adjust_piece_square_table_score(eg_table_R[square_enemy_R ^ 56]);

      // connected rooks
      if (attacks_enemy_R & enemy_R) score -= uo_score_connected_rooks / 2;

      attack_units_enemy += uo_popcnt(attacks_enemy_R & zone_own_K) * uo_attack_unit_R;
    }
    attacks_lower_value_own |= attacks_total_own_R;
    attacks_lower_value_enemy |= attacks_total_enemy_R;

    // queens
    temp = own_Q;
    score += uo_score_Q_square_attackable_by_P * (int16_t)uo_popcnt(own_Q & potential_attacks_enemy_P);
    while (temp)
    {
      uo_square square_own_Q = uo_bitboard_next_square(&temp);
      uo_bitboard attacks_own_Q = uo_bitboard_attacks_Q(square_own_Q, occupied);

      attacks_total_own_Q |= attacks_own_Q;

      int mobility_own_Q = uo_popcnt(uo_andn(mask_own | attacks_lower_value_enemy, attacks_own_Q));
      score += !mobility_own_Q * uo_score_zero_mobility_Q;
      score += uo_score_mul_ln(uo_score_mobility_Q, mobility_own_Q * mobility_own_Q);

      score += uo_score_Q + uo_score_extra_piece;

      score_mg += uo_score_adjust_piece_square_table_score(mg_table_Q[square_own_Q]);
      score_eg += uo_score_adjust_piece_square_table_score(eg_table_Q[square_own_Q]);

      attack_units_own += uo_popcnt(attacks_own_Q & zone_enemy_K) * uo_attack_unit_Q;
    }
    temp = enemy_Q;
    score -= uo_score_Q_square_attackable_by_P * (int16_t)uo_popcnt(enemy_Q & potential_attacks_own_P);
    while (temp)
    {
      uo_square square_enemy_Q = uo_bitboard_next_square(&temp);
      uo_bitboard attacks_enemy_Q = uo_bitboard_attacks_Q(square_enemy_Q, occupied);
      attacks_total_enemy_Q |= attacks_enemy_Q;

      int mobility_enemy_Q = uo_popcnt(uo_andn(mask_enemy | attacks_lower_value_own, attacks_enemy_Q));
      score -= !mobility_enemy_Q * uo_score_zero_mobility_Q;
      score -= uo_score_mul_ln(uo_score_mobility_Q, mobility_enemy_Q * mobility_enemy_Q);

      score -= uo_score_Q + uo_score_extra_piece;

      score_mg -= uo_score_adjust_piece_square_table_score(mg_table_Q[square_enemy_Q ^ 56]);
      score_eg -= uo_score_adjust_piece_square_table_score(eg_table_Q[square_enemy_Q ^ 56]);

      attack_units_enemy += uo_popcnt(attacks_enemy_Q & zone_own_K) * uo_attack_unit_Q;
    }
    attacks_lower_value_own |= attacks_total_own_Q;
    attacks_lower_value_enemy |= attacks_total_enemy_Q;

    uo_bitboard attacks_own = attacks_own_K | attacks_lower_value_own;
    uo_bitboard attacks_enemy = attacks_enemy_K | attacks_lower_value_enemy;

    // king safety

    uo_bitboard undefended_zone_own_K = uo_andn(attacks_lower_value_own, zone_own_K);
    uo_bitboard undefended_zone_enemy_K = uo_andn(attacks_lower_value_enemy, zone_enemy_K);

    supported_contact_check_by_R |= own_R & attacks_own & (attacks_enemy_K & uo_bitboard_attacks_R(square_enemy_K, 0));
    attack_units_own += uo_popcnt(supported_contact_check_by_R & undefended_zone_enemy_K) * uo_attack_unit_supported_contact_R;

    supported_contact_check_by_Q |= own_Q & attacks_own & attacks_enemy_K;
    attack_units_own += uo_popcnt(supported_contact_check_by_Q & undefended_zone_enemy_K) * uo_attack_unit_supported_contact_Q;

    assert(attack_units_own < 100);
    score += score_attacks_to_K[attack_units_own];

    assert(attack_units_enemy < 100);
    score -= score_attacks_to_K[attack_units_enemy];

    score_mg += uo_score_adjust_piece_square_table_score(mg_table_K[square_own_K]);
    score_eg += uo_score_adjust_piece_square_table_score(eg_table_K[square_own_K]);

    score_mg -= uo_score_adjust_piece_square_table_score(mg_table_K[square_enemy_K ^ 56]);
    score_eg -= uo_score_adjust_piece_square_table_score(eg_table_K[square_enemy_K ^ 56]);

    int mobility_own_K = uo_popcnt(uo_andn(attacks_enemy_K | attacks_lower_value_enemy, attacks_own_K));
    score += uo_score_zero_mobility_K * !mobility_own_K;
    score += uo_score_mobility_K * mobility_own_K;
    score += uo_score_K_square_attackable_by_P * (int16_t)uo_popcnt(own_K & potential_attacks_enemy_P);

    int mobility_enemy_K = uo_popcnt(uo_andn(attacks_own_K | attacks_lower_value_own, attacks_enemy_K));
    score -= uo_score_zero_mobility_K * !mobility_enemy_K;
    score -= uo_score_mobility_K * mobility_enemy_K;
    score -= uo_score_K_square_attackable_by_P * (int16_t)uo_popcnt(enemy_K & potential_attacks_own_P);

    score += uo_score_king_cover_pawn * (int32_t)uo_popcnt(attacks_own_K & own_P);
    score -= uo_score_king_cover_pawn * (int32_t)uo_popcnt(attacks_enemy_K & enemy_P);

    // doupled pawns
    score += uo_score_doubled_P * (((int16_t)uo_popcnt(own_P) - (int16_t)uo_popcnt(uo_bitboard_files(own_P) & uo_bitboard_rank_first))
      - ((int16_t)uo_popcnt(enemy_P) - (int16_t)uo_popcnt(uo_bitboard_files(enemy_P) & uo_bitboard_rank_first)));

    // isolated pawns
    temp = own_P;
    while (temp)
    {
      uo_square square_own_P = uo_bitboard_next_square(&temp);
      bool is_isolated_P = !(uo_square_bitboard_adjecent_files[square_own_P] & own_P);
      score += is_isolated_P * uo_score_isolated_P;
    }

    // isolated pawns
    temp = enemy_P;
    while (temp)
    {
      uo_square square_enemy_P = uo_bitboard_next_square(&temp);
      bool is_isolated_P = !(uo_square_bitboard_adjecent_files[square_enemy_P] & enemy_P);
      score -= is_isolated_P * uo_score_isolated_P;
    }

    // passed pawns
    uo_bitboard passed_own_P = uo_bitboard_passed_P(own_P, enemy_P);
    if (passed_own_P)
    {
      score += uo_score_passed_pawn * (int32_t)uo_popcnt(passed_own_P);
      score += uo_score_passed_pawn_on_fifth * (int32_t)uo_popcnt(passed_own_P & uo_bitboard_rank_fifth);
      score += uo_score_passed_pawn_on_sixth * (int32_t)uo_popcnt(passed_own_P & uo_bitboard_rank_sixth);
      score += uo_score_passed_pawn_on_seventh * (int32_t)uo_popcnt(passed_own_P & uo_bitboard_rank_seventh);
    }

    uo_bitboard passed_enemy_P = uo_bitboard_passed_enemy_P(enemy_P, own_P);
    if (passed_enemy_P)
    {
      score -= uo_score_passed_pawn * (int32_t)uo_popcnt(passed_enemy_P);
      score -= uo_score_passed_pawn_on_fifth * (int32_t)uo_popcnt(passed_enemy_P & uo_bitboard_rank_fourth);
      score -= uo_score_passed_pawn_on_sixth * (int32_t)uo_popcnt(passed_enemy_P & uo_bitboard_rank_third);
      score -= uo_score_passed_pawn_on_seventh * (int32_t)uo_popcnt(passed_enemy_P & uo_bitboard_rank_second);
    }

    // castling rights
    score_mg += uo_score_casting_right * (castling_right_own_OO + castling_right_own_OOO - castling_right_enemy_OO - castling_right_enemy_OOO);

    bool both_rooks_undeveloped_own = uo_popcnt(own_R & (uo_square_bitboard(uo_square__a1) | uo_square_bitboard(uo_square__h1))) == 2;
    bool both_rooks_undeveloped_enemy = uo_popcnt(enemy_R & (uo_square_bitboard(uo_square__a8) | uo_square_bitboard(uo_square__h8))) == 2;

    score_mg += uo_score_rook_stuck_in_corner * both_rooks_undeveloped_own * !(castling_right_own_OO + castling_right_own_OOO);
    score_mg -= uo_score_rook_stuck_in_corner * both_rooks_undeveloped_enemy * !(castling_right_enemy_OO + castling_right_enemy_OOO);

    // king safety
    score_mg += (uo_score_king_in_the_center * (0 != (own_K & (uo_bitboard_file[2] | uo_bitboard_file[3] | uo_bitboard_file[4] | uo_bitboard_file[5]))))
      - (uo_score_king_in_the_center * (0 != (enemy_K & (uo_bitboard_file[2] | uo_bitboard_file[3] | uo_bitboard_file[4] | uo_bitboard_file[5]))));

    score_mg += (uo_score_castled_king * (square_own_K == uo_square__g1))
      - (uo_score_castled_king * (square_enemy_K == uo_square__g8));

    score_mg += (uo_score_castled_king * (square_own_K == uo_square__b1))
      - (uo_score_castled_king * (square_enemy_K == uo_square__b8));

    score_mg += uo_popcnt(uo_bitboard_files(attacks_own_K) & uo_bitboard_rank_first) * uo_score_king_next_to_open_file;
    score_mg -= uo_popcnt(uo_bitboard_files(attacks_enemy_K) & uo_bitboard_rank_first) * uo_score_king_next_to_open_file;

    int32_t material_percentage = uo_position_material_percentage(position);

    score += score_mg * material_percentage / 100;
    score += score_eg * (100 - material_percentage) / 100;

    if (info)
    {
      *info = (uo_evaluation_info){
        .is_valid = true,
        .attacks_own = attacks_own,
        .attacks_enemy = attacks_enemy,
        .undefended_zone_own_K = undefended_zone_own_K,
        .undefended_zone_enemy_K = undefended_zone_enemy_K,
        .attack_units_own = attack_units_own,
        .attack_units_enemy = attack_units_enemy
      };
    }

    assert(score < uo_score_tb_win_threshold && score > -uo_score_tb_win_threshold);
    return score;
  }

  static inline int16_t uo_position_evaluate_and_cache(uo_position *position, uo_evaluation_info *info, uo_move_cache *move_cache)
  {
    uo_move_history *stack = position->stack;

    int16_t static_eval = stack->static_eval;
    bool is_info_ok = info == NULL;

    if (!is_info_ok
      && stack->eval_info.is_valid)
    {
      *info = stack->eval_info;
      is_info_ok = true;
    }

    if (is_info_ok
      && static_eval != uo_score_unknown)
    {
      return static_eval;
    }

    if (stack[-1].move == 0)
    {
      static_eval = -stack[-1].static_eval;

      if (!is_info_ok
        && stack[-1].eval_info.is_valid)
      {
        *info = (uo_evaluation_info){
          .is_valid = true,
          .attacks_own = uo_bswap(stack[-1].eval_info.attacks_enemy),
          .attacks_enemy = uo_bswap(stack[-1].eval_info.attacks_own),
          .undefended_zone_own_K = uo_bswap(stack[-1].eval_info.undefended_zone_enemy_K),
          .undefended_zone_enemy_K = uo_bswap(stack[-1].eval_info.undefended_zone_own_K),
          .attack_units_own = stack[-1].eval_info.attack_units_enemy,
          .attack_units_enemy = stack[-1].eval_info.attack_units_own
        };
        is_info_ok = true;
      }
    }

    if (is_info_ok
      && static_eval != uo_score_unknown)
    {
      stack->static_eval = static_eval;
      stack->eval_info = *info;
      return static_eval;
    }

    move_cache = uo_move_cache_get(move_cache, stack[-1].key, stack[-1].move);

    if (!is_info_ok
      && move_cache
      && move_cache->is_cached.eval_info)
    {
      *info = stack->eval_info = move_cache->eval_info;
      is_info_ok = true;
    }

    if (is_info_ok
      && move_cache
      && move_cache->is_cached.static_eval)
    {
      static_eval = stack->static_eval = move_cache->static_eval;
      return static_eval;
    }

    static_eval = stack->static_eval = uo_position_evaluate(position, info);

    if (move_cache)
    {
      if (info)
      {
        move_cache->is_cached.eval_info = true;
        move_cache->eval_info = *info;
      }

      move_cache->is_cached.static_eval = true;
      move_cache->static_eval = static_eval;
    }

    return static_eval;
  }

  static inline int16_t uo_position_move_evaluation(const uo_position *position, uo_move move, uo_move_cache *move_cache)
  {
    move_cache = uo_move_cache_get(move_cache, position->key, move);

    return move_cache && move_cache->is_cached.static_eval
      ? move_cache->static_eval
      : uo_score_unknown;
  }

  static inline uo_bitboard uo_position_move_checks(const uo_position *position, uo_move move, uo_move_cache *move_cache)
  {
    move_cache = uo_move_cache_get(move_cache, position->key, move);

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
    return position->ply >= UO_MAX_PLY
      || position->movelist.head - position->movelist.moves >= UO_MAX_PLY * UO_BRANCING_FACTOR - 0x100;
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

  static inline uo_piece uo_position_move_piece(const uo_position *position, uo_move move)
  {
    uo_square square_from = uo_move_square_from(move);
    return position->board[square_from];
  }

  static inline uo_bitboard uo_position_square_attackers(const uo_position *position, uo_square square, uo_bitboard occupied)
  {
    return occupied & (
      (uo_bitboard_attacks_P(square, uo_color_own) & position->enemy & position->P)
      | (uo_bitboard_attacks_P(square, uo_color_enemy) & position->own & position->P)
      | (uo_bitboard_attacks_N(square) & position->N)
      | (uo_bitboard_attacks_B(square, occupied) & (position->B | position->Q))
      | (uo_bitboard_attacks_R(square, occupied) & (position->R | position->Q))
      | (uo_bitboard_attacks_K(square) & position->K));
  }

  // Determine if move reveals discovered attacks after it is made.
  static inline bool uo_position_move_discoveries(const uo_position *position, uo_move move, uo_bitboard targets)
  {
    uo_square square_from = uo_move_square_from(move);
    uo_square square_to = uo_move_square_to(move);
    uo_bitboard occupied_before = position->own | position->enemy;
    uo_bitboard occupied_after = uo_andn(uo_square_bitboard(square_from), occupied_before);
    uo_bitboard attackers = position->own & occupied_after;
    uo_square square_enemy_K = uo_tzcnt(position->K & position->enemy);

    // Loop through target squares (pieces) and check whether they are attacked after move is made
    // but ignore those squares that were are already attacked before the move except king
    while (targets)
    {
      uo_square square = uo_bitboard_next_square(&targets);
      uo_bitboard attacks = uo_position_square_attackers(position, square, occupied_after) & attackers;

      if (attacks
        && square != square_enemy_K
        && (uo_position_square_attackers(position, square, occupied_before) & position->own))
      {
        attacks = 0;
      }

      if (attacks) return true;
    }

    return false;
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
    move_cache = uo_move_cache_get(move_cache, position->key, move);

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
      gain[depth - 1] = uo_min(-gain[depth], gain[depth - 1]);
    }

    if (move_cache)
    {
      move_cache->is_cached.see = true;
      move_cache->see = gain[0];
    }

    return gain[0];
  }

  // see: https://www.chessprogramming.org/SEE_-_The_Swap_Algorithm
  static inline bool uo_position_move_see_gt(const uo_position *position, uo_move move, int16_t gt, uo_move_cache *move_cache)
  {
    move_cache = uo_move_cache_get(move_cache, position->key, move);

    if (move_cache)
    {
      if (move_cache->is_cached.see_lbound && move_cache->see_lbound > gt) return true;
      if (move_cache->is_cached.see_ubound && move_cache->see_ubound <= gt) return false;
      if (move_cache->is_cached.see) return move_cache->see > gt;

      // Although static evaluation is not purely materialistic, let's still utilize it if it is available
      if (move_cache->is_cached.static_eval
        && position->stack->static_eval != uo_score_unknown)
      {
        int16_t eval_diff = -move_cache->static_eval - position->stack->static_eval;
        return eval_diff > gt;
      }
    }

    const uo_piece *board = position->board;
    uo_square square_to = uo_move_square_to(move);
    uo_piece piece_captured = board[square_to];

    // SEE cannot be higher than the value of captured piece
    int16_t see_ubound = uo_piece_value(piece_captured);

    if (see_ubound <= gt)
    {
      if (move_cache)
      {
        move_cache->is_cached.see_ubound = true;
        move_cache->see_ubound = see_ubound;
      }

      return false;
    }

    // Let's still try one more trick and use attacks bitboard from static evaluation to see if the square attacked
    if (position->stack->eval_info.is_valid)
    {
      bool is_attacked = position->stack->eval_info.attacks_enemy & uo_square_bitboard(square_to);
      if (!is_attacked)
      {
        if (move_cache)
        {
          move_cache->is_cached.see = true;
          move_cache->see = see_ubound;
        }

        return true;
      }
    }

    int16_t gain[32];
    gain[0] = see_ubound;

    uo_square square_from = uo_move_square_from(move);
    uo_piece piece = board[square_from];
    assert(uo_color(piece) == uo_color_own);

    // Speculative store, if defended
    gain[1] = uo_piece_value(piece) - gain[0];

    // SEE cannot be lower than the value of pieces after initial recapture
    int16_t see_lbound = -gain[1];
    if (see_lbound > gt)
    {
      if (move_cache)
      {
        move_cache->is_cached.see_lbound = true;
        move_cache->see_lbound = see_lbound;
      }

      return true;
    }

    // Resolve recapture chain

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

    size_t depth = 2;
    for (;; ++depth)
    {
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

      int16_t piece_value = uo_piece_value(piece);

      if (depth & 1)
      {
        see_lbound = see_ubound - piece_value;
        if (see_lbound > gt)
        {
          if (move_cache)
          {
            move_cache->is_cached.see_lbound = true;
            move_cache->see_lbound = see_lbound;
          }

          return true;
        }
      }
      else
      {
        see_ubound = see_lbound + piece_value;
        if (see_ubound <= gt)
        {
          if (move_cache)
          {
            move_cache->is_cached.see_ubound = true;
            move_cache->see_ubound = see_ubound;
          }

          return false;
        }
      }

      // speculative store, if defended
      gain[depth] = piece_value - gain[depth - 1];

      if (gain[depth - 1] > 0 && gain[depth] < 0)
      {
        // pruning does not influence the result
        ++depth;
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

    --depth;
    while (--depth)
    {
      gain[depth - 1] = uo_min(-gain[depth], gain[depth - 1]);
    }

    if (move_cache)
    {
      move_cache->is_cached.see = true;
      move_cache->see = gain[0];
    }

    return gain[0] > gt;
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
    // Use zero to indicate previous move
    uo_move *killers = move
      ? position->stack[-1].search.killers
      : position->stack[-2].search.killers;

    if (move == 0) move = position->stack[-1].move;

    return move == killers[0] || move == killers[1];
  }

  static inline bool uo_position_is_skipped_move(const uo_position *position, uo_move move)
  {
    const uo_move_history *stack = position->stack;
    assert(stack->moves_generated);
    assert(stack->moves_sorted);

    size_t skipped_move_count = stack->skipped_move_count;
    size_t non_skipped_move_count = stack->move_count - skipped_move_count;

    if (skipped_move_count < non_skipped_move_count)
    {
      for (size_t i = 0; i < skipped_move_count; ++i)
      {
        if (move == position->movelist.head[i]) return true;
      }

      return false;
    }

    for (size_t i = 0; i < non_skipped_move_count; ++i)
    {
      if (move == position->movelist.head[i]) return false;
    }

    return true;
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

  static inline void uo_position_update_cutoff_history(uo_position *position, size_t cutoff_move_num, size_t depth)
  {
    // Increment butterfly heuristic counter for previously tried non-cutoff moves
    for (size_t i = 0; i < cutoff_move_num; ++i)
    {
      uo_move move = position->movelist.head[i];
      if (uo_move_is_tactical(move)) continue;
      size_t index = uo_position_move_history_heuristic_index(position, move);
      ++position->bftable[index];
    }

    uo_move move = position->movelist.head[cutoff_move_num];
    if (uo_move_is_tactical(move)) return;

    // Update history heuristic score for the cutoff move
    size_t index = uo_position_move_history_heuristic_index(position, move);
    position->hhtable[index] += 2 << depth;

    // Update killer moves
    uo_move *killers = position->stack[-1].search.killers;
    if (killers[0] != move)
    {
      killers[1] = killers[0];
      killers[0] = move;
    }
  }

  static inline int16_t uo_position_move_relative_history_score(const uo_position *position, uo_move move)
  {
    // relative history heuristic
    size_t index = uo_position_move_history_heuristic_index(position, move);
    uint32_t hhscore = position->hhtable[index] << 4;
    uint32_t bfscore = position->bftable[index] + 1;

    return hhscore / bfscore;
  }

  static inline int16_t uo_position_calculate_non_tactical_move_score(const uo_position *position, uo_move move, uo_move_cache *move_cache)
  {
    const uo_piece *board = position->board;
    uo_square square_from = uo_move_square_from(move);
    uo_square square_to = uo_move_square_to(move);
    uo_piece piece = board[square_from];

    int16_t move_score = 0;

    // Checks
    uo_bitboard checks = uo_position_move_checks(position, move, move_cache);
    move_score += !checks ? 0 : uo_popcnt(checks) == 2 ? 1000 : 100;

    // Killer heuristic
    bool is_killer_move = uo_position_is_killer_move(position, move);
    move_score += is_killer_move * 1000;

    // Relative history heuristic
    int16_t rhscore = uo_position_move_relative_history_score(position, move);
    move_score += rhscore;

    // Default to piece-square table based move ordering
    if (move_score == 0)
    {
      int32_t material_percentage = uo_position_material_percentage(position);

      move_score -= uo_score_Q;
      int16_t score_mg = 0;
      int16_t score_eg = 0;

      switch (piece)
      {
        case uo_piece__P:
          score_mg += mg_table_P[square_to] - mg_table_P[square_from];
          score_eg += eg_table_P[square_to] - eg_table_P[square_from];
          break;

        case uo_piece__N:
          score_mg += mg_table_N[square_to] - mg_table_N[square_from];
          score_eg += eg_table_N[square_to] - eg_table_N[square_from];
          break;

        case uo_piece__B:
          score_mg += mg_table_B[square_to] - mg_table_B[square_from];
          score_eg += eg_table_B[square_to] - eg_table_B[square_from];
          break;

        case uo_piece__R:
          score_mg += mg_table_R[square_to] - mg_table_R[square_from];
          score_eg += eg_table_R[square_to] - eg_table_R[square_from];
          break;

        case uo_piece__Q:
          score_mg += mg_table_Q[square_to] - mg_table_Q[square_from];
          score_eg += eg_table_Q[square_to] - eg_table_Q[square_from];
          break;

        case uo_piece__K:
          score_mg += mg_table_K[square_to] - mg_table_K[square_from];
          score_eg += eg_table_K[square_to] - eg_table_K[square_from];
          break;
      }

      move_score += score_mg * material_percentage / 100;
      move_score += score_eg * (100 - material_percentage) / 100;
    }

    return move_score;
  }

  static inline int16_t uo_position_calculate_tactical_move_score(uo_position *position, uo_move move, uo_move_cache *move_cache)
  {
    int16_t move_score = 0;

    // Checks
    uo_bitboard checks = uo_position_move_checks(position, move, move_cache);
    move_score += !checks ? 0 : uo_popcnt(checks) == 2 ? 1000 : 100;

    uo_square move_type = uo_move_get_type(move);

    if (move_type == uo_move_type__enpassant)
    {
      move_score += 2050;
    }
    else if (move_type & uo_move_type__x)
    {
      move_score += 2000;

      const uo_piece *board = position->board;
      uo_square square_from = uo_move_square_from(move);
      uo_square square_to = uo_move_square_to(move);
      uo_piece piece = board[square_from];
      uo_piece piece_captured = board[square_to];

      // MVV-LVA (Most Valuable Victim - Least Valuable Aggressor)
      move_score += uo_piece_value(piece_captured) - uo_piece_value(piece) / (uo_score_Q / uo_score_P + 1);

      // Penalty of captures which lose material
      move_score -= !uo_position_move_see_gt(position, move, -1, move_cache) * uo_piece_value(piece);
    }

    if (move_type & uo_move_type__promo)
    {
      move_score += 2500 + move_type;
    }

    return move_score;
  }

  static inline int uo_position_partition_moves(uo_position *position, uo_move *movelist, int lo, int hi)
  {
    int16_t *move_scores = &position->movelist.move_scores[movelist - position->movelist.head];
    int16_t pivot_score = move_scores[hi];
    uo_move temp_move;
    int16_t temp_score;

    for (int j = lo; j <= hi - 1; j++)
    {
      if (move_scores[j] >= pivot_score)
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

  static inline void uo_position_insertion_sort_moves(uo_position *position, uo_move *movelist, int lo, int hi)
  {
    int16_t *move_scores = &position->movelist.move_scores[movelist - position->movelist.head];
    uo_move temp_move;
    int16_t temp_score;

    for (int i = lo + 1; i <= hi; i++)
    {
      temp_move = movelist[i];
      temp_score = move_scores[i];

      int j = i - 1;

      while (j >= lo && move_scores[j] < temp_score)
      {
        movelist[j + 1] = movelist[j];
        move_scores[j + 1] = move_scores[j];
        j--;
      }

      movelist[j + 1] = temp_move;
      move_scores[j + 1] = temp_score;
    }
  }

  static inline size_t uo_position_sort_skipped_moves(uo_position *position, uo_move *movelist, int lo, int hi)
  {
    int16_t *move_scores = &position->movelist.move_scores[movelist - position->movelist.head];
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
    if (lo >= hi) return;

    if (hi - lo <= UO_INSERTION_SORT_MOVE_COUNT + 1)
    {
      uo_position_insertion_sort_moves(position, movelist, lo, hi);
      return;
    }

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

  static inline void uo_position_sort_move_as_first(uo_position *position, uo_move move)
  {
    uo_move *movelist = position->movelist.head;
    size_t move_count = position->stack->move_count;

    // If there is no move specified or the move is already first, then return.
    if (!move || movelist[0] == move) return;

    // Find the specified move
    for (int i = 1; i < move_count; ++i)
    {
      if (movelist[i] == move)
      {
        // Move found. Shift preceding moves by one position.
        while (--i >= 0)
        {
          movelist[i + 1] = movelist[i];
        }

        // Place best move as first
        movelist[0] = move;
        break;
      }
    }
  }

  static inline void uo_position_sort_moves(uo_position *position, uo_move bestmove, uo_move_cache *move_cache)
  {
    if (!position->stack->moves_sorted)
    {
      uo_move *movelist = position->movelist.head;
      size_t move_count = position->stack->move_count;

      uo_position_sort_tactical_moves(position, move_cache);
      uo_position_sort_non_tactical_moves(position, move_cache);
      move_count -= uo_position_sort_skipped_moves(position, movelist, 0, move_count - 1);
      position->stack->moves_sorted = true;
    }

    // set bestmove as first and shift other moves by one position
    uo_position_sort_move_as_first(position, bestmove);
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

  uo_move uo_position_parse_move(const uo_position *position, char str[5]);

  uo_move uo_position_parse_pgn_move(uo_position *position, char *pgn);

  size_t uo_position_print_move(const uo_position *position, uo_move move, char str[6]);

  size_t uo_position_perft(uo_position *position, size_t depth);

  uo_position *uo_position_randomize(uo_position *position, const char *pieces /* e.g. KQRPPvKRRBNP */);

#ifdef __cplusplus
}
#endif

#endif
