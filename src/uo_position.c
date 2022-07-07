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

//const size_t uo_position__piece_bitboard_offset[0x22] = {
//    [uo_piece__P] = offsetof(uo_position, P),
//    [uo_piece__N] = offsetof(uo_position, N),
//    [uo_piece__B] = offsetof(uo_position, B),
//    [uo_piece__R] = offsetof(uo_position, R),
//    [uo_piece__Q] = offsetof(uo_position, Q),
//    [uo_piece__K] = offsetof(uo_position, K),
//    [uo_piece__p] = offsetof(uo_position, P),
//    [uo_piece__n] = offsetof(uo_position, N),
//    [uo_piece__b] = offsetof(uo_position, B),
//    [uo_piece__r] = offsetof(uo_position, R),
//    [uo_piece__q] = offsetof(uo_position, Q),
//    [uo_piece__k] = offsetof(uo_position, K)
//};

//bool uo_position_is_ok(uo_position *position)
//{
//  for (uo_square square = 0; square < 64; ++square)
//  {
//    uo_bitboard mask = uo_square_bitboard(square);
//    uo_piece piece = position->board[square];
//
//    if (piece == 0)
//    {
//      if (position->P & mask)
//      {
//        return false;
//      }
//
//      if (position->N & mask)
//      {
//        return false;
//      }
//
//      if (position->B & mask)
//      {
//        return false;
//      }
//
//      if (position->R & mask)
//      {
//        return false;
//      }
//
//      if (position->Q & mask)
//      {
//        return false;
//      }
//
//      if (position->K & mask)
//      {
//        return false;
//      }
//    }
//    else
//    {
//      uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);
//
//      if (!(*bitboard & mask))
//      {
//        return false;
//      }
//
//      if (bitboard != &position->P && position->P & mask)
//      {
//        return false;
//      }
//
//      if (bitboard != &position->N && position->N & mask)
//      {
//        return false;
//      }
//
//      if (bitboard != &position->B && position->B & mask)
//      {
//        return false;
//      }
//
//      if (bitboard != &position->R && position->R & mask)
//      {
//        return false;
//      }
//
//      if (bitboard != &position->Q && position->Q & mask)
//      {
//        return false;
//      }
//
//      if (bitboard != &position->K && position->K & mask)
//      {
//        return false;
//      }
//
//      if (uo_color(piece) == uo_white && (position->piece_color & mask))
//      {
//        return false;
//      }
//
//      if (uo_color(piece) == uo_black && !(position->piece_color & mask))
//      {
//        return false;
//      }
//    }
//  }
//
//  return true;
//}

#pragma region uo_side_consts

typedef const struct uo_pieces {
  uo_piece P;
  uo_piece N;
  uo_piece B;
  uo_piece R;
  uo_piece Q;
  uo_piece K;
} uo_pieces;

typedef const struct uo_side_consts {
  const uo_pieces pieces;
  const struct {
    uint8_t first;
    uint8_t second;
    uint8_t third;
    uint8_t fourth;
    uint8_t fifth;
    uint8_t sixth;
    uint8_t seventh;
    uint8_t eight;
  } ranks;
  const struct {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t f;
    uint8_t g;
    uint8_t h;
  } first_rank_squares;
  bool (*can_castle_OO)(uo_position_flags flags);
  bool (*can_castle_OOO)(uo_position_flags flags);
} uo_side_consts;

static const uo_side_consts uo_consts_by_color[2] = {
  [uo_white] = {
    .pieces = {
      .P = uo_piece__P,
      .N = uo_piece__N,
      .B = uo_piece__B,
      .R = uo_piece__R,
      .Q = uo_piece__Q,
      .K = uo_piece__K
    },
    .ranks = {0,1,2,3,4,5,6,7},
    .first_rank_squares = {
      .a = uo_square__a1,
      .b = uo_square__b1,
      .c = uo_square__c1,
      .d = uo_square__d1,
      .e = uo_square__e1,
      .f = uo_square__f1,
      .g = uo_square__g1,
      .h = uo_square__h1,
    },
    .can_castle_OO = uo_position_flags_castling_K,
    .can_castle_OOO = uo_position_flags_castling_Q,
  },
  [uo_black] = {
    .pieces = {
      .P = uo_piece__p,
      .N = uo_piece__n,
      .B = uo_piece__b,
      .R = uo_piece__r,
      .Q = uo_piece__q,
      .K = uo_piece__k
    },
    .ranks = {7,6,5,4,3,2,1,0},
    .first_rank_squares = {
      .a = uo_square__a8,
      .b = uo_square__b8,
      .c = uo_square__c8,
      .d = uo_square__d8,
      .e = uo_square__e8,
      .f = uo_square__f8,
      .g = uo_square__g8,
      .h = uo_square__h8,
    },
    .can_castle_OO = uo_position_flags_castling_k,
    .can_castle_OOO = uo_position_flags_castling_q,
  }
};

#pragma endregion


uint64_t uo_position_calculate_key(uo_position *position)
{
  uint64_t key = 0;

  uo_position_flags flags = position->flags;
  uint8_t color_to_move = uo_position_flags_color_to_move(flags);
  uint8_t castling = uo_position_flags_castling(flags);
  uint8_t enpassant_file = uo_position_flags_enpassant_file(flags);

  if (color_to_move == uo_black)
  {
    key ^= uo_zobrist_color_to_move;
  }

  key ^= uo_zobrist_castling[castling];

  if (enpassant_file)
  {
    key ^= uo_zobrist_enpassant_file[enpassant_file - 1];
  }

  for (uo_square square = 0; square < 64; ++square)
  {
    uo_piece piece = position->board[square];
    if (piece)
    {
      key ^= uo_zobrist[((size_t)piece << 6) + square];
    }
  }

  return key;
}

static inline void uo_position_flip_board(uo_position *position)
{
  uint64_t *board = (uint64_t *)position->board;

  for (int i = 0; i < 4; ++i)
  {
    uint64_t temp = board[i];
    board[i] = board[7 - i];
    board[7 - i] = temp;
  }
}

static inline void uo_position_set_flags(uo_position *position, uo_position_flags flags)
{
  uint64_t key = position->key;

  uint8_t enpassant_file = uo_position_flags_enpassant_file(position->flags);
  if (enpassant_file)
  {
    key ^= uo_zobrist_enpassant_file[enpassant_file - 1];
  }

  enpassant_file = uo_position_flags_enpassant_file(flags);
  if (enpassant_file)
  {
    key ^= uo_zobrist_enpassant_file[enpassant_file - 1];
  }

  uint8_t castling = uo_position_flags_castling(position->flags);
  key ^= uo_zobrist_castling[castling - 1];
  castling = uo_position_flags_castling(flags);
  key ^= uo_zobrist_castling[castling - 1];

  position->key = key;
  position->flags = flags;
}

static inline void uo_position_do_switch_turn(uo_position *position, uo_position_flags flags)
{
  uo_position_set_flags(position, flags);
  position->flags ^= 1;
  position->key ^= uo_zobrist_color_to_move;
  //position->board_xor ^= 56;
  ++position->ply;
}
static inline void uo_position_undo_switch_turn(uo_position *position)
{
  position->key ^= uo_zobrist_color_to_move;
  //position->board_xor ^= 56;
  --position->ply;

  uo_position_flags flags = *--position->history.flags;
  uo_position_set_flags(position, flags);

  uo_move move = *--position->history.move;
  if (uo_move_is_capture(move))
  {
    --position->history.piece_captured;
  }
}

static inline void uo_position_do_move(uo_position *position, uo_square square_from, uo_square square_to)
{
  uo_piece *board = position->board;
  uo_piece piece = board[square_from];
  board[square_from] = 0;
  board[square_to] = piece;

  size_t zpiece = (size_t)piece << 6;
  position->key ^= uo_zobrist[zpiece + square_from] ^ uo_zobrist[zpiece + square_to];

  uo_bitboard bitboard_from = uo_square_bitboard(square_from);
  uo_bitboard bitboard_to = uo_square_bitboard(square_to);

  uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);
  *bitboard ^= bitboard_from | bitboard_to;
  uo_bitboard *bitboard_color = uo_position_piece_bitboard(position, uo_color(piece));
  *bitboard_color ^= bitboard_from | bitboard_to;
}
static inline void uo_position_undo_move(uo_position *position, uo_square square_from, uo_square square_to)
{
  uo_piece *board = position->board;
  uo_piece piece = board[square_to];
  board[square_from] = piece;
  board[square_to] = 0;

  size_t zpiece = (size_t)piece << 6;
  position->key ^= uo_zobrist[zpiece + square_from] ^ uo_zobrist[zpiece + square_to];

  uo_bitboard bitboard_from = uo_square_bitboard(square_from);
  uo_bitboard bitboard_to = uo_square_bitboard(square_to);

  uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);
  *bitboard ^= bitboard_from | bitboard_to;
  uo_bitboard *bitboard_color = uo_position_piece_bitboard(position, uo_color(piece));
  *bitboard_color ^= bitboard_from | bitboard_to;
}

static inline void uo_position_do_capture(uo_position *position, uo_square square_from, uo_square square_to)
{
  uo_piece *board = position->board;
  uo_piece piece = board[square_from];
  uo_piece piece_captured = board[square_to];
  board[square_from] = 0;
  board[square_to] = piece;

  size_t zpiece = (size_t)piece << 6;
  size_t zpiece_captured = (size_t)piece_captured << 6;
  position->key ^= uo_zobrist[zpiece + square_from] ^ uo_zobrist[zpiece + square_to] ^ uo_zobrist[zpiece_captured + square_to];

  uo_bitboard bitboard_from = uo_square_bitboard(square_from);
  uo_bitboard bitboard_to = uo_square_bitboard(square_to);

  uo_bitboard *bitboard_captured = uo_position_piece_bitboard(position, piece_captured);
  *bitboard_captured = uo_andn(bitboard_to, *bitboard_captured);
  uo_bitboard *bitboard_color_captured = uo_position_piece_bitboard(position, uo_color(piece_captured));
  *bitboard_color_captured = uo_andn(bitboard_to, *bitboard_color_captured);

  uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);
  *bitboard ^= bitboard_from | bitboard_to;
  uo_bitboard *bitboard_color = uo_position_piece_bitboard(position, uo_color(piece));
  *bitboard_color ^= bitboard_from | bitboard_to;
}
static inline void uo_position_undo_capture(uo_position *position, uo_square square_from, uo_square square_to, uo_piece piece_captured)
{
  uo_piece *board = position->board;
  uo_piece piece = board[square_to];
  board[square_from] = piece;
  board[square_to] = piece_captured;

  size_t zpiece = (size_t)piece << 6;
  size_t zpiece_captured = (size_t)piece_captured << 6;
  position->key ^= uo_zobrist[zpiece + square_from] ^ uo_zobrist[zpiece + square_to] ^ uo_zobrist[zpiece_captured + square_to];

  uo_bitboard bitboard_from = uo_square_bitboard(square_from);
  uo_bitboard bitboard_to = uo_square_bitboard(square_to);

  uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);
  *bitboard ^= bitboard_from | bitboard_to;
  uo_bitboard *bitboard_color = uo_position_piece_bitboard(position, uo_color(piece));
  *bitboard_color ^= bitboard_from | bitboard_to;

  uo_bitboard *bitboard_captured = uo_position_piece_bitboard(position, piece_captured);
  *bitboard_captured |= bitboard_to;
  uo_bitboard *bitboard_color_captured = uo_position_piece_bitboard(position, uo_color(piece_captured));
  *bitboard_color_captured |= bitboard_to;
}

static inline void uo_position_do_enpassant(uo_position *position, uo_square square_from, uo_square square_to)
{
  uo_position_do_move(position, square_from, square_to);

  uo_piece *board = position->board;
  uo_piece piece = board[square_to];
  uo_square square_piece_captured = square_to + uo_direction_backwards(piece);
  uo_piece piece_captured = position->board[square_piece_captured];
  board[square_piece_captured] = 0;

  size_t zpiece_captured = (size_t)piece_captured << 6;
  position->key ^= uo_zobrist[zpiece_captured + square_piece_captured];

  uo_bitboard enpassant = uo_square_bitboard(square_piece_captured);
  uo_bitboard *bitboard_captured = uo_position_piece_bitboard(position, piece_captured);
  *bitboard_captured = uo_andn(enpassant, *bitboard_captured);
  uo_bitboard *bitboard_color_captured = uo_position_piece_bitboard(position, uo_color(piece_captured));
  *bitboard_color_captured = uo_andn(enpassant, *bitboard_color_captured);
}
static inline void uo_position_undo_enpassant(uo_position *position, uo_square square_from, uo_square square_to)
{
  uo_position_undo_move(position, square_from, square_to);

  uo_piece *board = position->board;
  uo_piece piece = board[square_from];
  uo_square square_piece_captured = square_to + uo_direction_backwards(piece);
  uo_piece piece_captured = piece ^ 1;
  position->board[square_piece_captured] = piece_captured;

  size_t zpiece_captured = (size_t)piece_captured << 6;
  position->key ^= uo_zobrist[zpiece_captured + square_piece_captured];

  uo_bitboard enpassant = uo_square_bitboard(square_piece_captured);
  uo_bitboard *bitboard_captured = uo_position_piece_bitboard(position, piece_captured);
  *bitboard_captured |= enpassant;
  uo_bitboard *bitboard_color_captured = uo_position_piece_bitboard(position, uo_color(piece_captured));
  *bitboard_color_captured |= enpassant;
}

static inline void uo_position_do_promo(uo_position *position, uo_square square_from, uo_piece piece)
{
  uo_piece *board = position->board;
  uo_piece pawn = board[square_from];
  uo_square square_to = square_from + uo_direction_forward(piece);
  board[square_from] = 0;
  board[square_to] = piece;

  size_t zpawn = (size_t)pawn << 6;
  size_t zpiece = (size_t)piece << 6;
  position->key ^= uo_zobrist[zpawn + square_from] ^ uo_zobrist[zpiece + square_to];

  uo_bitboard bitboard_from = uo_square_bitboard(square_from);
  uo_bitboard bitboard_to = uo_square_bitboard(square_to);

  uo_bitboard *bitboard_pawn = uo_position_piece_bitboard(position, pawn);
  *bitboard_pawn = uo_andn(bitboard_from, *bitboard_pawn);

  uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);
  *bitboard |= bitboard_to;

  uo_bitboard *bitboard_color = uo_position_piece_bitboard(position, uo_color(piece));
  *bitboard_color ^= bitboard_from | bitboard_to;
}
static inline void uo_position_undo_promo(uo_position *position, uo_square square_from)
{
  uo_square square_to = square_from + uo_direction_backwards(position->flags);

  uo_piece *board = position->board;
  uo_piece piece = board[square_to];
  uo_piece pawn = uo_piece__P | uo_color(piece);
  board[square_from] = pawn;
  board[square_to] = 0;

  size_t zpawn = (size_t)pawn << 6;
  size_t zpiece = (size_t)piece << 6;
  position->key ^= uo_zobrist[zpawn + square_from] ^ uo_zobrist[zpiece + square_to];

  uo_bitboard bitboard_from = uo_square_bitboard(square_from);
  uo_bitboard bitboard_to = uo_square_bitboard(square_to);

  uo_bitboard *bitboard_pawn = uo_position_piece_bitboard(position, pawn);
  *bitboard_pawn |= bitboard_from;

  uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);
  *bitboard = uo_andn(bitboard_to, *bitboard);

  uo_bitboard *bitboard_color = uo_position_piece_bitboard(position, uo_color(piece));
  *bitboard_color ^= bitboard_from | bitboard_to;
}

static inline void uo_position_do_promo_capture(uo_position *position, uo_square square_from, uo_square square_to, uo_piece piece)
{
  uo_piece *board = position->board;
  uo_piece pawn = board[square_from];
  uo_piece piece_captured = board[square_to];
  board[square_from] = 0;
  board[square_to] = piece;

  size_t zpawn = (size_t)pawn << 6;
  size_t zpiece = (size_t)piece << 6;
  size_t zpiece_captured = (size_t)piece_captured << 6;
  position->key ^= uo_zobrist[zpawn + square_from] ^ uo_zobrist[zpiece + square_to] ^ uo_zobrist[zpiece_captured + square_to];

  uo_bitboard bitboard_from = uo_square_bitboard(square_from);
  uo_bitboard bitboard_to = uo_square_bitboard(square_to);

  uo_bitboard *bitboard_captured = uo_position_piece_bitboard(position, piece_captured);
  *bitboard_captured = uo_andn(bitboard_to, *bitboard_captured);

  uo_bitboard *bitboard_color_captured = uo_position_piece_bitboard(position, uo_color(piece_captured));
  *bitboard_color_captured = uo_andn(bitboard_to, *bitboard_color_captured);

  uo_bitboard *bitboard_pawn = uo_position_piece_bitboard(position, pawn);
  *bitboard_pawn = uo_andn(bitboard_from, *bitboard_pawn);

  uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);
  *bitboard |= bitboard_to;

  uo_bitboard *bitboard_color = uo_position_piece_bitboard(position, uo_color(piece));
  *bitboard_color ^= bitboard_from | bitboard_to;
}
static inline void uo_position_undo_promo_capture(uo_position *position, uo_square square_from, uo_square square_to, uo_piece piece_captured)
{
  uo_piece *board = position->board;
  uo_piece piece = board[square_to];
  uo_piece pawn = uo_piece__P | uo_color(piece);
  board[square_from] = pawn;
  board[square_to] = piece_captured;

  size_t zpawn = (size_t)pawn << 6;
  size_t zpiece = (size_t)piece << 6;
  size_t zpiece_captured = (size_t)piece_captured << 6;
  position->key ^= uo_zobrist[zpawn + square_from] ^ uo_zobrist[zpiece + square_to] ^ uo_zobrist[zpiece_captured + square_to];

  uo_bitboard bitboard_from = uo_square_bitboard(square_from);
  uo_bitboard bitboard_to = uo_square_bitboard(square_to);

  uo_bitboard *bitboard_pawn = uo_position_piece_bitboard(position, pawn);
  *bitboard_pawn |= bitboard_from;

  uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);
  *bitboard = uo_andn(bitboard_to, *bitboard);
  uo_bitboard *bitboard_color = uo_position_piece_bitboard(position, uo_color(piece));
  *bitboard_color ^= bitboard_from | bitboard_to;

  uo_bitboard *bitboard_captured = uo_position_piece_bitboard(position, piece_captured);
  *bitboard_captured |= bitboard_to;
  uo_bitboard *bitboard_color_captured = uo_position_piece_bitboard(position, uo_color(piece_captured));
  *bitboard_color_captured |= bitboard_to;
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
      uo_bitboard *bitboard_color = uo_position_piece_bitboard(position, uo_color(piece));
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
    ++position->ply;
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
  position->ply += fullmove << 1;

  position->flags = flags;
  position->key = uo_position_calculate_key(position);
  position->history.move = position->history.moves;
  position->history.piece_captured = position->history.captures;
  position->history.flags = position->history.prev_flags;

  return position;
}

size_t uo_position_print_fen(uo_position *position, char fen[90])
{
  char *ptr = fen;

  for (int rank = 7; rank >= 0; --rank)
  {
    int empty_count = 0;
    for (int file = 0; file < 8; ++file)
    {
      uo_square square = (rank << 3) + file;
      uo_piece piece = position->board[square];

      if (piece)
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

  uo_position_flags flags = position->flags;

  uint8_t color_to_move = uo_position_flags_color_to_move(flags);
  ptr += sprintf(ptr, " %c ", color_to_move ? 'b' : 'w');

  uint8_t castling = uo_position_flags_castling(flags);

  if (castling)
  {
    uint8_t castling_K = uo_position_flags_castling_K(flags);
    if (castling_K)
      ptr += sprintf(ptr, "%c", 'K');

    uint8_t castling_Q = uo_position_flags_castling_Q(flags);
    if (castling_Q)
      ptr += sprintf(ptr, "%c", 'Q');

    uint8_t castling_k = uo_position_flags_castling_k(flags);
    if (castling_k)
      ptr += sprintf(ptr, "%c", 'k');

    uint8_t castling_q = uo_position_flags_castling_q(flags);
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
  uint16_t fullmoves = (position->ply >> 1) + 1;

  ptr += sprintf(ptr, "%d %d", rule50, fullmoves);

  return ptr - fen;
}

size_t uo_position_print_diagram(uo_position *position, char diagram[663])
{
  char *ptr = diagram;

  for (int rank = 7; rank >= 0; --rank)
  {
    ptr += sprintf(ptr, " +---+---+---+---+---+---+---+---+\n |");
    for (int file = 0; file < 8; ++file)
    {
      uo_square square = (rank << 3) + file;
      uo_piece piece = position->board[square];
      ptr += sprintf(ptr, " %c |", piece ? uo_piece_to_char(piece) : ' ');
    }
    ptr += sprintf(ptr, " %d\n", rank + 1);
  }

  ptr += sprintf(ptr, " +---+---+---+---+---+---+---+---+\n");
  ptr += sprintf(ptr, "   a   b   c   d   e   f   g   h\n");

  return ptr - diagram;
}

void uo_position_make_move(uo_position *position, uo_move move)
{
  uo_square square_from = uo_move_square_from(move);
  uo_square square_to = uo_move_square_to(move);
  uo_piece *board = position->board;
  uo_piece piece = board[square_from];
  uo_piece piece_captured = 0;

  uo_position_flags flags = position->flags;

  *position->history.flags++ = flags;
  *position->history.move++ = move;

  uint8_t color_to_move = uo_position_flags_color_to_move(flags);
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
      uint8_t rank_fourth = uo_rank_fourth(flags);
      uo_bitboard mask_enemy_P = *uo_position_piece_bitboard(position, uo_piece__p ^ color_to_move);

      uo_bitboard adjecent_enemy_pawn = ((bitboard_to << 1) | (bitboard_to >> 1)) & uo_bitboard_rank[rank_fourth] & mask_enemy_P;
      if (adjecent_enemy_pawn)
      {
        uo_bitboard mask_enemy_K = *uo_position_piece_bitboard(position, uo_piece__k ^ color_to_move);
        uo_square square_enemy_K = uo_tzcnt(mask_enemy_K);
        uint8_t rank_enemy_K = uo_square_rank(square_enemy_K);

        if (rank_enemy_K == rank_fourth && uo_popcnt(adjecent_enemy_pawn) == 1)
        {
          uo_bitboard bitboard_rank_enemy_K = uo_bitboard_rank[rank_enemy_K];
          uo_bitboard occupied = uo_andn(uo_bitboard_file[uo_square_file(square_to)], position->white | position->black);
          uo_bitboard mask_own_R = *uo_position_piece_bitboard(position, uo_piece__R | color_to_move);
          uo_bitboard mask_own_Q = *uo_position_piece_bitboard(position, uo_piece__Q | color_to_move);
          uo_bitboard rank_pins_to_enemy_K = uo_bitboard_pins_R(square_enemy_K, occupied, mask_own_R | mask_own_Q) & bitboard_rank_enemy_K;

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
      *position->history.piece_captured++ = piece_captured;
      uo_position_do_capture(position, square_from, square_to);
      break;

    case uo_move_type__enpassant:
      piece_captured = board[square_to];
      *position->history.piece_captured++ = piece_captured;
      uo_position_do_enpassant(position, square_from, square_to);
      break;

    case uo_move_type__OO:
      uo_position_do_move(position, square_from, square_to);
      if (color_to_move == uo_white)
      {
        uo_position_do_move(position, uo_square__h1, uo_square__f1);
      }
      else
      {
        uo_position_do_move(position, uo_square__h8, uo_square__f8);
      }
      break;

    case uo_move_type__OOO:
      uo_position_do_move(position, square_from, square_to);
      if (color_to_move == uo_white)
      {
        uo_position_do_move(position, uo_square__a1, uo_square__d1);
      }
      else
      {
        uo_position_do_move(position, uo_square__a8, uo_square__d8);
      }
      break;

    case uo_move_type__promo_N:
      uo_position_do_promo(position, square_from, uo_piece__N | color_to_move);
      break;

    case uo_move_type__promo_B:
      uo_position_do_promo(position, square_from, uo_piece__B | color_to_move);
      break;

    case uo_move_type__promo_R:
      uo_position_do_promo(position, square_from, uo_piece__R | color_to_move);
      break;

    case uo_move_type__promo_Q:
      uo_position_do_promo(position, square_from, uo_piece__Q | color_to_move);
      break;

    case uo_move_type__promo_Nx:
      piece_captured = board[square_to];
      *position->history.piece_captured++ = piece_captured;
      uo_position_do_promo_capture(position, square_from, square_to, uo_piece__N | color_to_move);
      break;

    case uo_move_type__promo_Bx:
      piece_captured = board[square_to];
      *position->history.piece_captured++ = piece_captured;
      uo_position_do_promo_capture(position, square_from, square_to, uo_piece__B | color_to_move);
      break;

    case uo_move_type__promo_Rx:
      piece_captured = board[square_to];
      *position->history.piece_captured++ = piece_captured;
      uo_position_do_promo_capture(position, square_from, square_to, uo_piece__R | color_to_move);
      break;

    case uo_move_type__promo_Qx:
      piece_captured = board[square_to];
      *position->history.piece_captured++ = piece_captured;
      uo_position_do_promo_capture(position, square_from, square_to, uo_piece__Q | color_to_move);
      break;
  }

  if (piece_captured)
  {
    flags = uo_position_flags_update_rule50(flags, 0);

    if (uo_piece_type(piece_captured) == uo_piece__R)
    {
      switch (square_to)
      {
        case uo_square__a1:
          flags = uo_position_flags_update_castling_Q(flags, false);
          break;

        case uo_square__h1:
          flags = uo_position_flags_update_castling_K(flags, false);
          break;

        case uo_square__a8:
          flags = uo_position_flags_update_castling_q(flags, false);
          break;

        case uo_square__h8:
          flags = uo_position_flags_update_castling_k(flags, false);
          break;
      }
    }
  }

  switch (uo_piece_type(piece))
  {
    case uo_piece__P:
      flags = uo_position_flags_update_rule50(flags, 0);
      break;

    case uo_piece__K:
      if (color_to_move == uo_white)
      {
        flags = uo_position_flags_update_castling_white(flags, 0);
      }
      else
      {
        flags = uo_position_flags_update_castling_black(flags, 0);
      }
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

        case uo_square__a8:
          flags = uo_position_flags_update_castling_q(flags, false);
          break;

        case uo_square__h8:
          flags = uo_position_flags_update_castling_k(flags, false);
          break;
      }
  }

  uo_position_do_switch_turn(position, flags);
}

void uo_position_unmake_move(uo_position *position)
{
  uo_move move = position->history.move[-1];
  uo_position_flags flags = position->flags;
  uint8_t color_to_move = !uo_position_flags_color_to_move(flags);
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
      uo_position_undo_capture(position, square_from, square_to, position->history.piece_captured[-1]);
      break;

    case uo_move_type__enpassant:
      uo_position_undo_enpassant(position, square_from, square_to);
      break;

    case uo_move_type__OO:
      uo_position_undo_move(position, square_from, square_to);
      if (color_to_move == uo_white)
      {
        uo_position_undo_move(position, uo_square__h1, uo_square__f1);
      }
      else
      {
        uo_position_undo_move(position, uo_square__h8, uo_square__f8);
      }
      break;

    case uo_move_type__OOO:
      uo_position_undo_move(position, square_from, square_to);
      if (color_to_move == uo_white)
      {
        uo_position_undo_move(position, uo_square__a1, uo_square__d1);
      }
      else
      {
        uo_position_undo_move(position, uo_square__a8, uo_square__d8);
      }
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
      uo_position_undo_promo_capture(position, square_from, square_to, position->history.piece_captured[-1]);
      break;
  }

  uo_position_undo_switch_turn(position);
}

bool uo_position_is_legal_move(uo_position *position, uo_move move)
{
  uo_piece *board = position->board;
  uo_square square_from = uo_move_square_from(move);
  uo_piece piece = board[square_from];
  if (!piece) return false;

  uint8_t color_to_move = uo_position_flags_color_to_move(position->flags);
  if (uo_color(piece) != color_to_move) return false;

  //uo_move_type move_type = uo_move_get_type(move);
  //uo_square square_to = uo_move_square_to(move);
  //uo_piece piece_captured = board[square_to];

  uo_position_make_move(position, move);

  position->flags ^= 1;

  if (uo_position_is_check(position))
  {
    position->flags ^= 1;
    return false;
  }

  position->flags ^= 1;
  uo_position_unmake_move(position);
  return true;
}

size_t uo_position_get_moves(uo_position *position, uo_move *movelist)
{
  uo_move *moves = movelist;
  uo_square square_from;
  uo_square square_to;

  uo_position_flags flags = position->flags;
  uint8_t color_to_move = uo_position_flags_color_to_move(flags);
  uint8_t castling = uo_position_flags_castling(flags);
  uint8_t enpassant_file = uo_position_flags_enpassant_file(flags);
  uo_bitboard bitboard_enpassant_file = enpassant_file ? uo_bitboard_file[enpassant_file - 1] : 0;

  uo_side_consts own = uo_consts_by_color[color_to_move];
  uo_pieces pieces_enemy = uo_consts_by_color[!color_to_move].pieces;

  uo_piece *board = position->board;

  uo_bitboard occupied = position->white | position->black;
  uo_bitboard empty = ~occupied;
  uo_bitboard mask_own = *uo_position_piece_bitboard(position, color_to_move);
  uo_bitboard mask_enemy = *uo_position_piece_bitboard(position, !color_to_move);

  uo_bitboard own_P = *uo_position_piece_bitboard(position, own.pieces.P);
  uo_bitboard own_N = *uo_position_piece_bitboard(position, own.pieces.N);
  uo_bitboard own_B = *uo_position_piece_bitboard(position, own.pieces.B);
  uo_bitboard own_R = *uo_position_piece_bitboard(position, own.pieces.R);
  uo_bitboard own_Q = *uo_position_piece_bitboard(position, own.pieces.Q);
  uo_bitboard own_K = *uo_position_piece_bitboard(position, own.pieces.K);
  uo_bitboard enemy_P = *uo_position_piece_bitboard(position, pieces_enemy.P);
  uo_bitboard enemy_N = *uo_position_piece_bitboard(position, pieces_enemy.N);
  uo_bitboard enemy_B = *uo_position_piece_bitboard(position, pieces_enemy.B);
  uo_bitboard enemy_R = *uo_position_piece_bitboard(position, pieces_enemy.R);
  uo_bitboard enemy_Q = *uo_position_piece_bitboard(position, pieces_enemy.Q);
  uo_bitboard enemy_K = *uo_position_piece_bitboard(position, pieces_enemy.K);
  uo_bitboard enemy_BQ = enemy_B | enemy_Q;
  uo_bitboard enemy_RQ = enemy_R | enemy_Q;

  uo_bitboard bitboard_first_rank = uo_bitboard_rank[own.ranks.first];
  uo_bitboard bitboard_second_rank = uo_bitboard_rank[own.ranks.second];
  uo_bitboard bitboard_fourth_rank = uo_bitboard_rank[own.ranks.fourth];
  uo_bitboard bitboard_fifth_rank = uo_bitboard_rank[own.ranks.fifth];
  uo_bitboard bitboard_seventh_rank = uo_bitboard_rank[own.ranks.seventh];
  uo_bitboard bitboard_last_rank = uo_bitboard_rank[own.ranks.eight];

  int8_t direction_forwards = uo_direction_forward(color_to_move);
  int8_t direction_backwards = -direction_forwards;

  // 1. Check for checks and pieces pinned to the king

  uo_square square_own_K = uo_tzcnt(own_K);
  uint8_t file_own_K = uo_square_file(square_own_K);
  uint8_t rank_own_K = uo_square_rank(square_own_K);
  uo_bitboard moves_K = uo_bitboard_moves_K(square_own_K, mask_own, mask_enemy);

  uo_bitboard enemy_checks_BQ = enemy_BQ & uo_bitboard_attacks_B(square_own_K, occupied);
  uo_bitboard enemy_checks_RQ = enemy_RQ & uo_bitboard_attacks_R(square_own_K, occupied);
  uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(square_own_K);
  uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(square_own_K, color_to_move);
  uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_BQ | enemy_checks_RQ | enemy_checks_P;

  uo_bitboard pins_to_own_K_by_B = uo_bitboard_pins_B(square_own_K, occupied, enemy_BQ);
  uo_bitboard pins_to_own_K_by_R = uo_bitboard_pins_R(square_own_K, occupied, enemy_RQ);
  uo_bitboard pins_to_own_K = pins_to_own_K_by_B | pins_to_own_K_by_R;

  if (enemy_checks)
  {
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
        uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(square_to, color_to_move);
        uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(square_to);
        uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_BQ | enemy_checks_RQ | enemy_checks_P | enemy_checks_K;

        if (enemy_checks)
          continue;

        uo_piece piece_captured = board[square_to];
        uo_move_type move_type = piece_captured ? uo_move_type__x : uo_move_type__quiet;

        *moves++ = uo_move_encode(square_own_K, square_to, move_type);
      }

      return moves - movelist;
    }

    // Only one piece is giving a check

    // Let's first consider captures by each piece type

    uo_bitboard attack_checker_diagonals = uo_bitboard_attacks_B(square_enemy_checker, occupied);
    uo_bitboard attack_checker_lines = uo_bitboard_attacks_R(square_enemy_checker, occupied);

    uo_bitboard attack_checker_B = uo_andn(pins_to_own_K, own_B & attack_checker_diagonals);
    while (attack_checker_B)
    {
      square_from = uo_bitboard_next_square(&attack_checker_B);
      *moves++ = uo_move_encode(square_from, square_enemy_checker, uo_move_type__x);
    }

    uo_bitboard attack_checker_R = uo_andn(pins_to_own_K, own_R & attack_checker_lines);
    while (attack_checker_R)
    {
      square_from = uo_bitboard_next_square(&attack_checker_R);
      *moves++ = uo_move_encode(square_from, square_enemy_checker, uo_move_type__x);
    }

    uo_bitboard attack_checker_Q = uo_andn(pins_to_own_K, own_Q & (attack_checker_diagonals | attack_checker_lines));
    while (attack_checker_Q)
    {
      square_from = uo_bitboard_next_square(&attack_checker_Q);
      *moves++ = uo_move_encode(square_from, square_enemy_checker, uo_move_type__x);
    }

    uo_bitboard attack_checker_N = uo_andn(pins_to_own_K, own_N & uo_bitboard_attacks_N(square_enemy_checker));
    while (attack_checker_N)
    {
      square_from = uo_bitboard_next_square(&attack_checker_N);
      *moves++ = uo_move_encode(square_from, square_enemy_checker, uo_move_type__x);
    }

    uo_bitboard attack_checker_P = uo_andn(pins_to_own_K, own_P & uo_bitboard_attacks_P(square_enemy_checker, !color_to_move));
    while (attack_checker_P)
    {
      square_from = uo_bitboard_next_square(&attack_checker_P);
      uo_bitboard bitboard_from = uo_square_bitboard(square_from);

      if (own_P & bitboard_from & bitboard_seventh_rank)
      {
        *moves++ = uo_move_encode(square_from, square_enemy_checker, uo_move_type__promo_Qx);
        *moves++ = uo_move_encode(square_from, square_enemy_checker, uo_move_type__promo_Nx);
        *moves++ = uo_move_encode(square_from, square_enemy_checker, uo_move_type__promo_Bx);
        *moves++ = uo_move_encode(square_from, square_enemy_checker, uo_move_type__promo_Rx);
      }
      else
      {
        *moves++ = uo_move_encode(square_from, square_enemy_checker, uo_move_type__x);
      }
    }

    // Let's still check for en passant captures
    if (enemy_checks_P && enpassant_file)
    {
      if (enpassant_file > 1 && (own_P & (enemy_checks_P >> 1)))
      {
        *moves++ = uo_move_encode(square_enemy_checker - 1, square_enemy_checker + direction_forwards, uo_move_type__enpassant);
      }

      if (enpassant_file < 8 && (own_P & (enemy_checks_P << 1)))
      {
        *moves++ = uo_move_encode(square_enemy_checker + 1, square_enemy_checker + direction_forwards, uo_move_type__enpassant);
      }
    }

    // All possible captures considered. Next let's consider blocking moves

    uint8_t file_enemy_checker = uo_square_file(square_enemy_checker);
    uint8_t rank_enemy_checker = uo_square_rank(square_enemy_checker);

    uo_square squares_between[6];
    size_t i = uo_squares_between(square_own_K, square_enemy_checker, squares_between);

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
        *moves++ = uo_move_encode(square_from, square_between, uo_move_type__quiet);
      }

      uo_bitboard block_R = uo_andn(pins_to_own_K, own_R & block_lines);
      while (block_R)
      {
        square_from = uo_bitboard_next_square(&block_R);
        *moves++ = uo_move_encode(square_from, square_between, uo_move_type__quiet);
      }

      uo_bitboard block_Q = uo_andn(pins_to_own_K, own_Q & (block_diagonals | block_lines));
      while (block_Q)
      {
        square_from = uo_bitboard_next_square(&block_Q);
        *moves++ = uo_move_encode(square_from, square_between, uo_move_type__quiet);
      }

      uo_bitboard block_N = uo_andn(pins_to_own_K, own_N & uo_bitboard_moves_N(square_between, mask_enemy, mask_own));
      while (block_N)
      {
        square_from = uo_bitboard_next_square(&block_N);
        *moves++ = uo_move_encode(square_from, square_between, uo_move_type__quiet);
      }

      uo_bitboard block_P = own_P & uo_square_bitboard(square_between + direction_backwards);

      if ((bitboard_square_between & bitboard_fourth_rank) && (uo_bitboard_double_push_P(uo_square_bitboard(square_between + direction_backwards * 2), color_to_move, empty)))
      {
        block_P |= own_P & uo_square_bitboard(square_between + (direction_backwards * 2));
      }

      block_P = uo_andn(pins_to_own_K, block_P);
      while (block_P)
      {
        square_from = uo_bitboard_next_square(&block_P);
        uo_bitboard bitboard_from = uo_square_bitboard(square_from);

        if (bitboard_from & bitboard_seventh_rank)
        {
          *moves++ = uo_move_encode(square_from, square_between, uo_move_type__promo_Q);
          *moves++ = uo_move_encode(square_from, square_between, uo_move_type__promo_N);
          *moves++ = uo_move_encode(square_from, square_between, uo_move_type__promo_B);
          *moves++ = uo_move_encode(square_from, square_between, uo_move_type__promo_R);
        }
        else if ((bitboard_from & bitboard_second_rank) && (bitboard_square_between & bitboard_fourth_rank))
        {
          *moves++ = uo_move_encode(square_from, square_between, uo_move_type__P_double_push);
        }
        else
        {
          *moves++ = uo_move_encode(square_from, square_between, uo_move_type__quiet);
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
      uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(square_to, color_to_move);
      uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(square_to);
      uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_BQ | enemy_checks_RQ | enemy_checks_P | enemy_checks_K;

      if (enemy_checks)
        continue;

      uo_piece piece_captured = board[square_to];
      uo_move_type move_type = piece_captured ? uo_move_type__x : uo_move_type__quiet;

      *moves++ = uo_move_encode(square_own_K, square_to, move_type);
    }

    return moves - movelist;
  }

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
      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__x);
    }

    uo_bitboard quiets_N = moves_N & empty;
    while (quiets_N)
    {
      square_to = uo_bitboard_next_square(&quiets_N);
      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__quiet);
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
      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__x);
    }

    uo_bitboard quiets_B = moves_B & empty;
    while (quiets_B)
    {
      square_to = uo_bitboard_next_square(&quiets_B);
      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__quiet);
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
      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__x);
    }

    uo_bitboard quiets_R = moves_R & empty;
    while (quiets_R)
    {
      square_to = uo_bitboard_next_square(&quiets_R);
      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__quiet);
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
      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__x);
    }

    uo_bitboard quiets_Q = moves_Q & empty;
    while (quiets_Q)
    {
      square_to = uo_bitboard_next_square(&quiets_Q);
      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__quiet);
    }
  }

  // Moves for non pinned pawns
  uo_bitboard non_pinned_P = uo_andn(pins_to_own_K, own_P);

  // Single pawn push
  uo_bitboard non_pinned_single_push_P = uo_bitboard_single_push_P(non_pinned_P, color_to_move, empty);

  while (non_pinned_single_push_P)
  {
    square_to = uo_bitboard_next_square(&non_pinned_single_push_P);
    square_from = square_to + direction_backwards;

    if (uo_square_bitboard(square_to) & bitboard_last_rank)
    {
      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Q);
      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_N);
      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_B);
      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_R);
    }
    else
    {
      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__quiet);
    }
  }

  // Double pawn push
  uo_bitboard non_pinned_double_push_P = uo_bitboard_double_push_P(non_pinned_P, color_to_move, empty);

  while (non_pinned_double_push_P)
  {
    square_to = uo_bitboard_next_square(&non_pinned_double_push_P);
    square_from = square_to + direction_backwards * 2;
    *moves++ = uo_move_encode(square_from, square_to, uo_move_type__P_double_push);
  }

  // Captures to right
  uo_bitboard non_pinned_captures_right_P = uo_bitboard_captures_right_P(non_pinned_P, color_to_move, mask_enemy);

  while (non_pinned_captures_right_P)
  {
    square_to = uo_bitboard_next_square(&non_pinned_captures_right_P);
    square_from = square_to + direction_backwards - 1;

    if (uo_square_bitboard(square_to) & bitboard_last_rank)
    {
      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Qx);
      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Nx);
      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Bx);
      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Rx);
    }
    else
    {
      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__x);
    }
  }

  // Captures to left
  uo_bitboard non_pinned_captures_left_P = uo_bitboard_captures_left_P(non_pinned_P, color_to_move, mask_enemy);

  while (non_pinned_captures_left_P)
  {
    square_to = uo_bitboard_next_square(&non_pinned_captures_left_P);
    square_from = square_to + direction_backwards + 1;

    if (uo_square_bitboard(square_to) & bitboard_last_rank)
    {
      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Qx);
      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Nx);
      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Bx);
      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Rx);
    }
    else
    {
      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__x);
    }
  }

  // Non pinned pawn en passant captures
  if (enpassant_file)
  {
    uo_bitboard enpassant = bitboard_enpassant_file & bitboard_fifth_rank;
    square_to = uo_tzcnt(enpassant) + direction_forwards;

    if (enpassant_file > 1 && (non_pinned_P & (enpassant >> 1)))
    {
      *moves++ = uo_move_encode(square_to - 1 + direction_backwards, square_to, uo_move_type__enpassant);
    }

    if (enpassant_file < 8 && (non_pinned_P & (enpassant << 1)))
    {
      *moves++ = uo_move_encode(square_to + 1 + direction_backwards, square_to, uo_move_type__enpassant);
    }
  }

  // Next, moves for pinned pieces

  if (pins_to_own_K)
  {

    // Moves for pinned bishops
    uo_bitboard pinned_B = own_B & pins_to_own_K;
    while (pinned_B)
    {
      square_from = uo_bitboard_next_square(&pinned_B);
      uo_bitboard moves_B = uo_bitboard_moves_B(square_from, mask_own, mask_enemy);
      uo_bitboard bitboard_from = uo_square_bitboard(square_from);
      uo_bitboard occupied_after_move = uo_andn(bitboard_from, occupied);

      while (moves_B)
      {
        square_to = uo_bitboard_next_square(&moves_B);
        uo_bitboard bitboard_to = uo_square_bitboard(square_to);
        uo_bitboard enemy_checks_BQ = enemy_BQ & uo_andn(bitboard_to, uo_bitboard_attacks_B(square_own_K, occupied_after_move | bitboard_to));
        uo_bitboard enemy_checks_RQ = enemy_RQ & uo_andn(bitboard_to, uo_bitboard_attacks_R(square_own_K, occupied_after_move | bitboard_to));
        uo_bitboard enemy_checks = enemy_checks_BQ | enemy_checks_RQ;

        if (enemy_checks)
          continue;

        uo_piece piece_captured = board[square_to];
        uo_move_type move_type = piece_captured ? uo_move_type__x : uo_move_type__quiet;
        *moves++ = uo_move_encode(square_from, square_to, move_type);
      }
    }

    // Moves for pinned rooks
    uo_bitboard pinned_R = own_R & pins_to_own_K;
    while (pinned_R)
    {
      square_from = uo_bitboard_next_square(&pinned_R);
      uo_bitboard moves_R = uo_bitboard_moves_R(square_from, mask_own, mask_enemy);
      uo_bitboard bitboard_from = uo_square_bitboard(square_from);
      uo_bitboard occupied_after_move = uo_andn(bitboard_from, occupied);

      while (moves_R)
      {
        square_to = uo_bitboard_next_square(&moves_R);
        uo_bitboard bitboard_to = uo_square_bitboard(square_to);
        uo_bitboard enemy_checks_BQ = enemy_BQ & uo_andn(bitboard_to, uo_bitboard_attacks_B(square_own_K, occupied_after_move | bitboard_to));
        uo_bitboard enemy_checks_RQ = enemy_RQ & uo_andn(bitboard_to, uo_bitboard_attacks_R(square_own_K, occupied_after_move | bitboard_to));
        uo_bitboard enemy_checks = enemy_checks_BQ | enemy_checks_RQ;

        if (enemy_checks)
          continue;

        uo_piece piece_captured = board[square_to];
        uo_move_type move_type = piece_captured ? uo_move_type__x : uo_move_type__quiet;
        *moves++ = uo_move_encode(square_from, square_to, move_type);
      }
    }

    // Moves for pinned queens
    uo_bitboard pinned_Q = own_Q & pins_to_own_K;
    while (pinned_Q)
    {
      square_from = uo_bitboard_next_square(&pinned_Q);
      uo_bitboard moves_Q = uo_bitboard_moves_Q(square_from, mask_own, mask_enemy);
      uo_bitboard bitboard_from = uo_square_bitboard(square_from);
      uo_bitboard occupied_after_move = uo_andn(bitboard_from, occupied);

      while (moves_Q)
      {
        square_to = uo_bitboard_next_square(&moves_Q);
        uo_bitboard bitboard_to = uo_square_bitboard(square_to);
        uo_bitboard enemy_checks_BQ = enemy_BQ & uo_andn(bitboard_to, uo_bitboard_attacks_B(square_own_K, occupied_after_move | bitboard_to));
        uo_bitboard enemy_checks_RQ = enemy_RQ & uo_andn(bitboard_to, uo_bitboard_attacks_R(square_own_K, occupied_after_move | bitboard_to));
        uo_bitboard enemy_checks = enemy_checks_BQ | enemy_checks_RQ;

        if (enemy_checks)
          continue;

        uo_piece piece_captured = board[square_to];
        uo_move_type move_type = piece_captured ? uo_move_type__x : uo_move_type__quiet;
        *moves++ = uo_move_encode(square_from, square_to, move_type);
      }
    }

    // Moves for pinned pawns
    uo_bitboard pinned_P = own_P & pins_to_own_K;

    // Single pawn push
    uo_bitboard pinned_single_push_P = uo_bitboard_single_push_P(pinned_P, color_to_move, empty);
    while (pinned_single_push_P)
    {
      square_to = uo_bitboard_next_square(&pinned_single_push_P);
      square_from = square_to + direction_backwards;
      uo_bitboard bitboard_from = uo_square_bitboard(square_from);
      uo_bitboard bitboard_to = uo_square_bitboard(square_to);
      uo_bitboard occupied_after_move = uo_andn(bitboard_from, occupied | bitboard_to);

      uo_bitboard enemy_checks_BQ = enemy_BQ & uo_andn(bitboard_to, uo_bitboard_attacks_B(square_own_K, occupied_after_move));
      uo_bitboard enemy_checks_RQ = enemy_RQ & uo_andn(bitboard_to, uo_bitboard_attacks_R(square_own_K, occupied_after_move));
      uo_bitboard enemy_checks = enemy_checks_BQ | enemy_checks_RQ;

      if (enemy_checks)
        continue;

      if (bitboard_to & bitboard_last_rank)
      {
        *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Q);
        *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_N);
        *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_B);
        *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_R);
      }
      else
      {
        *moves++ = uo_move_encode(square_from, square_to, uo_move_type__quiet);
      }
    }

    // Double pawn push
    uo_bitboard pinned_double_push_P = uo_bitboard_double_push_P(pinned_P, color_to_move, empty);
    while (pinned_double_push_P)
    {
      square_to = uo_bitboard_next_square(&pinned_double_push_P);
      square_from = square_to + direction_backwards * 2;
      uo_bitboard bitboard_from = uo_square_bitboard(square_from);
      uo_bitboard bitboard_to = uo_square_bitboard(square_to);
      uo_bitboard occupied_after_move = uo_andn(bitboard_from, occupied | bitboard_to);

      uo_bitboard enemy_checks_BQ = enemy_BQ & uo_andn(bitboard_to, uo_bitboard_attacks_B(square_own_K, occupied_after_move));
      uo_bitboard enemy_checks_RQ = enemy_RQ & uo_andn(bitboard_to, uo_bitboard_attacks_R(square_own_K, occupied_after_move));
      uo_bitboard enemy_checks = enemy_checks_BQ | enemy_checks_RQ;

      if (enemy_checks)
        continue;

      *moves++ = uo_move_encode(square_from, square_to, uo_move_type__P_double_push);
    }

    // Captures to right
    uo_bitboard pinned_captures_right_P = uo_bitboard_captures_right_P(pinned_P, color_to_move, mask_enemy);
    while (pinned_captures_right_P)
    {
      square_to = uo_bitboard_next_square(&pinned_captures_right_P);
      square_from = square_to + direction_backwards - 1;
      uo_bitboard bitboard_from = uo_square_bitboard(square_from);
      uo_bitboard bitboard_to = uo_square_bitboard(square_to);
      uo_bitboard occupied_after_move = uo_andn(bitboard_from, occupied | bitboard_to);

      uo_bitboard enemy_checks_BQ = enemy_BQ & uo_andn(bitboard_to, uo_bitboard_attacks_B(square_own_K, occupied_after_move));
      uo_bitboard enemy_checks_RQ = enemy_RQ & uo_andn(bitboard_to, uo_bitboard_attacks_R(square_own_K, occupied_after_move));
      uo_bitboard enemy_checks = enemy_checks_BQ | enemy_checks_RQ;

      if (enemy_checks)
        continue;

      if (bitboard_to & bitboard_last_rank)
      {
        *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Qx);
        *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Nx);
        *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Bx);
        *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Rx);
      }
      else
      {
        *moves++ = uo_move_encode(square_from, square_to, uo_move_type__x);
      }
    }

    // Captures to left
    uo_bitboard pinned_captures_left_P = uo_bitboard_captures_left_P(pinned_P, color_to_move, mask_enemy);
    while (pinned_captures_left_P)
    {
      square_to = uo_bitboard_next_square(&pinned_captures_left_P);
      square_from = square_to + direction_backwards + 1;
      uo_bitboard bitboard_from = uo_square_bitboard(square_from);
      uo_bitboard bitboard_to = uo_square_bitboard(square_to);
      uo_bitboard occupied_after_move = uo_andn(bitboard_from, occupied | bitboard_to);

      uo_bitboard enemy_checks_BQ = enemy_BQ & uo_andn(bitboard_to, uo_bitboard_attacks_B(square_own_K, occupied_after_move));
      uo_bitboard enemy_checks_RQ = enemy_RQ & uo_andn(bitboard_to, uo_bitboard_attacks_R(square_own_K, occupied_after_move));
      uo_bitboard enemy_checks = enemy_checks_BQ | enemy_checks_RQ;

      if (enemy_checks)
        continue;

      if (bitboard_to & bitboard_last_rank)
      {
        *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Qx);
        *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Nx);
        *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Bx);
        *moves++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Rx);
      }
      else
      {
        *moves++ = uo_move_encode(square_from, square_to, uo_move_type__x);
      }
    }

    // Pinned pawn en passant captures
    if (enpassant_file)
    {
      uo_bitboard bitboard_from = uo_square_bitboard(square_from);
      uo_bitboard occupied_after_move = uo_andn(bitboard_from, occupied);
      uo_bitboard enpassant = bitboard_enpassant_file & bitboard_fifth_rank;
      square_to = uo_tzcnt(enpassant) + direction_forwards;

      if (enpassant_file > 1 && (pinned_P & (enpassant >> 1)))
      {
        uo_bitboard enemy_checks_BQ = enemy_BQ & uo_bitboard_attacks_B(square_to, occupied_after_move);
        uo_bitboard enemy_checks_RQ = enemy_RQ & uo_bitboard_attacks_R(square_to, occupied_after_move);
        uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(square_to);
        uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(square_to, color_to_move);
        uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_BQ | enemy_checks_RQ | enemy_checks_P;

        if (!enemy_checks)
        {
          *moves++ = uo_move_encode(square_to - 1 + direction_backwards, square_to, uo_move_type__enpassant);
        }
      }

      if (enpassant_file < 8 && (pinned_P & (enpassant << 1)))
      {
        uo_bitboard enemy_checks_BQ = enemy_BQ & uo_bitboard_attacks_B(square_to, occupied_after_move);
        uo_bitboard enemy_checks_RQ = enemy_RQ & uo_bitboard_attacks_R(square_to, occupied_after_move);
        uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(square_to);
        uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(square_to, color_to_move);
        uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_BQ | enemy_checks_RQ | enemy_checks_P;

        if (!enemy_checks)
        {
          *moves++ = uo_move_encode(square_to + 1 + direction_backwards, square_to, uo_move_type__enpassant);
        }
      }
    }
  }

  // Finally, let's consider king moves

  // Castling moves

  if (own.can_castle_OO(flags) && !(occupied & (uo_square_bitboard(own.first_rank_squares.f) | uo_square_bitboard(own.first_rank_squares.g))))
  {
    uo_bitboard enemy_checks_BQ = enemy_BQ & uo_bitboard_attacks_B(own.first_rank_squares.f, occupied);
    uo_bitboard enemy_checks_RQ = enemy_RQ & uo_bitboard_attacks_R(own.first_rank_squares.f, occupied);
    uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(own.first_rank_squares.f);
    uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(own.first_rank_squares.f, color_to_move);
    uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(own.first_rank_squares.f);
    uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_BQ | enemy_checks_RQ | enemy_checks_P | enemy_checks_K;

    if (!enemy_checks)
    {
      uo_bitboard enemy_checks_BQ = enemy_BQ & uo_bitboard_attacks_B(own.first_rank_squares.g, occupied);
      uo_bitboard enemy_checks_RQ = enemy_RQ & uo_bitboard_attacks_R(own.first_rank_squares.g, occupied);
      uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(own.first_rank_squares.g);
      uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(own.first_rank_squares.g, color_to_move);
      uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(own.first_rank_squares.g);
      uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_BQ | enemy_checks_RQ | enemy_checks_P | enemy_checks_K;

      if (!enemy_checks)
      {
        *moves++ = uo_move_encode(square_own_K, own.first_rank_squares.g, uo_move_type__OO);
      }
    }
  }

  if (own.can_castle_OOO(flags) && !(occupied & (uo_square_bitboard(own.first_rank_squares.d) | uo_square_bitboard(own.first_rank_squares.c) | uo_square_bitboard(own.first_rank_squares.b))))
  {
    uo_bitboard enemy_checks_BQ = enemy_BQ & uo_bitboard_attacks_B(own.first_rank_squares.d, occupied);
    uo_bitboard enemy_checks_RQ = enemy_RQ & uo_bitboard_attacks_R(own.first_rank_squares.d, occupied);
    uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(own.first_rank_squares.d);
    uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(own.first_rank_squares.d, color_to_move);
    uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(own.first_rank_squares.d);
    uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_BQ | enemy_checks_RQ | enemy_checks_P | enemy_checks_K;

    if (!enemy_checks)
    {
      uo_bitboard enemy_checks_BQ = enemy_BQ & uo_bitboard_attacks_B(own.first_rank_squares.c, occupied);
      uo_bitboard enemy_checks_RQ = enemy_RQ & uo_bitboard_attacks_R(own.first_rank_squares.c, occupied);
      uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(own.first_rank_squares.c);
      uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(own.first_rank_squares.c, color_to_move);
      uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(own.first_rank_squares.c);
      uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_BQ | enemy_checks_RQ | enemy_checks_P | enemy_checks_K;

      if (!enemy_checks)
      {
        *moves++ = uo_move_encode(square_own_K, own.first_rank_squares.c, uo_move_type__OOO);
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
    uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(square_to, color_to_move);
    uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(square_to);
    uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_BQ | enemy_checks_RQ | enemy_checks_P | enemy_checks_K;

    if (enemy_checks)
      continue;

    uo_piece piece_captured = board[square_to];
    uo_move_type move_type = piece_captured ? uo_move_type__x : uo_move_type__quiet;
    *moves++ = uo_move_encode(square_own_K, square_to, move_type);
  }

  return moves - movelist;
}

bool uo_position_is_check(uo_position *position)
{
  uint8_t color_to_move = uo_position_flags_color_to_move(position->flags);

  uo_bitboard occupied = position->white | position->black;
  uo_pieces pieces_enemy = uo_consts_by_color[!color_to_move].pieces;

  uo_bitboard own_K = *uo_position_piece_bitboard(position, uo_piece__K | color_to_move);
  uo_bitboard enemy_P = *uo_position_piece_bitboard(position, pieces_enemy.P);
  uo_bitboard enemy_N = *uo_position_piece_bitboard(position, pieces_enemy.N);
  uo_bitboard enemy_B = *uo_position_piece_bitboard(position, pieces_enemy.B);
  uo_bitboard enemy_R = *uo_position_piece_bitboard(position, pieces_enemy.R);
  uo_bitboard enemy_Q = *uo_position_piece_bitboard(position, pieces_enemy.Q);
  uo_bitboard enemy_K = *uo_position_piece_bitboard(position, pieces_enemy.K);
  uo_bitboard enemy_BQ = enemy_B | enemy_Q;
  uo_bitboard enemy_RQ = enemy_R | enemy_Q;

  uo_square square_own_K = uo_tzcnt(own_K);

  uo_bitboard enemy_checks_BQ = enemy_BQ & uo_bitboard_attacks_B(square_own_K, occupied);
  uo_bitboard enemy_checks_RQ = enemy_RQ & uo_bitboard_attacks_R(square_own_K, occupied);
  uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(square_own_K);
  uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(square_own_K, color_to_move);
  uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_BQ | enemy_checks_RQ | enemy_checks_P;

  if (!enemy_checks)
  {
    position->checkers[0] = 0;
    position->checkers[1] = 0;
    return false;
  }

  position->checkers[0] = uo_bitboard_next_square(&enemy_checks);

  if (!enemy_checks)
  {
    position->checkers[1] = 0;
    return true;
  }

  position->checkers[1] = uo_bitboard_next_square(&enemy_checks);
  return true;
}

// Only direct checks, does not take into account discoveries
bool uo_position_is_check_move(uo_position *position, uo_move move)
{
  uo_square square_from = uo_move_square_from(move);
  uo_piece piece = position->board[square_from];
  uo_square square_to = uo_move_square_to(move);
  uint8_t color = uo_color(piece);

  uo_bitboard mask_enemy_K = *uo_position_piece_bitboard(position, uo_piece__K | !color);

  switch (uo_piece_type(piece))
  {
    case uo_piece__P:
      return mask_enemy_K & uo_bitboard_attacks_P(square_to, color);

    case uo_piece__N:
      return mask_enemy_K & uo_bitboard_attacks_N(square_to);
  }

  uo_bitboard occupied = position->white | position->black;

  switch (uo_piece_type(piece))
  {
    case uo_piece__B:
      return mask_enemy_K & uo_bitboard_attacks_B(square_to, occupied);

    case uo_piece__R:
      return mask_enemy_K & uo_bitboard_attacks_R(square_to, occupied);

    case uo_piece__Q:
      return mask_enemy_K & (
        uo_bitboard_attacks_B(square_to, occupied) |
        uo_bitboard_attacks_R(square_to, occupied));
  }

  return false;
}

uo_move uo_position_parse_move(uo_position *position, char str[5])
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

  uo_bitboard bitboard_from = uo_square_bitboard(square_from);
  uo_bitboard bitboard_to = uo_square_bitboard(square_to);
  uo_move_type move_type = uo_move_type__quiet;
  uo_piece piece = position->board[square_from];
  uo_piece piece_captured = position->board[square_to];
  uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);

  if (piece_captured)
  {
    move_type |= uo_move_type__x;
  }
  else if (uo_piece_type(piece) == uo_piece__K)
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
  else if (uo_piece_type(piece) == uo_piece__P)
  {
    if (uo_square_file(square_from) != uo_square_file(square_to))
    {
      move_type = uo_move_type__enpassant;
      piece_captured = uo_piece__P | !uo_color(piece);
    }
    else
    {
      if (piece & uo_white)
      {
        if (square_to - square_from == 16)
        {
          move_type = uo_move_type__P_double_push;
        }
      }
      else // black
      {
        if (square_from - square_to == 16)
        {
          move_type = uo_move_type__P_double_push;
        }
      }
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
