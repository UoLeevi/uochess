#include "uo_position.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

// see: https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation
// example fen: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
uo_position *uo_position_from_fen(uo_position *pos, char *fen)
{
  char piece_placement[72];
  char active_color;
  char castling[5];
  char enpassant[3];
  int halfmoves;
  int fullmove;

  int count = sscanf(fen, "%71s %c %4s %2s %d %d",
    piece_placement, &active_color,
    castling, enpassant,
    &halfmoves, &fullmove);

  if (count != 6)
  {
    return NULL;
  }

  // clear position
  memset(pos, 0, sizeof * pos);

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

      uo_piece piece = uo_piece_from_char[c];

      if (!piece)
      {
        return NULL;
      }

      uo_bitboard *bitboard = uo_position__piece_bitboard(pos, piece);
      uo_square square = (i << 3) + j;
      uo_bitboard mask = uo_square_bitboard(square);

      *bitboard |= mask;

      if (piece & uo_piece__black)
      {
        pos->piece_color |= mask;
      }
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
    pos->color_to_move = uo_piece__black;
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
      pos->castling |= 1;
      c = *ptr++;
    }

    if (c == 'Q')
    {
      pos->castling |= 2;
      c = *ptr++;
    }

    if (c == 'k')
    {
      pos->castling |= 4;
      c = *ptr++;
    }

    if (c == 'q')
    {
      pos->castling |= 8;
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

    pos->enpassant = (rank << 3) + file;
  }

  // 5. Halfmove clock
  // 6. Fullmove number
  pos->halfmoves = halfmoves;
  pos->fullmove = fullmove;

  return pos;
}

char *uo_position_to_diagram(uo_position *pos, char diagram[72])
{
  for (int i = 0; i < 64; ++i)
  {
    uo_piece piece = uo_position_square_piece(pos, i);
    int rank = uo_square_rank(i);
    int file = uo_square_file(i);
    int row = 7 - rank;
    int index = (row << 3) + file + row;
    char c = uo_piece_to_char[piece];
    diagram[index] = c ? c : '.';
  }

  for (int i = 8; i < 72; i += 9)
  {
    diagram[i] = '\n';
  }

  diagram[71] = '\0';

  return diagram;
}


uo_move_ex uo_position_make_move(uo_position *position, uo_move move)
{
  uo_square square_from = uo_move_square_from(move);
  uo_bitboard bitboard_from = uo_square_bitboard(square_from);
  uo_square square_to = uo_move_square_to(move);
  uo_bitboard bitboard_to = uo_square_bitboard(square_to);
  uo_move_type move_type = uo_move_get_type(move);
  uo_piece piece = uo_position_square_piece(position, square_from);
  uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);

  *bitboard &= ~bitboard_from;
  position->enpassant = 0;

  if (position->color_to_move == uo_piece__black)
  {
    position->piece_color |= bitboard_to;

    if (move_type == uo_move_type__P_double_push)
    {
      // en passant
      uo_bitboard adjecent_enemy_pawn = ((bitboard_to << 1) | (bitboard_to >> 1)) & uo_bitboard_rank[4] & ~position->piece_color & position->p;
      if (adjecent_enemy_pawn) {
        position->enpassant = square_to + 8;
      }
    }
    else if (move_type & uo_move_type__enpassant)
    {
      // en passant capture
      uo_bitboard enpassant = uo_square_bitboard(position->enpassant);
      *bitboard &= ~(enpassant << 8);
    }
    else if (piece & uo_piece__K)
    {
      position->castling &= 0x3;
    }
    else if (square_from == uo_square__a8)
    {
      position->castling &= 0x7;
    }
    else if (square_from == uo_square__h8)
    {
      position->castling &= 0xb;
    }
  }
  else
  {
    position->piece_color &= ~bitboard_to;

    if (move_type == uo_move_type__P_double_push)
    {
      // en passant - white
      uo_bitboard adjecent_enemy_pawn = ((bitboard_to << 1) | (bitboard_to >> 1)) & uo_bitboard_rank[3] & position->piece_color & position->p;
      if (adjecent_enemy_pawn) {
        position->enpassant = square_to - 8;
      }
    }
    else if (move_type & uo_move_type__enpassant)
    {
      // en passant capture
      uo_bitboard enpassant = uo_square_bitboard(position->enpassant);
      *bitboard &= ~(enpassant >> 8);
    }
    else if (piece & uo_piece__K)
    {
      position->castling &= 0xc;
    }
    else if (square_from == uo_square__a1)
    {
      position->castling &= 0xd;
    }
    else if (square_from == uo_square__h1)
    {
      position->castling &= 0xe;
    }
  }

  uo_piece piece_captured = 0;

  // capture
  if (move_type & uo_move_type__x)
  {
    position->halfmoves = 0;

    piece_captured = uo_position_square_piece(position, square_to);
    uo_bitboard *bitboard_captured = uo_position_piece_bitboard(position, piece_captured);
    *bitboard_captured &= ~bitboard_to;
  }
  // pawn move
  else if (piece & uo_piece__P)
  {
    position->halfmoves = 0;
  }

  // promotion
  if (move_type & uo_move_type__promo)
  {
    if (move_type & uo_move_type__promo_Q)
    {
      position->q |= bitboard_to;
    }
    else if (move_type & uo_move_type__promo_R)
    {
      position->r |= bitboard_to;
    }
    else if (move_type & uo_move_type__promo_B)
    {
      position->b |= bitboard_to;
    }
    else // uo_move_type__promo_N
    {
      position->n |= bitboard_to;
    }
  }
  else
  {
    *bitboard |= bitboard_to;
  }

  position->color_to_move = !position->color_to_move;
  ++position->fullmove;

  return uo_move_ex_encode(move, piece_captured);
}



void uo_position_unmake_move(uo_position *position, uo_move_ex move)
{
  uo_square square_from = uo_move_square_from(move);
  uo_bitboard bitboard_from = uo_square_bitboard(square_from);
  uo_square square_to = uo_move_square_to(move);
  uo_bitboard bitboard_to = uo_square_bitboard(square_to);
  uo_move_type move_type = uo_move_get_type(move);
  uo_piece piece = uo_position_square_piece(position, square_to);
  uo_piece piece_captured = uo_move_ex_piece_captured(move);

  // TODO
}
