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
  memset(pos, 0, sizeof *pos);

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

      pos->board[square] = piece;
      *bitboard |= mask;

      if (piece >> 7)
      {
        pos->black_piece |= mask;
      }
    }

    c = *ptr++;

    if (i != 0 && c != '/')
    {
      return NULL;
    }
  }

  // 2. Active color

  if (active_color == 'w')
  {
    pos->white_to_move = true;
  }
  else if (active_color != 'b')
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

    pos->enpassant = (uo_bitboard)1 << (rank * 8 + file);
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
    uo_piece piece = pos->board[i];
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