#include "uo_position.h"
#include "uo_zobrist.h"
#include "uo_util.h"
#include "uo_move.h"
#include "uo_piece.h"
#include "uo_square.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <immintrin.h>

typedef struct uo_movegenlist
{
  struct
  {
    struct
    {
      uo_move *root;
      uo_move *head;
    }tactical;
    struct
    {
      uo_move *root;
      uo_move *head;
    }non_tactical;
  } moves;
} uo_movegenlist;

bool uo_position_is_ok(const uo_position *position)
{
  if (position->movelist.head - position->movelist.moves > UO_MAX_PLY * UO_BRANCING_FACTOR)
  {
    return false;
  }

  if (uo_popcnt(position->K) != 2)
  {
    return false;
  }

  for (uo_square square = 0; square < 64; ++square)
  {
    uo_bitboard mask = uo_square_bitboard(square);
    uo_piece piece = position->board[square];

    if (piece <= 1)
    {
      if (position->P & mask)
      {
        return false;
      }

      if (position->N & mask)
      {
        return false;
      }

      if (position->B & mask)
      {
        return false;
      }

      if (position->R & mask)
      {
        return false;
      }

      if (position->Q & mask)
      {
        return false;
      }

      if (position->K & mask)
      {
        return false;
      }
    }
    else
    {
      uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);

      if (!(*bitboard & mask))
      {
        return false;
      }

      if (bitboard != &position->P && position->P & mask)
      {
        return false;
      }

      if (bitboard != &position->N && position->N & mask)
      {
        return false;
      }

      if (bitboard != &position->B && position->B & mask)
      {
        return false;
      }

      if (bitboard != &position->R && position->R & mask)
      {
        return false;
      }

      if (bitboard != &position->Q && position->Q & mask)
      {
        return false;
      }

      if (bitboard != &position->K && position->K & mask)
      {
        return false;
      }

      if (uo_color(piece) == uo_color_own && !(position->own & mask))
      {
        return false;
      }

      if (uo_color(piece) == uo_color_enemy && !(position->enemy & mask))
      {
        return false;
      }
    }
  }

  return true;
}

bool uo_position_is_move_ok(const uo_position *position, uo_move move)
{
  uo_square square_from = uo_move_square_from(move);
  uo_square square_to = uo_move_square_to(move);

  const uo_piece *board = position->board;

  uo_piece piece = board[square_from];
  if (piece <= 1)
  {
    return false;
  }

  if (uo_color(piece) != uo_color_own)
  {
    return false;
  }

  uo_piece piece_captured = board[square_to];
  if (piece_captured == uo_piece__k)
  {
    return false;
  }

  if (uo_move_is_capture(move))
  {
    if (piece_captured <= 1 && !uo_move_is_enpassant(move))
    {
      return false;
    }
  }
  else if (piece_captured > 1)
  {
    return false;
  }

  return true;
}

bool uo_position_is_checks_and_pins_ok(const uo_position *position)
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

  uo_bitboard checks_by_BQ = enemy_BQ & uo_bitboard_attacks_B(square_own_K, occupied);
  uo_bitboard checks_by_RQ = enemy_RQ & uo_bitboard_attacks_R(square_own_K, occupied);
  uo_bitboard checks_by_N = enemy_N & uo_bitboard_attacks_N(square_own_K);
  uo_bitboard checks_by_P = enemy_P & uo_bitboard_attacks_P(square_own_K, uo_color_own);

  uo_bitboard pins_by_BQ = uo_bitboard_pins_B(square_own_K, occupied, enemy_BQ);
  uo_bitboard pins_by_RQ = uo_bitboard_pins_R(square_own_K, occupied, enemy_RQ);

  if (position->checks.by_BQ != checks_by_BQ) return false;
  if (position->checks.by_RQ != checks_by_RQ) return false;
  if (position->checks.by_N != checks_by_N) return false;
  if (position->checks.by_P != checks_by_P) return false;
  if (position->pins.by_BQ != pins_by_BQ) return false;
  if (position->pins.by_RQ != pins_by_RQ) return false;

  return true;
}

uint64_t uo_position_calculate_key(uo_position *const position)
{
  uint64_t key = 0;

  uo_position_flags flags = position->flags;
  uint8_t color_to_move = uo_position_flags_color_to_move(flags);
  uint8_t castling = uo_position_flags_castling(flags);
  uint8_t enpassant_file = uo_position_flags_enpassant_file(flags);

  if (color_to_move == uo_black)
  {
    key = uo_bswap(key);
  }

  key ^= uo_zobrist_castling[castling];
  key ^= uo_zobrist_enpassant_file[enpassant_file];

  for (uo_square square = 0; square < 64; ++square)
  {
    uo_piece piece = position->board[square];
    if (piece > 1)
    {
      key ^= uo_zobkey(piece, square);
    }
  }

  return key;
}

static inline void uo_position_flip_board(uo_position *position)
{
  uo_bitboard temp = position->own;
  position->own = uo_bswap(position->enemy);
  position->enemy = uo_bswap(temp);

  position->P = uo_bswap(position->P);
  position->N = uo_bswap(position->N);
  position->B = uo_bswap(position->B);
  position->R = uo_bswap(position->R);
  position->Q = uo_bswap(position->Q);
  position->K = uo_bswap(position->K);

  position->key = uo_bswap(position->key);

  __m256i mask_color = _mm256_set1_epi64x(0x0101010101010101ull);
  __m256i *board_lo = (__m256i *)(&position->board[0]);
  __m256i *board_hi = (__m256i *)(&position->board[32]);
  __m256i lo = _mm256_loadu_si256(board_lo);
  __m256i hi = _mm256_loadu_si256(board_hi);
  lo = _mm256_xor_si256(lo, mask_color);
  hi = _mm256_xor_si256(hi, mask_color);
  lo = _mm256_permute4x64_epi64(lo, 0x1B);
  hi = _mm256_permute4x64_epi64(hi, 0x1B);
  _mm256_storeu_si256(board_lo, hi);
  _mm256_storeu_si256(board_hi, lo);

  position->flags = uo_position_flags_flip_castling(position->flags);
}

static inline void uo_position_update_repetitions(uo_position *position)
{
  uo_move_history *stack = position->stack;

  int rule50 = uo_position_flags_rule50(position->flags);
  for (int i = 1; i <= rule50; ++i)
  {
    if (stack[-i].key == position->key)
    {
      stack->repetitions = stack[-i].repetitions + 1;
      return;
    }
  }

  stack->repetitions = 0;
}

static inline void uo_position_set_flags(uo_position *position, uo_position_flags flags)
{
  uint64_t key = position->key;

  uint8_t enpassant_file = uo_position_flags_enpassant_file(position->flags);
  key ^= uo_zobrist_enpassant_file[enpassant_file];
  enpassant_file = uo_position_flags_enpassant_file(flags);
  key ^= uo_zobrist_enpassant_file[enpassant_file];

  uint8_t castling = uo_position_flags_castling(position->flags);
  key ^= uo_zobrist_castling[castling];
  castling = uo_position_flags_castling(flags);
  key ^= uo_zobrist_castling[castling];

  position->key = key;
  position->flags = flags;
}

static inline void uo_position_do_switch_turn(uo_position *position, uo_position_flags flags)
{
  uo_position_set_flags(position, flags);
  position->update_status.checks_and_pins = false;
  position->movelist.head += position->stack->move_count;
  position->flags ^= 1;
  ++position->ply;
  ++position->stack;
  position->stack->moves_generated = false;
}
static inline void uo_position_undo_switch_turn(uo_position *position)
{
  --position->ply;
  --position->stack;
  uo_move_history *stack = position->stack;
  position->movelist.head -= stack->move_count;
  position->update_status.checks_and_pins = false;
  uo_position_set_flags(position, stack->flags);

  if (uo_move_is_capture(stack->move))
  {
    --position->piece_captured;
  }
}

static inline void uo_position_do_move(uo_position *position, uo_square square_from, uo_square square_to)
{
  uo_piece *board = position->board;
  uo_piece piece = board[square_from];
  board[square_from] = 0;
  board[square_to] = piece;

  position->key ^= uo_zobkey(piece, square_from) ^ uo_zobkey(piece, square_to);

  uo_bitboard bitboard_from = uo_square_bitboard(square_from);
  uo_bitboard bitboard_to = uo_square_bitboard(square_to);

  uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);
  *bitboard ^= bitboard_from | bitboard_to;
  position->own ^= bitboard_from | bitboard_to;
}
static inline void uo_position_undo_move(uo_position *position, uo_square square_from, uo_square square_to)
{
  uo_piece *board = position->board;
  uo_piece piece = board[square_to];
  board[square_from] = piece;
  board[square_to] = 0;

  position->key ^= uo_zobkey(piece, square_from) ^ uo_zobkey(piece, square_to);

  uo_bitboard bitboard_from = uo_square_bitboard(square_from);
  uo_bitboard bitboard_to = uo_square_bitboard(square_to);

  uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);
  *bitboard ^= bitboard_from | bitboard_to;
  position->own ^= bitboard_from | bitboard_to;
}

static inline void uo_position_do_capture(uo_position *position, uo_square square_from, uo_square square_to)
{
  uo_piece *board = position->board;
  uo_piece piece = board[square_from];
  uo_piece piece_captured = board[square_to];
  board[square_from] = 0;
  board[square_to] = piece;

  position->key ^= uo_zobkey(piece, square_from) ^ uo_zobkey(piece, square_to) ^ uo_zobkey(piece_captured, square_to);

  uo_bitboard bitboard_from = uo_square_bitboard(square_from);
  uo_bitboard bitboard_to = uo_square_bitboard(square_to);

  uo_bitboard *bitboard_captured = uo_position_piece_bitboard(position, piece_captured);
  *bitboard_captured = uo_andn(bitboard_to, *bitboard_captured);
  position->enemy = uo_andn(bitboard_to, position->enemy);

  uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);
  *bitboard ^= bitboard_from | bitboard_to;
  position->own ^= bitboard_from | bitboard_to;
}
static inline void uo_position_undo_capture(uo_position *position, uo_square square_from, uo_square square_to, uo_piece piece_captured)
{
  uo_piece *board = position->board;
  uo_piece piece = board[square_to];
  board[square_from] = piece;
  board[square_to] = piece_captured;

  position->key ^= uo_zobkey(piece, square_from) ^ uo_zobkey(piece, square_to) ^ uo_zobkey(piece_captured, square_to);

  uo_bitboard bitboard_from = uo_square_bitboard(square_from);
  uo_bitboard bitboard_to = uo_square_bitboard(square_to);

  uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);
  *bitboard ^= bitboard_from | bitboard_to;
  position->own ^= bitboard_from | bitboard_to;

  uo_bitboard *bitboard_captured = uo_position_piece_bitboard(position, piece_captured);
  *bitboard_captured |= bitboard_to;
  position->enemy |= bitboard_to;
}

static inline void uo_position_do_enpassant(uo_position *position, uo_square square_from, uo_square square_to)
{
  uo_position_do_move(position, square_from, square_to);

  uo_piece *board = position->board;
  uo_piece piece = board[square_to];
  uo_square square_piece_captured = square_to - 8;
  board[square_piece_captured] = 0;

  position->key ^= uo_zobkey(uo_piece__p, square_piece_captured);

  uo_bitboard enpassant = uo_square_bitboard(square_piece_captured);
  position->P = uo_andn(enpassant, position->P);
  position->enemy = uo_andn(enpassant, position->enemy);
}
static inline void uo_position_undo_enpassant(uo_position *position, uo_square square_from, uo_square square_to)
{
  uo_position_undo_move(position, square_from, square_to);

  uo_piece *board = position->board;
  uo_piece piece = board[square_from];
  uo_square square_piece_captured = square_to - 8;
  position->board[square_piece_captured] = uo_piece__p;

  position->key ^= uo_zobkey(uo_piece__p, square_piece_captured);

  uo_bitboard enpassant = uo_square_bitboard(square_piece_captured);
  position->P |= enpassant;
  position->enemy |= enpassant;
}

static inline void uo_position_do_promo(uo_position *position, uo_square square_from, uo_piece piece)
{
  uo_piece *board = position->board;
  uo_square square_to = square_from + 8;
  board[square_from] = 0;
  board[square_to] = piece;

  position->key ^= uo_zobkey(uo_piece__P, square_from) ^ uo_zobkey(piece, square_to);

  uo_bitboard bitboard_from = uo_square_bitboard(square_from);
  uo_bitboard bitboard_to = uo_square_bitboard(square_to);

  position->P = uo_andn(bitboard_from, position->P);

  uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);
  *bitboard |= bitboard_to;

  position->own ^= bitboard_from | bitboard_to;
}
static inline void uo_position_undo_promo(uo_position *position, uo_square square_from)
{
  uo_square square_to = square_from + 8;

  uo_piece *board = position->board;
  uo_piece piece = board[square_to];
  board[square_from] = uo_piece__P;
  board[square_to] = 0;

  position->key ^= uo_zobkey(uo_piece__P, square_from) ^ uo_zobkey(piece, square_to);

  uo_bitboard bitboard_from = uo_square_bitboard(square_from);
  uo_bitboard bitboard_to = uo_square_bitboard(square_to);

  position->P |= bitboard_from;

  uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);
  *bitboard = uo_andn(bitboard_to, *bitboard);

  position->own ^= bitboard_from | bitboard_to;
}

static inline void uo_position_do_promo_capture(uo_position *position, uo_square square_from, uo_square square_to, uo_piece piece)
{
  uo_piece *board = position->board;
  uo_piece piece_captured = board[square_to];
  board[square_from] = 0;
  board[square_to] = piece;

  position->key ^= uo_zobkey(uo_piece__P, square_from) ^ uo_zobkey(piece, square_to) ^ uo_zobkey(piece_captured, square_to);

  uo_bitboard bitboard_from = uo_square_bitboard(square_from);
  uo_bitboard bitboard_to = uo_square_bitboard(square_to);

  uo_bitboard *bitboard_captured = uo_position_piece_bitboard(position, piece_captured);
  *bitboard_captured = uo_andn(bitboard_to, *bitboard_captured);

  position->enemy = uo_andn(bitboard_to, position->enemy);
  position->P = uo_andn(bitboard_from, position->P);

  uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);
  *bitboard |= bitboard_to;

  position->own ^= bitboard_from | bitboard_to;
}
static inline void uo_position_undo_promo_capture(uo_position *position, uo_square square_from, uo_square square_to, uo_piece piece_captured)
{
  uo_piece *board = position->board;
  uo_piece piece = board[square_to];
  board[square_from] = uo_piece__P;
  board[square_to] = piece_captured;

  position->key ^= uo_zobkey(uo_piece__P, square_from) ^ uo_zobkey(piece, square_to) ^ uo_zobkey(piece_captured, square_to);

  uo_bitboard bitboard_from = uo_square_bitboard(square_from);
  uo_bitboard bitboard_to = uo_square_bitboard(square_to);

  position->P |= bitboard_from;

  uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);
  *bitboard = uo_andn(bitboard_to, *bitboard);
  position->own ^= bitboard_from | bitboard_to;

  uo_bitboard *bitboard_captured = uo_position_piece_bitboard(position, piece_captured);
  *bitboard_captured |= bitboard_to;
  position->enemy |= bitboard_to;
}

// see: https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation
// example fen: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
uo_position *uo_position_from_fen(uo_position *position, char *fen)
{
  uo_position_flags flags = 0;
  char piece_placement[72];
  char active_color;
  char castling[5];
  char enpassant[3];
  int rule50;
  int fullmove;

  int count = sscanf(fen, "%71s %c %4s %2s %d %d",
    piece_placement, &active_color,
    castling, enpassant,
    &rule50, &fullmove);

  if (count != 6)
  {
    return NULL;
  }

  // clear position
  memset(position, 0, sizeof * position);

  char c;

  // 1. Piece placement

  char *ptr = piece_placement;

  // loop by rank
  for (int i = 7; i >= 0; --i)
  {
    // loop by file
    for (int j = 0; j < 8; ++j)
    {
      c = *ptr++;

      // empty squares
      if (c > '0' && c <= ('8' - j))
      {
        j += c - '0' - 1;
        continue;
      }

      uo_piece piece = uo_piece_from_char(c);

      if (!piece)
      {
        return NULL;
      }

      uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);
      uo_bitboard *bitboard_color = position->bitboards + uo_color(piece);
      uo_square square = (i << 3) + j;
      uo_bitboard mask = uo_square_bitboard(square);

      position->board[square] = piece;
      *bitboard |= mask;
      *bitboard_color |= mask;
    }

    c = *ptr++;

    if (i != 0 && c != '/')
    {
      return NULL;
    }
  }

  // 2. Active color

  if (active_color == 'b')
  {
    flags = uo_position_flags_update_color_to_move(flags, uo_black);
    ++position->root_ply;
  }
  else if (active_color != 'w')
  {
    return NULL;
  }

  // 3. Castling availability

  ptr = castling;
  c = *ptr++;

  if (c == '-')
  {
    c = *ptr++;
  }
  else
  {
    if (c == 'K')
    {
      flags = uo_position_flags_update_castling_K(flags, true);
      c = *ptr++;
    }

    if (c == 'Q')
    {
      flags = uo_position_flags_update_castling_Q(flags, true);
      c = *ptr++;
    }

    if (c == 'k')
    {
      flags = uo_position_flags_update_castling_k(flags, true);
      c = *ptr++;
    }

    if (c == 'q')
    {
      flags = uo_position_flags_update_castling_q(flags, true);
      c = *ptr++;
    }
  }

  // 4. En passant target square

  ptr = enpassant;
  c = *ptr++;

  if (c != '-')
  {
    uint8_t file = c - 'a';
    if (file < 0 || file >= 8)
    {
      return NULL;
    }

    c = *ptr++;
    uint8_t rank = c - '1';
    if (rank < 0 || rank >= 8)
    {
      return NULL;
    }

    flags = uo_position_flags_update_enpassant_file(flags, file + 1);
  }

  // 5. Halfmove clock
  // 6. Fullmove number
  flags = uo_position_flags_update_rule50(flags, rule50);
  position->root_ply += (fullmove - 1) << 1;
  position->ply = 0;
  position->stack = position->history;
  position->flags = flags;
  position->key = uo_position_calculate_key(position);
  uo_position_reset_root(position);

  if (uo_color(position->flags) == uo_black)
  {
    uo_position_flip_board(position);
  }

  uo_position_update_checks_and_pins(position);

  return position;
}

size_t uo_position_print_fen(uo_position *const position, char fen[90])
{
  if (uo_color(position->flags) == uo_black)
  {
    uo_position_flip_board(position);
  }

  char *ptr = fen;

  for (int rank = 7; rank >= 0; --rank)
  {
    int empty_count = 0;
    for (int file = 0; file < 8; ++file)
    {
      uo_square square = (rank << 3) + file;
      uo_piece piece = position->board[square];

      if (piece > 1)
      {
        if (empty_count)
        {
          ptr += sprintf(ptr, "%d", empty_count);
          empty_count = 0;
        }

        ptr += sprintf(ptr, "%c", uo_piece_to_char(piece));
      }
      else
      {
        ++empty_count;
      }
    }

    if (empty_count)
    {
      ptr += sprintf(ptr, "%d", empty_count);
    }

    if (rank > 0)
    {
      ptr += sprintf(ptr, "/");
    }
  }

  if (uo_color(position->flags) == uo_black)
  {
    uo_position_flip_board(position);
  }

  uo_position_flags flags = position->flags;

  uint8_t color_to_move = uo_position_flags_color_to_move(flags);
  ptr += sprintf(ptr, " %c ", color_to_move ? 'b' : 'w');

  uint8_t castling = uo_position_flags_castling(flags);

  if (castling)
  {
    uint8_t castling_K = uo_position_flags_castling_OO(flags);
    if (castling_K)
      ptr += sprintf(ptr, "%c", 'K');

    uint8_t castling_Q = uo_position_flags_castling_OOO(flags);
    if (castling_Q)
      ptr += sprintf(ptr, "%c", 'Q');

    uint8_t castling_k = uo_position_flags_castling_enemy_OO(flags);
    if (castling_k)
      ptr += sprintf(ptr, "%c", 'k');

    uint8_t castling_q = uo_position_flags_castling_enemy_OOO(flags);
    if (castling_q)
      ptr += sprintf(ptr, "%c", 'q');

    ptr += sprintf(ptr, " ");
  }
  else
  {
    ptr += sprintf(ptr, "- ");
  }

  uint8_t enpassant_file = uo_position_flags_enpassant_file(flags);
  if (enpassant_file)
  {
    ptr += sprintf(ptr, "%c%d ", 'a' + enpassant_file - 1, color_to_move ? 3 : 6);
  }
  else
  {
    ptr += sprintf(ptr, "- ");
  }

  uint8_t rule50 = uo_position_flags_rule50(flags);
  uint16_t fullmoves = ((position->root_ply + position->ply) >> 1) + 1;

  ptr += sprintf(ptr, "%d %d", rule50, fullmoves);

  return ptr - fen;
}

size_t uo_position_print_diagram(uo_position *const position, char diagram[663])
{
  char *ptr = diagram;

  if (uo_color(position->flags) == uo_black)
  {
    uo_position_flip_board(position);
  }

  for (int rank = 7; rank >= 0; --rank)
  {
    ptr += sprintf(ptr, " +---+---+---+---+---+---+---+---+\n |");
    for (int file = 0; file < 8; ++file)
    {
      uo_square square = (rank << 3) + file;
      uo_piece piece = position->board[square];
      ptr += sprintf(ptr, " %c |", piece > 1 ? uo_piece_to_char(piece) : ' ');
    }
    ptr += sprintf(ptr, " %d\n", rank + 1);
  }

  if (uo_color(position->flags) == uo_black)
  {
    uo_position_flip_board(position);
  }

  ptr += sprintf(ptr, " +---+---+---+---+---+---+---+---+\n");
  ptr += sprintf(ptr, "   a   b   c   d   e   f   g   h\n");

  return ptr - diagram;
}

void uo_position_copy(uo_position *restrict dst, const uo_position *restrict src)
{
  size_t movelist_advance = src->movelist.head - src->movelist.moves;
  size_t generated_move_count = src->stack->moves_generated ? src->stack->move_count : 0;
  size_t size = offsetof(uo_position, movelist) + ((movelist_advance + generated_move_count) * sizeof(uo_move));
  memcpy(dst, src, size);
  dst->piece_captured = dst->captures + (src->piece_captured - src->captures);
  dst->movelist.head = dst->movelist.moves + movelist_advance;
  dst->stack = dst->history + (src->stack - src->history);
}

void uo_position_make_move(uo_position *position, uo_move move)
{
  assert(uo_position_is_move_ok(position, move));
  assert(uo_position_is_ok(position));

  uo_square square_from = uo_move_square_from(move);
  uo_square square_to = uo_move_square_to(move);
  uo_piece *board = position->board;
  uo_piece piece = board[square_from];
  uo_piece piece_captured = 0;

  uo_position_flags flags = position->flags;

  uo_move_history *stack = position->stack;
  stack->flags = flags;
  stack->move = move;
  stack->key = position->key;

  uint8_t rule50 = uo_position_flags_rule50(flags);
  flags = uo_position_flags_update_rule50(flags, rule50 + 1);
  flags = uo_position_flags_update_enpassant_file(flags, 0);

  uo_move_type move_type = uo_move_get_type(move);

  switch (move_type)
  {
    case uo_move_type__quiet:
      uo_position_do_move(position, square_from, square_to);
      break;

    case uo_move_type__P_double_push: {
      uo_position_do_move(position, square_from, square_to);

      uo_bitboard bitboard_to = uo_square_bitboard(square_to);

      // en passant

      uo_bitboard mask_enemy = position->enemy;
      uo_bitboard enemy_P = mask_enemy & position->P;

      uo_bitboard adjecent_enemy_pawn = uo_square_bitboard_adjecent_files[square_to] & uo_bitboard_rank_fourth & enemy_P;
      if (adjecent_enemy_pawn)
      {
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
            flags = uo_position_flags_update_enpassant_file(flags, uo_square_file(square_to) + 1);
          }
        }
        else
        {
          flags = uo_position_flags_update_enpassant_file(flags, uo_square_file(square_to) + 1);
        }
      }
      break;
    }

    case uo_move_type__x:
      piece_captured = board[square_to];
      *position->piece_captured++ = piece_captured;
      uo_position_do_capture(position, square_from, square_to);
      break;

    case uo_move_type__enpassant:
      piece_captured = board[square_to];
      *position->piece_captured++ = piece_captured;
      uo_position_do_enpassant(position, square_from, square_to);
      break;

    case uo_move_type__OO:
      uo_position_do_move(position, square_from, square_to);
      uo_position_do_move(position, uo_square__h1, uo_square__f1);
      break;

    case uo_move_type__OOO:
      uo_position_do_move(position, square_from, square_to);
      uo_position_do_move(position, uo_square__a1, uo_square__d1);
      break;

    case uo_move_type__promo_N:
      uo_position_do_promo(position, square_from, uo_piece__N);
      break;

    case uo_move_type__promo_B:
      uo_position_do_promo(position, square_from, uo_piece__B);
      break;

    case uo_move_type__promo_R:
      uo_position_do_promo(position, square_from, uo_piece__R);
      break;

    case uo_move_type__promo_Q:
      uo_position_do_promo(position, square_from, uo_piece__Q);
      break;

    case uo_move_type__promo_Nx:
      piece_captured = board[square_to];
      *position->piece_captured++ = piece_captured;
      uo_position_do_promo_capture(position, square_from, square_to, uo_piece__N);
      break;

    case uo_move_type__promo_Bx:
      piece_captured = board[square_to];
      *position->piece_captured++ = piece_captured;
      uo_position_do_promo_capture(position, square_from, square_to, uo_piece__B);
      break;

    case uo_move_type__promo_Rx:
      piece_captured = board[square_to];
      *position->piece_captured++ = piece_captured;
      uo_position_do_promo_capture(position, square_from, square_to, uo_piece__R);
      break;

    case uo_move_type__promo_Qx:
      piece_captured = board[square_to];
      *position->piece_captured++ = piece_captured;
      uo_position_do_promo_capture(position, square_from, square_to, uo_piece__Q);
      break;
  }

  if (piece_captured > 1)
  {
    flags = uo_position_flags_update_rule50(flags, 0);

    if (piece_captured == uo_piece__r)
    {
      switch (square_to)
      {
        case uo_square__a8:
          flags = uo_position_flags_update_castling_q(flags, false);
          break;

        case uo_square__h8:
          flags = uo_position_flags_update_castling_k(flags, false);
          break;
      }
    }
  }

  switch (piece)
  {
    case uo_piece__P:
      flags = uo_position_flags_update_rule50(flags, 0);
      break;

    case uo_piece__K:
      flags = uo_position_flags_update_castling_own(flags, 0);
      break;

    case uo_piece__R:
      switch (square_from)
      {
        case uo_square__a1:
          flags = uo_position_flags_update_castling_Q(flags, false);
          break;

        case uo_square__h1:
          flags = uo_position_flags_update_castling_K(flags, false);
          break;
      }
  }

  assert(uo_position_is_ok(position));

  uo_position_do_switch_turn(position, flags);
  uo_position_flip_board(position);
  uo_position_update_repetitions(position);

  assert(uo_position_is_ok(position));
}

void uo_position_unmake_move(uo_position *position)
{
  uo_position_flip_board(position);

  uo_move move = uo_position_previous_move(position);
  uo_position_flags flags = position->flags;
  uo_square square_from = uo_move_square_from(move);
  uo_square square_to = uo_move_square_to(move);
  uo_move_type move_type = uo_move_get_type(move);

  switch (move_type)
  {
    case uo_move_type__quiet:
    case uo_move_type__P_double_push:
      uo_position_undo_move(position, square_from, square_to);
      break;

    case uo_move_type__x:
      uo_position_undo_capture(position, square_from, square_to, position->piece_captured[-1]);
      break;

    case uo_move_type__enpassant:
      uo_position_undo_enpassant(position, square_from, square_to);
      break;

    case uo_move_type__OO:
      uo_position_undo_move(position, square_from, square_to);
      uo_position_undo_move(position, uo_square__h1, uo_square__f1);
      break;

    case uo_move_type__OOO:
      uo_position_undo_move(position, square_from, square_to);
      uo_position_undo_move(position, uo_square__a1, uo_square__d1);
      break;

    case uo_move_type__promo_N:
    case uo_move_type__promo_B:
    case uo_move_type__promo_R:
    case uo_move_type__promo_Q:
      uo_position_undo_promo(position, square_from);
      break;

    case uo_move_type__promo_Nx:
    case uo_move_type__promo_Bx:
    case uo_move_type__promo_Rx:
    case uo_move_type__promo_Qx:
      uo_position_undo_promo_capture(position, square_from, square_to, position->piece_captured[-1]);
      break;
  }

  uo_position_undo_switch_turn(position);

  assert(uo_position_is_ok(position));
}

void uo_position_make_null_move(uo_position *position)
{
  assert(uo_position_is_ok(position));

  uo_position_flags flags = position->flags;

  uo_move_history *stack = position->stack;
  stack->flags = flags;
  stack->move = 0;
  stack->repetitions = 0;

  flags = uo_position_flags_update_enpassant_file(flags, 0);

  uo_position_do_switch_turn(position, flags);
  uo_position_flip_board(position);

  assert(uo_position_is_ok(position));
}

void uo_position_unmake_null_move(uo_position *position)
{
  uo_position_flip_board(position);
  uo_position_undo_switch_turn(position);

  assert(uo_position_is_ok(position));
}

bool uo_position_is_legal_move(uo_position *position, uo_move move)
{
  uo_piece *board = position->board;
  uo_square square_from = uo_move_square_from(move);
  uo_piece piece = board[square_from];

  if (piece <= 1) return false;
  if (uo_color(piece) != uo_color_own) return false;

  size_t move_count = uo_position_generate_moves(position);

  for (size_t i = 0; i < move_count; ++i)
  {
    if (move == position->movelist.head[i])
    {
      return true;
    }
  }

  return false;
}

static inline void uo_movegenlist_init(uo_movegenlist *movegenlist, uo_move *root, size_t tactical_move_count_guess)
{
  movegenlist->moves.tactical.head = movegenlist->moves.tactical.root = root;
  movegenlist->moves.non_tactical.head = movegenlist->moves.non_tactical.root = root + tactical_move_count_guess;
}

static inline void uo_movegenlist_insert_non_tactical_move(uo_movegenlist *movegenlist, uo_move move)
{
  *movegenlist->moves.non_tactical.head++ = move;
}

static inline void uo_movegenlist_insert_tactical_move(uo_movegenlist *movegenlist, uo_move move)
{
  if (movegenlist->moves.tactical.head == movegenlist->moves.non_tactical.root)
  {
    *movegenlist->moves.non_tactical.head++ = *movegenlist->moves.non_tactical.root++;
  }

  *movegenlist->moves.tactical.head++ = move;
}

static inline size_t uo_movegenlist_count_and_pack_generated_moves(uo_movegenlist *movegenlist, uo_move_history *stack)
{
  stack->moves_generated = true;
  size_t tactical_move_count = stack->tactical_move_count = movegenlist->moves.tactical.head - movegenlist->moves.tactical.root;
  size_t non_tactical_move_count = movegenlist->moves.non_tactical.head - movegenlist->moves.non_tactical.root;
  size_t move_count = stack->move_count = tactical_move_count + non_tactical_move_count;

  size_t gap = movegenlist->moves.non_tactical.root - movegenlist->moves.tactical.head;

  if (gap > 0)
  {
    size_t displaced_move_count = gap > non_tactical_move_count ? non_tactical_move_count : gap;
    memmove(movegenlist->moves.tactical.head, movegenlist->moves.non_tactical.head - displaced_move_count, displaced_move_count * sizeof(uo_move));
  }

  return move_count;
}

size_t uo_position_generate_moves(uo_position *position)
{
  assert(uo_position_is_ok(position));

  // Generated moves are initially order so that captures and promotions appear first in the move list

  uo_move_history *stack = position->stack;
  uo_movegenlist movegenlist;

  uo_square square_from;
  uo_square square_to;

  uo_position_flags flags = position->flags;
  uint8_t enpassant_file = uo_position_flags_enpassant_file(flags);
  uo_bitboard bitboard_enpassant_file = enpassant_file ? uo_bitboard_file[enpassant_file - 1] : 0;

  uo_piece *board = position->board;

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
  uo_bitboard enemy_BQ = enemy_B | enemy_Q;
  uo_bitboard enemy_RQ = enemy_R | enemy_Q;

  uo_bitboard attacks_enemy_P = uo_bitboard_attacks_enemy_P(enemy_P);

  // 1. Check for checks and pieces pinned to the king

  uo_square square_own_K = uo_tzcnt(own_K);

  if (!position->update_status.checks_and_pins)
  {
    position->checks.by_BQ = enemy_BQ & uo_bitboard_attacks_B(square_own_K, occupied);
    position->checks.by_RQ = enemy_RQ & uo_bitboard_attacks_R(square_own_K, occupied);
    position->checks.by_N = enemy_N & uo_bitboard_attacks_N(square_own_K);
    position->checks.by_P = enemy_P & uo_bitboard_attacks_P(square_own_K, uo_color_own);

    position->pins.by_BQ = uo_bitboard_pins_B(square_own_K, occupied, enemy_BQ);
    position->pins.by_RQ = uo_bitboard_pins_R(square_own_K, occupied, enemy_RQ);
    position->update_status.checks_and_pins = true;
  }

  uo_bitboard moves_K = uo_bitboard_moves_K(square_own_K, mask_own, mask_enemy);
  moves_K = uo_andn(attacks_enemy_P, moves_K);

  uo_bitboard enemy_checks = position->checks.by_N | position->checks.by_BQ | position->checks.by_RQ | position->checks.by_P;
  uo_bitboard pins_to_own_K_by_BQ = position->pins.by_BQ;
  uo_bitboard pins_to_own_K_by_RQ = position->pins.by_RQ;
  uo_bitboard pins_to_own_K = position->pins.by_BQ | position->pins.by_RQ;

  if (enemy_checks)
  {
    uo_movegenlist_init(&movegenlist, position->movelist.head, 1);

    uo_square square_enemy_checker = uo_tzcnt(enemy_checks);

    if (uo_popcnt(enemy_checks) == 2)
    {
      // Double check, king must move

      while (moves_K)
      {
        square_to = uo_bitboard_next_square(&moves_K);
        uo_bitboard occupied_after_move_by_K = uo_andn(own_K, occupied);
        uo_bitboard enemy_checks_BQ = enemy_BQ & uo_bitboard_attacks_B(square_to, occupied_after_move_by_K);
        uo_bitboard enemy_checks_RQ = enemy_RQ & uo_bitboard_attacks_R(square_to, occupied_after_move_by_K);
        uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(square_to);
        uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(square_to);
        uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_BQ | enemy_checks_RQ | enemy_checks_K;

        if (enemy_checks)
          continue;

        uo_piece piece_captured = board[square_to];
        if (piece_captured > 1)
        {
          uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_own_K, square_to, uo_move_type__x));
        }
        else
        {
          uo_movegenlist_insert_non_tactical_move(&movegenlist, uo_move_encode(square_own_K, square_to, uo_move_type__quiet));
        }
      }

      stack->moves_generated = true;

      size_t move_count = uo_movegenlist_count_and_pack_generated_moves(&movegenlist, stack);
      stack->move_count = move_count;
      return uo_movegenlist_count_and_pack_generated_moves(&movegenlist, stack);
    }

    // Only one piece is giving a check

    // Let's first consider captures by each piece type

    uo_bitboard attack_checker_diagonals = uo_bitboard_attacks_B(square_enemy_checker, occupied);
    uo_bitboard attack_checker_lines = uo_bitboard_attacks_R(square_enemy_checker, occupied);

    uo_bitboard attack_checker_B = uo_andn(pins_to_own_K, own_B & attack_checker_diagonals);
    while (attack_checker_B)
    {
      square_from = uo_bitboard_next_square(&attack_checker_B);
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_enemy_checker, uo_move_type__x));
    }

    uo_bitboard attack_checker_R = uo_andn(pins_to_own_K, own_R & attack_checker_lines);
    while (attack_checker_R)
    {
      square_from = uo_bitboard_next_square(&attack_checker_R);
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_enemy_checker, uo_move_type__x));
    }

    uo_bitboard attack_checker_Q = uo_andn(pins_to_own_K, own_Q & (attack_checker_diagonals | attack_checker_lines));
    while (attack_checker_Q)
    {
      square_from = uo_bitboard_next_square(&attack_checker_Q);
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_enemy_checker, uo_move_type__x));
    }

    uo_bitboard attack_checker_N = uo_andn(pins_to_own_K, own_N & uo_bitboard_attacks_N(square_enemy_checker));
    while (attack_checker_N)
    {
      square_from = uo_bitboard_next_square(&attack_checker_N);
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_enemy_checker, uo_move_type__x));
    }

    uo_bitboard attack_checker_P = uo_andn(pins_to_own_K, own_P & uo_bitboard_attacks_P(square_enemy_checker, uo_black));
    while (attack_checker_P)
    {
      square_from = uo_bitboard_next_square(&attack_checker_P);
      uo_bitboard bitboard_from = uo_square_bitboard(square_from);

      if (own_P & bitboard_from & uo_bitboard_rank_seventh)
      {
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_enemy_checker, uo_move_type__promo_Qx));
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_enemy_checker, uo_move_type__promo_Nx));
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_enemy_checker, uo_move_type__promo_Bx));
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_enemy_checker, uo_move_type__promo_Rx));
      }
      else
      {
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_enemy_checker, uo_move_type__x));
      }
    }

    // Let's still check for en passant captures
    if (position->checks.by_P && enpassant_file)
    {
      uo_bitboard non_pinned_P = uo_andn(pins_to_own_K, own_P);

      if (enpassant_file > 1 && (non_pinned_P & (position->checks.by_P >> 1)))
      {
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_enemy_checker - 1, square_enemy_checker + 8, uo_move_type__enpassant));
      }

      if (enpassant_file < 8 && (non_pinned_P & (position->checks.by_P << 1)))
      {
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_enemy_checker + 1, square_enemy_checker + 8, uo_move_type__enpassant));
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
      uo_bitboard bitboard_square_between = uo_square_bitboard(square_between);

      uo_bitboard block_diagonals = uo_bitboard_moves_B(square_between, mask_enemy, mask_own);
      uo_bitboard block_lines = uo_bitboard_moves_R(square_between, mask_enemy, mask_own);

      uo_bitboard block_B = uo_andn(pins_to_own_K, own_B & block_diagonals);
      while (block_B)
      {
        square_from = uo_bitboard_next_square(&block_B);
        uo_movegenlist_insert_non_tactical_move(&movegenlist, uo_move_encode(square_from, square_between, uo_move_type__quiet));
      }

      uo_bitboard block_R = uo_andn(pins_to_own_K, own_R & block_lines);
      while (block_R)
      {
        square_from = uo_bitboard_next_square(&block_R);
        uo_movegenlist_insert_non_tactical_move(&movegenlist, uo_move_encode(square_from, square_between, uo_move_type__quiet));
      }

      uo_bitboard block_Q = uo_andn(pins_to_own_K, own_Q & (block_diagonals | block_lines));
      while (block_Q)
      {
        square_from = uo_bitboard_next_square(&block_Q);
        uo_movegenlist_insert_non_tactical_move(&movegenlist, uo_move_encode(square_from, square_between, uo_move_type__quiet));
      }

      uo_bitboard block_N = uo_andn(pins_to_own_K, own_N & uo_bitboard_moves_N(square_between, mask_enemy, mask_own));
      while (block_N)
      {
        square_from = uo_bitboard_next_square(&block_N);
        uo_movegenlist_insert_non_tactical_move(&movegenlist, uo_move_encode(square_from, square_between, uo_move_type__quiet));
      }

      uo_bitboard block_P = own_P & uo_square_bitboard(square_between - 8);

      if ((bitboard_square_between & uo_bitboard_rank_fourth) && (uo_bitboard_double_push_P(uo_square_bitboard(square_between - 16), empty)))
      {
        block_P |= own_P & uo_square_bitboard(square_between - 16);
      }

      block_P = uo_andn(pins_to_own_K, block_P);
      while (block_P)
      {
        square_from = uo_bitboard_next_square(&block_P);
        uo_bitboard bitboard_from = uo_square_bitboard(square_from);

        if (bitboard_from & uo_bitboard_rank_seventh)
        {
          uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_between, uo_move_type__promo_Q));
          uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_between, uo_move_type__promo_N));
          uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_between, uo_move_type__promo_B));
          uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_between, uo_move_type__promo_R));
        }
        else if ((bitboard_from & uo_bitboard_rank_second) && (bitboard_square_between & uo_bitboard_rank_fourth))
        {
          uo_movegenlist_insert_non_tactical_move(&movegenlist, uo_move_encode(square_from, square_between, uo_move_type__P_double_push));
        }
        else
        {
          uo_movegenlist_insert_non_tactical_move(&movegenlist, uo_move_encode(square_from, square_between, uo_move_type__quiet));
        }
      }
    }

    // Finally, let's consider king moves

    while (moves_K)
    {
      square_to = uo_bitboard_next_square(&moves_K);
      uo_bitboard occupied_after_move_by_K = uo_andn(own_K, occupied);
      uo_bitboard enemy_checks_BQ = enemy_BQ & uo_bitboard_attacks_B(square_to, occupied_after_move_by_K);
      uo_bitboard enemy_checks_RQ = enemy_RQ & uo_bitboard_attacks_R(square_to, occupied_after_move_by_K);
      uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(square_to);
      uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(square_to);
      uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_BQ | enemy_checks_RQ | enemy_checks_K;

      if (enemy_checks)
        continue;

      uo_piece piece_captured = board[square_to];
      if (piece_captured > 1)
      {
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_own_K, square_to, uo_move_type__x));
      }
      else
      {
        uo_movegenlist_insert_non_tactical_move(&movegenlist, uo_move_encode(square_own_K, square_to, uo_move_type__quiet));
      }
    }

    return uo_movegenlist_count_and_pack_generated_moves(&movegenlist, stack);
  }

  size_t centralized_piece_count = uo_popcnt(uo_bitboard_big_center & (mask_own | mask_enemy));
  size_t tactical_move_count_guess = centralized_piece_count << 1;

  uo_movegenlist_init(&movegenlist, position->movelist.head, tactical_move_count_guess);

  // King is not in check. Let's list moves by piece type

  // First, non pinned pieces

  // Moves for non pinned knights
  uo_bitboard non_pinned_N = uo_andn(pins_to_own_K, own_N);
  while (non_pinned_N)
  {
    square_from = uo_bitboard_next_square(&non_pinned_N);
    uo_bitboard moves_N = uo_bitboard_moves_N(square_from, mask_own, mask_enemy);

    uo_bitboard captures_N = moves_N & mask_enemy;
    while (captures_N)
    {
      square_to = uo_bitboard_next_square(&captures_N);
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__x));
    }

    uo_bitboard quiets_N = moves_N & empty;
    while (quiets_N)
    {
      square_to = uo_bitboard_next_square(&quiets_N);
      uo_movegenlist_insert_non_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__quiet));
    }
  }

  // Moves for non pinned bishops
  uo_bitboard non_pinned_B = uo_andn(pins_to_own_K, own_B);
  while (non_pinned_B)
  {
    square_from = uo_bitboard_next_square(&non_pinned_B);
    uo_bitboard moves_B = uo_bitboard_moves_B(square_from, mask_own, mask_enemy);

    uo_bitboard captures_B = moves_B & mask_enemy;
    while (captures_B)
    {
      square_to = uo_bitboard_next_square(&captures_B);
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__x));
    }

    uo_bitboard quiets_B = moves_B & empty;
    while (quiets_B)
    {
      square_to = uo_bitboard_next_square(&quiets_B);
      uo_movegenlist_insert_non_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__quiet));
    }
  }

  // Moves for non pinned rooks
  uo_bitboard non_pinned_R = uo_andn(pins_to_own_K, own_R);
  while (non_pinned_R)
  {
    square_from = uo_bitboard_next_square(&non_pinned_R);
    uo_bitboard moves_R = uo_bitboard_moves_R(square_from, mask_own, mask_enemy);

    uo_bitboard captures_R = moves_R & mask_enemy;
    while (captures_R)
    {
      square_to = uo_bitboard_next_square(&captures_R);
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__x));
    }

    uo_bitboard quiets_R = moves_R & empty;
    while (quiets_R)
    {
      square_to = uo_bitboard_next_square(&quiets_R);
      uo_movegenlist_insert_non_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__quiet));
    }
  }

  // Moves for non pinned queens
  uo_bitboard non_pinned_Q = uo_andn(pins_to_own_K, own_Q);
  while (non_pinned_Q)
  {
    square_from = uo_bitboard_next_square(&non_pinned_Q);
    uo_bitboard moves_Q = uo_bitboard_moves_Q(square_from, mask_own, mask_enemy);

    uo_bitboard captures_Q = moves_Q & mask_enemy;
    while (captures_Q)
    {
      square_to = uo_bitboard_next_square(&captures_Q);
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__x));
    }

    uo_bitboard quiets_Q = moves_Q & empty;
    while (quiets_Q)
    {
      square_to = uo_bitboard_next_square(&quiets_Q);
      uo_movegenlist_insert_non_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__quiet));
    }
  }

  // Moves for non pinned pawns
  uo_bitboard non_pinned_P = uo_andn(pins_to_own_K, own_P);

  // Single pawn push
  uo_bitboard non_pinned_single_push_P = uo_bitboard_single_push_P(non_pinned_P, empty);

  while (non_pinned_single_push_P)
  {
    square_to = uo_bitboard_next_square(&non_pinned_single_push_P);
    square_from = square_to - 8;

    if (uo_square_bitboard(square_to) & uo_bitboard_rank_last)
    {
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_Q));
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_N));
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_B));
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_R));
    }
    else
    {
      uo_movegenlist_insert_non_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__quiet));
    }
  }

  // Double pawn push
  uo_bitboard non_pinned_double_push_P = uo_bitboard_double_push_P(non_pinned_P, empty);

  while (non_pinned_double_push_P)
  {
    square_to = uo_bitboard_next_square(&non_pinned_double_push_P);
    square_from = square_to - 16;
    uo_movegenlist_insert_non_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__P_double_push));
  }

  // Captures to right
  uo_bitboard non_pinned_captures_right_P = uo_bitboard_captures_right_P(non_pinned_P, mask_enemy);

  while (non_pinned_captures_right_P)
  {
    square_to = uo_bitboard_next_square(&non_pinned_captures_right_P);
    square_from = square_to - 9;

    if (uo_square_bitboard(square_to) & uo_bitboard_rank_last)
    {
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_Qx));
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_Nx));
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_Bx));
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_Rx));
    }
    else
    {
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__x));
    }
  }

  // Captures to left
  uo_bitboard non_pinned_captures_left_P = uo_bitboard_captures_left_P(non_pinned_P, mask_enemy);

  while (non_pinned_captures_left_P)
  {
    square_to = uo_bitboard_next_square(&non_pinned_captures_left_P);
    square_from = square_to - 7;

    if (uo_square_bitboard(square_to) & uo_bitboard_rank_last)
    {
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_Qx));
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_Nx));
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_Bx));
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_Rx));
    }
    else
    {
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__x));
    }
  }

  // Non pinned pawn en passant captures
  if (enpassant_file)
  {
    uo_bitboard enpassant = bitboard_enpassant_file & uo_bitboard_rank_fifth;
    square_to = uo_tzcnt(enpassant) + 8;

    if (enpassant_file > 1 && (non_pinned_P & (enpassant >> 1)))
    {
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_to - 9, square_to, uo_move_type__enpassant));
    }

    if (enpassant_file < 8 && (non_pinned_P & (enpassant << 1)))
    {
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_to - 7, square_to, uo_move_type__enpassant));
    }
  }

  // Next, moves for pinned pieces

  if (pins_to_own_K & mask_own)
  {

    // Diagonal moves for pinned bishops and queens
    uo_bitboard pinned_BQ = (own_B | own_Q) & pins_to_own_K_by_BQ;
    while (pinned_BQ)
    {
      square_from = uo_bitboard_next_square(&pinned_BQ);
      uo_bitboard moves_BQ = uo_bitboard_moves_B(square_from, mask_own, mask_enemy) & pins_to_own_K_by_BQ;

      uo_bitboard captures_BQ = moves_BQ & mask_enemy;
      while (captures_BQ)
      {
        square_to = uo_bitboard_next_square(&captures_BQ);
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__x));
      }

      uo_bitboard quiets_BQ = moves_BQ & empty;
      while (quiets_BQ)
      {
        square_to = uo_bitboard_next_square(&quiets_BQ);
        uo_movegenlist_insert_non_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__quiet));
      }
    }

    // Vertical and horizontal moves for pinned rooks and queens
    uo_bitboard pinned_RQ = (own_R | own_Q) & pins_to_own_K_by_RQ;
    while (pinned_RQ)
    {
      square_from = uo_bitboard_next_square(&pinned_RQ);
      uo_bitboard moves_RQ = uo_bitboard_moves_R(square_from, mask_own, mask_enemy) & pins_to_own_K_by_RQ;

      uo_bitboard captures_RQ = moves_RQ & mask_enemy;
      while (captures_RQ)
      {
        square_to = uo_bitboard_next_square(&captures_RQ);
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__x));
      }

      uo_bitboard quiets_RQ = moves_RQ & empty;
      while (quiets_RQ)
      {
        square_to = uo_bitboard_next_square(&quiets_RQ);
        uo_movegenlist_insert_non_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__quiet));
      }
    }

    // Moves for pinned pawns
    uo_bitboard pinned_file_P = own_P & pins_to_own_K_by_RQ & uo_square_bitboard_file[square_own_K];

    // Single pawn push
    uo_bitboard pinned_single_push_P = uo_bitboard_single_push_P(pinned_file_P, empty);
    while (pinned_single_push_P)
    {
      square_to = uo_bitboard_next_square(&pinned_single_push_P);
      square_from = square_to - 8;
      uo_bitboard bitboard_to = uo_square_bitboard(square_to);

      if (bitboard_to & uo_bitboard_rank_last)
      {
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_Q));
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_N));
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_B));
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_R));
      }
      else
      {
        uo_movegenlist_insert_non_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__quiet));
      }
    }

    // Double pawn push
    uo_bitboard pinned_double_push_P = uo_bitboard_double_push_P(pinned_file_P, empty);
    while (pinned_double_push_P)
    {
      square_to = uo_bitboard_next_square(&pinned_double_push_P);
      square_from = square_to - 16;
      uo_movegenlist_insert_non_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__P_double_push));
    }

    // Captures by pinned pawns
    uo_bitboard pinned_diag_P = own_P & pins_to_own_K_by_BQ;

    // Captures to right
    uo_bitboard pinned_captures_right_P = uo_bitboard_captures_right_P(pinned_diag_P, mask_enemy) & pins_to_own_K_by_BQ;
    while (pinned_captures_right_P)
    {
      square_to = uo_bitboard_next_square(&pinned_captures_right_P);
      square_from = square_to - 9;
      uo_bitboard bitboard_to = uo_square_bitboard(square_to);

      if (bitboard_to & uo_bitboard_rank_last)
      {
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_Qx));
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_Nx));
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_Bx));
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_Rx));
      }
      else
      {
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__x));
      }
    }

    // Captures to left
    uo_bitboard pinned_captures_left_P = uo_bitboard_captures_left_P(pinned_diag_P, mask_enemy) & pins_to_own_K_by_BQ;
    while (pinned_captures_left_P)
    {
      square_to = uo_bitboard_next_square(&pinned_captures_left_P);
      square_from = square_to - 7;
      uo_bitboard bitboard_to = uo_square_bitboard(square_to);

      if (bitboard_to & uo_bitboard_rank_last)
      {
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_Qx));
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_Nx));
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_Bx));
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__promo_Rx));
      }
      else
      {
        uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_from, square_to, uo_move_type__x));
      }
    }

    // Pinned pawn en passant captures
    if (enpassant_file)
    {
      // TODO: en passant flag should not be set if the move would be illegal

      uo_bitboard enpassant = bitboard_enpassant_file & uo_bitboard_rank_fifth;
      square_to = uo_tzcnt(enpassant) + 8;
      uo_bitboard bitboard_to = uo_square_bitboard(square_to);

      if (bitboard_to & position->pins.by_BQ)
      {
        if (enpassant_file > 1 && (pinned_diag_P & (enpassant >> 1)))
        {
          uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_to - 9, square_to, uo_move_type__enpassant));
        }

        if (enpassant_file < 8 && (pinned_diag_P & (enpassant << 1)))
        {
          uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_to - 7, square_to, uo_move_type__enpassant));
        }
      }
    }
  }

  // Finally, let's consider king moves

  // Castling moves

  if (uo_position_flags_castling_OO(flags) && !(occupied & (uo_square_bitboard(uo_square__f1) | uo_square_bitboard(uo_square__g1))))
  {
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

      if (!enemy_checks)
      {
        uo_movegenlist_insert_non_tactical_move(&movegenlist, uo_move_encode(square_own_K, uo_square__g1, uo_move_type__OO));
      }
    }
  }

  if (uo_position_flags_castling_OOO(flags) && !(occupied & (uo_square_bitboard(uo_square__d1) | uo_square_bitboard(uo_square__c1) | uo_square_bitboard(uo_square__b1))))
  {
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

      if (!enemy_checks)
      {
        uo_movegenlist_insert_non_tactical_move(&movegenlist, uo_move_encode(square_own_K, uo_square__c1, uo_move_type__OOO));
      }
    }
  }

  // Non castling king moves

  while (moves_K)
  {
    square_to = uo_bitboard_next_square(&moves_K);
    uo_bitboard occupied_after_move_by_K = uo_andn(own_K, occupied);
    uo_bitboard enemy_checks_BQ = enemy_BQ & uo_bitboard_attacks_B(square_to, occupied_after_move_by_K);
    uo_bitboard enemy_checks_RQ = enemy_RQ & uo_bitboard_attacks_R(square_to, occupied_after_move_by_K);
    uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(square_to);
    uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(square_to);
    uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_BQ | enemy_checks_RQ | enemy_checks_K;

    if (enemy_checks)
      continue;

    uo_piece piece_captured = board[square_to];
    if (piece_captured > 1)
    {
      uo_movegenlist_insert_tactical_move(&movegenlist, uo_move_encode(square_own_K, square_to, uo_move_type__x));
    }
    else
    {
      uo_movegenlist_insert_non_tactical_move(&movegenlist, uo_move_encode(square_own_K, square_to, uo_move_type__quiet));
    }
  }

  return uo_movegenlist_count_and_pack_generated_moves(&movegenlist, stack);
}

uo_move uo_position_parse_move(uo_position *const position, char str[5])
{
  if (!str)
    return (uo_move) { 0 };

  char *ptr = str;

  int file_from = ptr[0] - 'a';
  if (file_from < 0 || file_from > 7)
    return (uo_move) { 0 };
  int rank_from = ptr[1] - '1';
  if (rank_from < 0 || rank_from > 7)
    return (uo_move) { 0 };
  uo_square square_from = (rank_from << 3) + file_from;

  int file_to = ptr[2] - 'a';
  if (file_to < 0 || file_to > 7)
    return (uo_move) { 0 };
  int rank_to = ptr[3] - '1';
  if (rank_to < 0 || rank_to > 7)
    return (uo_move) { 0 };
  uo_square square_to = (rank_to << 3) + file_to;

  if (uo_color(position->flags) == uo_black)
  {
    square_from ^= 56;
    square_to ^= 56;
  }

  uo_bitboard bitboard_from = uo_square_bitboard(square_from);
  uo_bitboard bitboard_to = uo_square_bitboard(square_to);
  uo_move_type move_type = uo_move_type__quiet;
  uo_piece piece = position->board[square_from];
  uo_piece piece_captured = position->board[square_to];
  uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);

  if (piece_captured > 1)
  {
    move_type |= uo_move_type__x;
  }
  else if (piece == uo_piece__K)
  {
    if (square_to - square_from == 2)
    {
      move_type = uo_move_type__OO;
    }
    else if (square_from - square_to == 2)
    {
      move_type = uo_move_type__OOO;
    }
  }
  else if (piece == uo_piece__P)
  {
    if (uo_square_file(square_from) != uo_square_file(square_to))
    {
      move_type = uo_move_type__enpassant;
      piece_captured = uo_piece__P | !uo_color(piece);
    }
    else if (square_to - square_from == 16)
    {
      move_type = uo_move_type__P_double_push;
    }
  }

  char promo = ptr[4];
  switch (promo)
  {
    case 'q':
      move_type |= uo_move_type__promo_Q;
      break;
    case 'r':
      move_type |= uo_move_type__promo_R;
      break;
    case 'b':
      move_type |= uo_move_type__promo_B;
      break;
    case 'n':
      move_type |= uo_move_type__promo_N;
      break;
    default:
      break;
  }

  return uo_move_encode(square_from, square_to, move_type);
}

size_t uo_position_print_move(uo_position *const position, uo_move move, char str[6])
{
  if (uo_color(position->flags) == uo_black)
  {
    // flip perspective
    move ^= 0xE38;
  }

  uo_square square_from = uo_move_square_from(move);
  uo_square square_to = uo_move_square_to(move);
  uo_move_type move_type_promo = uo_move_get_type(move) & uo_move_type__promo_Q;

  sprintf(str, "%c%d%c%d",
    'a' + uo_square_file(square_from), 1 + uo_square_rank(square_from),
    'a' + uo_square_file(square_to), 1 + uo_square_rank(square_to));

  if (move_type_promo)
  {
    switch (move_type_promo)
    {
      case uo_move_type__promo_Q: str[4] = 'q'; break;
      case uo_move_type__promo_R: str[4] = 'r'; break;
      case uo_move_type__promo_B: str[4] = 'b'; break;
      case uo_move_type__promo_N: str[4] = 'n'; break;
      default: break;
    }

    str[5] = '\0';
    return 5;
  }
  else
  {
    str[4] = '\0';
    return 4;
  }
}

size_t uo_position_perft(uo_position *position, size_t depth)
{
  size_t move_count = uo_position_generate_moves(position);
  //uint64_t key = thread->position.key;

  if (depth == 1)
  {
    return move_count;
  }

  size_t node_count = 0;

  //char fen_before_make[90];
  //char fen_after_unmake[90];

  //uo_position_print_fen(position, fen_before_make);

  for (size_t i = 0; i < move_count; ++i)
  {
    uo_move move = position->movelist.head[i];
    uo_position_make_move(position, move);
    //uo_position_print_move(position, move, buf);
    //printf("move: %s\n", buf);
    //uo_position_print_fen(position, buf);
    //printf("%s\n", buf);
    //uo_position_print_diagram(position, buf);
    //printf("%s\n", buf);
    node_count += uo_position_perft(position, depth - 1);
    uo_position_unmake_move(position);
    //assert(thread->position.key == key);

  //  uo_position_print_fen(position, fen_after_unmake);

  //  if (strcmp(fen_before_make, fen_after_unmake) != 0 /* || key != thread->position.key */)
  //  {
  //    uo_position position;
  //    uo_position_from_fen(&position, fen_before_make);

  //    uo_position_print_move(position, move, buf);
  //    printf("error when unmaking move: %s\n", buf);
  //    printf("\nbefore make move\n");
  //    uo_position_print_diagram(&position, buf);
  //    printf("\n%s", buf);
  //    printf("\n");
  //    printf("Fen: %s\n", fen_before_make);
  //    printf("Key: %" PRIu64 "\n", key);
  //    printf("\nafter unmake move\n");
  //    uo_position_print_diagram(position, buf);
  //    printf("\n%s", buf);
  //    printf("\n");
  //    printf("Fen: %s\n", fen_after_unmake);
  //    printf("Key: %" PRIu64 "\n", thread->position.key);
  //    printf("\n");
  //  }
  }

  return node_count;
}
