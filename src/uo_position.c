#include "uo_position.h"
#include "uo_util.h"
#include "uo_move.h"
#include "uo_piece.h"
#include "uo_square.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

// see: https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation
// example fen: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
uo_position *uo_position_from_fen(uo_position *position, char *fen)
{
  uo_position_flags flags = 0;
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

      uo_piece piece = uo_piece_from_char[c];

      if (!piece)
      {
        return NULL;
      }

      uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);
      uo_square square = (i << 3) + j;
      uo_bitboard mask = uo_square_bitboard(square);

      *bitboard |= mask;

      if (piece & uo_piece__black)
      {
        position->piece_color |= mask;
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
    flags = uo_position_flags_update_color_to_move(flags, uo_piece__black);
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
  flags = uo_position_flags_update_halfmoves(flags, halfmoves);
  position->fullmove = fullmove;

  position->flags = flags;

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
      uo_square sq = (rank << 3) + file;
      uo_piece piece = uo_position_square_piece(position, sq);

      if (piece)
      {
        if (empty_count)
        {
          ptr += sprintf(ptr, "%d", empty_count);
          empty_count = 0;
        }

        ptr += sprintf(ptr, "%c", uo_piece_to_char[piece]);
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
    if (castling_K) ptr += sprintf(ptr, "%c", 'K');

    uint8_t castling_Q = uo_position_flags_castling_Q(flags);
    if (castling_Q) ptr += sprintf(ptr, "%c", 'Q');

    uint8_t castling_k = uo_position_flags_castling_k(flags);
    if (castling_k) ptr += sprintf(ptr, "%c", 'k');

    uint8_t castling_q = uo_position_flags_castling_q(flags);
    if (castling_q) ptr += sprintf(ptr, "%c", 'q');

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

  uint8_t halfmoves = uo_position_flags_halfmoves(flags);
  uint16_t fullmoves = position->fullmove;

  ptr += sprintf(ptr, "%d %d", halfmoves, fullmoves);

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
      uo_square sq = (rank << 3) + file;
      uo_piece piece = uo_position_square_piece(position, sq);
      ptr += sprintf(ptr, " %c |", piece ? uo_piece_to_char[piece] : ' ');
    }
    ptr += sprintf(ptr, " %d\n", rank + 1);
  }

  ptr += sprintf(ptr, " +---+---+---+---+---+---+---+---+\n");
  ptr += sprintf(ptr, "   a   b   c   d   e   f   g   h\n");

  return ptr - diagram;
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
  uo_position_flags flags_prev = position->flags;
  uo_position_flags flags_new = flags_prev;
  uint8_t color_to_move = uo_position_flags_color_to_move(flags_prev);
  uint8_t halfmoves = uo_position_flags_halfmoves(flags_prev);
  uint8_t castling = uo_position_flags_castling(flags_prev);
  uint8_t enpassant_file = uo_position_flags_enpassant_file(flags_prev);

  uo_piece piece_captured = 0;

  *bitboard &= ~bitboard_from;
  flags_new = uo_position_flags_update_enpassant_file(flags_new, 0);

  if (color_to_move == uo_piece__black)
  {
    position->piece_color |= bitboard_to;

    // en passant capture
    if (move_type == uo_move_type__enpassant)
    {
      flags_new = uo_position_flags_update_halfmoves(flags_new, 0);
      uo_bitboard enpassant = uo_square_bitboard(23 + enpassant_file);
      *bitboard &= ~enpassant;
    }
    else if (move_type == uo_move_type__P_double_push)
    {
      flags_new = uo_position_flags_update_halfmoves(flags_new, 0);
      // en passant
      uo_bitboard adjecent_enemy_pawn = ((bitboard_to << 1) | (bitboard_to >> 1)) & uo_bitboard_rank[4] & ~position->piece_color & position->P;
      if (adjecent_enemy_pawn)
      {
        uo_square square_enemy_K = uo_ffs(position->K & ~position->piece_color) - 1;
        uint8_t rank_enemy_K = uo_square_rank(square_enemy_K);
        uo_bitboard bitboard_rank_enemy_K = uo_bitboard_rank[rank_enemy_K];

        if (rank_enemy_K == 4 && uo_popcount(adjecent_enemy_pawn) == 1)
        {
          uo_bitboard occupied = position->P | position->N | position->B | position->R | position->Q | position->K;
          occupied &= ~uo_bitboard_file[uo_square_file(square_to)];
          uo_bitboard rank_pins_to_enemy_K = uo_bitboard_pins(square_enemy_K, occupied, 0, (position->R | position->Q) & position->piece_color) & bitboard_rank_enemy_K;

          if (!(rank_pins_to_enemy_K & adjecent_enemy_pawn))
          {
            flags_new = uo_position_flags_update_enpassant_file(flags_new, uo_square_file(square_to) + 1);
          }
        }
        else
        {
          flags_new = uo_position_flags_update_enpassant_file(flags_new, uo_square_file(square_to) + 1);
        }
      }
    }
    // capture
    else if (move_type & uo_move_type__x)
    {
      flags_new = uo_position_flags_update_halfmoves(flags_new, 0);

      piece_captured = uo_position_square_piece(position, square_to);
      uo_bitboard *bitboard_captured = uo_position_piece_bitboard(position, piece_captured);
      *bitboard_captured &= ~bitboard_to;

      if (piece_captured & uo_piece__R)
      {
        switch (square_to)
        {
          case uo_square__a1:
            flags_new = uo_position_flags_update_castling_Q(flags_new, false);
            break;

          case uo_square__h1:
            flags_new = uo_position_flags_update_castling_K(flags_new, false);
            break;

          default:
            break;
        }
      }
    }
    // pawn move
    else if (piece & uo_piece__P)
    {
      flags_new = uo_position_flags_update_halfmoves(flags_new, 0);
    }

    if (piece & uo_piece__K)
    {
      flags_new = uo_position_flags_update_castling_black(flags_new, false);

      // castling
      if (move_type == uo_move_type__OO)
      {
        position->R &= ~uo_square_bitboard(uo_square__h8);
        position->R |= uo_square_bitboard(uo_square__f8);
        position->piece_color |= uo_square_bitboard(uo_square__f8);
      }
      else if (move_type == uo_move_type__OOO)
      {
        position->R &= ~uo_square_bitboard(uo_square__a8);
        position->R |= uo_square_bitboard(uo_square__d8);
        position->piece_color |= uo_square_bitboard(uo_square__d8);
      }
    }
    else if (square_from == uo_square__a8)
    {
      flags_new = uo_position_flags_update_castling_q(flags_new, false);
    }
    else if (square_from == uo_square__h8)
    {
      flags_new = uo_position_flags_update_castling_k(flags_new, false);
    }
  }
  else // white
  {
    position->piece_color &= ~bitboard_to;

    // en passant capture
    if (move_type == uo_move_type__enpassant)
    {
      flags_new = uo_position_flags_update_halfmoves(flags_new, 0);
      uo_bitboard enpassant = uo_square_bitboard(31 + enpassant_file);
      *bitboard &= ~enpassant;
    }
    else if (move_type == uo_move_type__P_double_push)
    {
      flags_new = uo_position_flags_update_halfmoves(flags_new, 0);
      // en passant
      uo_bitboard adjecent_enemy_pawn = ((bitboard_to << 1) | (bitboard_to >> 1)) & uo_bitboard_rank[3] & position->piece_color & position->P;
      if (adjecent_enemy_pawn)
      {
        uo_square square_enemy_K = uo_ffs(position->K & position->piece_color) - 1;
        uint8_t rank_enemy_K = uo_square_rank(square_enemy_K);
        uo_bitboard bitboard_rank_enemy_K = uo_bitboard_rank[rank_enemy_K];

        if (rank_enemy_K == 3 && uo_popcount(adjecent_enemy_pawn) == 1)
        {
          uo_bitboard occupied = position->P | position->N | position->B | position->R | position->Q | position->K;
          occupied &= ~uo_bitboard_file[uo_square_file(square_to)];
          uo_bitboard rank_pins_to_enemy_K = uo_bitboard_pins(square_enemy_K, occupied, 0, (position->R | position->Q) & ~position->piece_color) & bitboard_rank_enemy_K;

          if (!(rank_pins_to_enemy_K & adjecent_enemy_pawn))
          {
            flags_new = uo_position_flags_update_enpassant_file(flags_new, uo_square_file(square_to) + 1);
          }
        }
        else
        {
          flags_new = uo_position_flags_update_enpassant_file(flags_new, uo_square_file(square_to) + 1);
        }
      }
    }
    // capture
    else if (move_type & uo_move_type__x)
    {
      flags_new = uo_position_flags_update_halfmoves(flags_new, 0);

      piece_captured = uo_position_square_piece(position, square_to);
      uo_bitboard *bitboard_captured = uo_position_piece_bitboard(position, piece_captured);
      *bitboard_captured &= ~bitboard_to;

      if (piece_captured & uo_piece__R)
      {
        switch (square_to)
        {
          case uo_square__a8:
            flags_new = uo_position_flags_update_castling_q(flags_new, false);
            break;

          case uo_square__h8:
            flags_new = uo_position_flags_update_castling_k(flags_new, false);
            break;

          default:
            break;
        }
      }
    }
    // pawn move
    else if (piece & uo_piece__P)
    {
      flags_new = uo_position_flags_update_halfmoves(flags_new, 0);
    }

    if (piece & uo_piece__K)
    {
      flags_new = uo_position_flags_update_castling_white(flags_new, false);

      // castling
      if (move_type == uo_move_type__OO)
      {
        position->R &= ~uo_square_bitboard(uo_square__h1);
        position->R |= uo_square_bitboard(uo_square__f1);
        position->piece_color &= ~uo_square_bitboard(uo_square__f1);
      }
      else if (move_type == uo_move_type__OOO)
      {
        position->R &= ~uo_square_bitboard(uo_square__a1);
        position->R |= uo_square_bitboard(uo_square__d1);
        position->piece_color &= ~uo_square_bitboard(uo_square__d1);
      }
    }
    else if (square_from == uo_square__a1)
    {
      flags_new = uo_position_flags_update_castling_Q(flags_new, false);
    }
    else if (square_from == uo_square__h1)
    {
      flags_new = uo_position_flags_update_castling_K(flags_new, false);
    }
  }

  // promotion
  if (move_type & uo_move_type__promo)
  {
    switch (move_type & ~uo_move_type__x)
    {
      case uo_move_type__promo_Q:
        position->Q |= bitboard_to;
        break;

      case uo_move_type__promo_R:
        position->R |= bitboard_to;
        break;

      case uo_move_type__promo_B:
        position->B |= bitboard_to;
        break;

      case uo_move_type__promo_N:
        position->N |= bitboard_to;
        break;

      default: /* uo_move_type__promo_N */
        position->N |= bitboard_to;
        break;
    }
  }
  else
  {
    *bitboard |= bitboard_to;
  }

  flags_new = uo_position_flags_update_color_to_move(flags_new, !color_to_move);
  ++position->fullmove;

  position->flags = flags_new;

  return uo_move_ex_encode(move, flags_prev, piece_captured);
}

void uo_position_unmake_move(uo_position *position, uo_move_ex move)
{
  uo_square square_from = uo_move_square_from(move);
  uo_bitboard bitboard_from = uo_square_bitboard(square_from);
  uo_square square_to = uo_move_square_to(move);
  uo_bitboard bitboard_to = uo_square_bitboard(square_to);
  uo_move_type move_type = uo_move_get_type(move);
  uo_position_flags flags_prev = uo_move_ex_flags(move);
  uo_piece piece = uo_position_square_piece(position, square_to);

  --position->fullmove;

  // promotion
  if (move_type & uo_move_type__promo)
  {
    piece &= uo_piece__p;

    switch (move_type & ~uo_move_type__x)
    {
      case uo_move_type__promo_Q:
        position->Q &= ~bitboard_to;
        break;

      case uo_move_type__promo_R:
        position->R &= ~bitboard_to;
        break;

      case uo_move_type__promo_B:
        position->B &= ~bitboard_to;
        break;

      case uo_move_type__promo_N:
        position->N &= ~bitboard_to;
        break;

      default: /* uo_move_type__promo_N */
        position->N &= ~bitboard_to;
        break;
    }

    position->P |= bitboard_from;
  }
  else
  {
    uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);
    *bitboard &= ~bitboard_to;
    *bitboard |= bitboard_from;
  }

  if (uo_piece_color(piece) == uo_piece__black)
  {
    position->piece_color |= bitboard_from;

    if (piece & uo_piece__K)
    {
      // castling
      if (move_type == uo_move_type__OO)
      {
        position->R &= ~uo_square_bitboard(uo_square__f8);
        position->R |= uo_square_bitboard(uo_square__h8);
        position->piece_color |= uo_square_bitboard(uo_square__h8);
      }
      else if (move_type == uo_move_type__OOO)
      {
        position->R &= ~uo_square_bitboard(uo_square__d8);
        position->R |= uo_square_bitboard(uo_square__a8);
        position->piece_color |= uo_square_bitboard(uo_square__a8);
      }
    }

    // capture
    if (move_type & uo_move_type__x)
    {
      if (move_type == uo_move_type__enpassant)
      {
        // en passant capture
        uo_bitboard enpassant = bitboard_to << 8;
        position->P |= enpassant;
        position->piece_color &= ~enpassant;
      }
      else
      {
        uo_piece piece_captured = uo_move_ex_piece_captured(move);
        uo_bitboard *bitboard_captured = uo_position_piece_bitboard(position, piece_captured);
        *bitboard_captured |= bitboard_to;
        position->piece_color &= ~bitboard_to;
      }
    }
  }
  else // white
  {
    position->piece_color &= ~bitboard_from;

    if (piece & uo_piece__K)
    {
      // castling
      if (move_type == uo_move_type__OO)
      {
        position->R &= ~uo_square_bitboard(uo_square__f1);
        position->R |= uo_square_bitboard(uo_square__h1);
        position->piece_color &= ~uo_square_bitboard(uo_square__h1);
      }
      else if (move_type == uo_move_type__OOO)
      {
        position->R &= ~uo_square_bitboard(uo_square__d1);
        position->R |= uo_square_bitboard(uo_square__a1);
        position->piece_color &= ~uo_square_bitboard(uo_square__a1);
      }
    }

    // capture
    if (move_type & uo_move_type__x)
    {
      if (move_type == uo_move_type__enpassant)
      {
        // en passant capture
        uo_bitboard enpassant = bitboard_to >> 8;
        position->P |= enpassant;
        position->piece_color |= enpassant;
      }
      else
      {
        uo_piece piece_captured = uo_move_ex_piece_captured(move);
        uo_bitboard *bitboard_captured = uo_position_piece_bitboard(position, piece_captured);
        *bitboard_captured |= bitboard_to;
        position->piece_color |= bitboard_to;
      }
    }
  }

  position->flags = flags_prev;
}

size_t uo_position_get_moves(uo_position *position, uo_move *movelist)
{
  size_t count = 0;
  uo_square square_from;
  uo_square square_to;

  uo_position_flags flags = position->flags;
  uint8_t color_to_move = uo_position_flags_color_to_move(flags);
  uint8_t halfmoves = uo_position_flags_halfmoves(flags);
  uint8_t castling = uo_position_flags_castling(flags);
  uint8_t enpassant_file = uo_position_flags_enpassant_file(flags);
  uo_bitboard bitboard_enpassant_file = enpassant_file ? uo_bitboard_file[enpassant_file - 1] : 0;

  uo_bitboard bitboard_P = position->P;
  uo_bitboard bitboard_N = position->N;
  uo_bitboard bitboard_B = position->B;
  uo_bitboard bitboard_R = position->R;
  uo_bitboard bitboard_Q = position->Q;
  uo_bitboard bitboard_K = position->K;

  uo_bitboard occupied = bitboard_P | bitboard_N | bitboard_B | bitboard_R | bitboard_Q | bitboard_K;
  uo_bitboard mask_enemy = occupied & position->piece_color;
  uo_bitboard mask_own = occupied & ~mask_enemy;
  uo_bitboard temp;
  uo_piece piece_enemy_P = uo_piece__P;
  uo_piece piece_own_P = uo_piece__P;

  uo_bitboard bitboard_first_rank;
  uo_bitboard bitboard_second_rank;
  uo_bitboard bitboard_fourth_rank;
  uo_bitboard bitboard_fifth_rank;
  uo_bitboard bitboard_seventh_rank;
  uo_bitboard bitboard_last_rank;

  int8_t direction_forwards;
  int8_t direction_backwards;

  if (color_to_move)
  {
    temp = mask_own;
    mask_own = mask_enemy;
    mask_enemy = temp;
    piece_own_P |= uo_piece__black;
    bitboard_first_rank = uo_bitboard_rank[7];
    bitboard_second_rank = uo_bitboard_rank[6];
    bitboard_fourth_rank = uo_bitboard_rank[4];
    bitboard_fifth_rank = uo_bitboard_rank[3];
    bitboard_seventh_rank = uo_bitboard_rank[1];
    bitboard_last_rank = uo_bitboard_rank[0];

    direction_forwards = -8;
    direction_backwards = 8;
  }
  else
  {
    piece_enemy_P |= uo_piece__black;
    bitboard_first_rank = uo_bitboard_rank[0];
    bitboard_second_rank = uo_bitboard_rank[1];
    bitboard_fourth_rank = uo_bitboard_rank[3];
    bitboard_fifth_rank = uo_bitboard_rank[4];
    bitboard_seventh_rank = uo_bitboard_rank[6];
    bitboard_last_rank = uo_bitboard_rank[7];

    direction_forwards = 8;
    direction_backwards = -8;
  }

  uo_bitboard own_P = bitboard_P & mask_own;
  uo_bitboard own_N = bitboard_N & mask_own;
  uo_bitboard own_B = bitboard_B & mask_own;
  uo_bitboard own_R = bitboard_R & mask_own;
  uo_bitboard own_Q = bitboard_Q & mask_own;
  uo_bitboard own_K = bitboard_K & mask_own;
  uo_bitboard enemy_P = bitboard_P & mask_enemy;
  uo_bitboard enemy_N = bitboard_N & mask_enemy;
  uo_bitboard enemy_B = bitboard_B & mask_enemy;
  uo_bitboard enemy_R = bitboard_R & mask_enemy;
  uo_bitboard enemy_Q = bitboard_Q & mask_enemy;
  uo_bitboard enemy_K = bitboard_K & mask_enemy;

  // 1. Check for checks and pieces pinned to the king

  uo_square square_own_K = uo_ffs(own_K) - 1;
  uint8_t file_own_K = uo_square_file(square_own_K);
  uint8_t rank_own_K = uo_square_rank(square_own_K);
  uo_bitboard moves_K = uo_bitboard_moves_K(square_own_K, mask_own, mask_enemy);

  uo_bitboard enemy_checks_diagonals = uo_bitboard_attacks_B(square_own_K, occupied);
  uo_bitboard enemy_checks_lines = uo_bitboard_attacks_R(square_own_K, occupied);
  uo_bitboard enemy_checks_B = enemy_B & enemy_checks_diagonals;
  uo_bitboard enemy_checks_R = enemy_R & enemy_checks_lines;
  uo_bitboard enemy_checks_Q = enemy_Q & (enemy_checks_lines | enemy_checks_diagonals);
  uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(square_own_K);
  uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(square_own_K, color_to_move);
  uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_B | enemy_checks_R | enemy_checks_Q | enemy_checks_P;

  uo_bitboard pins_to_own_K = uo_bitboard_pins(square_own_K, occupied, enemy_B | enemy_Q, enemy_R | enemy_Q);

  if (enemy_checks)
  {
    uo_square square_enemy_checker = uo_ffs(enemy_checks) - 1;

    if (uo_popcount(enemy_checks) == 2)
    {
      // Double check, king must move

      square_to = -1;
      while (uo_bitboard_next_square(moves_K, &square_to))
      {
        uo_bitboard enemy_checks_diagonals = uo_bitboard_attacks_B(square_to, occupied & ~own_K);
        uo_bitboard enemy_checks_lines = uo_bitboard_attacks_R(square_to, occupied & ~own_K);
        uo_bitboard enemy_checks_B = enemy_B & enemy_checks_diagonals;
        uo_bitboard enemy_checks_R = enemy_R & enemy_checks_lines;
        uo_bitboard enemy_checks_Q = enemy_Q & (enemy_checks_lines | enemy_checks_diagonals);
        uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(square_to);
        uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(square_to, color_to_move);
        uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(square_to);
        uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_B | enemy_checks_R | enemy_checks_Q | enemy_checks_P | enemy_checks_K;

        if (enemy_checks) continue;

        uo_move_type move_type = (uo_square_bitboard(square_to) & mask_enemy) ? uo_move_type__x : uo_move_type__quiet;
        *movelist++ = uo_move_encode(square_own_K, square_to, move_type);
        ++count;
      }

      return count;
    }

    // Only one piece is giving a check

    // Let's first consider captures by each piece type

    uo_bitboard attack_checker_diagonals = uo_bitboard_attacks_B(square_enemy_checker, occupied);
    uo_bitboard attack_checker_lines = uo_bitboard_attacks_R(square_enemy_checker, occupied);
    uo_bitboard attack_checker_B = own_B & attack_checker_diagonals;
    uo_bitboard attack_checker_R = own_R & attack_checker_lines;
    uo_bitboard attack_checker_Q = own_Q & (attack_checker_diagonals | attack_checker_lines);
    uo_bitboard attack_checker_N = own_N & uo_bitboard_attacks_N(square_enemy_checker);
    uo_bitboard attack_checker_P = own_P & uo_bitboard_attacks_P(square_enemy_checker, !color_to_move);
    uo_bitboard attack_checker = attack_checker_N | attack_checker_B | attack_checker_R | attack_checker_Q | attack_checker_P;
    attack_checker &= ~pins_to_own_K;

    square_from = -1;
    while (uo_bitboard_next_square(attack_checker, &square_from))
    {
      uo_bitboard bitboard = uo_square_bitboard(square_from);

      if (own_P & bitboard & bitboard_seventh_rank)
      {
        *movelist++ = uo_move_encode(square_from, square_enemy_checker, uo_move_type__promo_Qx);
        ++count;
        *movelist++ = uo_move_encode(square_from, square_enemy_checker, uo_move_type__promo_Nx);
        ++count;
        *movelist++ = uo_move_encode(square_from, square_enemy_checker, uo_move_type__promo_Bx);
        ++count;
        *movelist++ = uo_move_encode(square_from, square_enemy_checker, uo_move_type__promo_Rx);
        ++count;
      }
      else
      {
        *movelist++ = uo_move_encode(square_from, square_enemy_checker, uo_move_type__x);
        ++count;
      }
    }

    // Let's still check for en passant captures
    if (enemy_checks_P && enpassant_file)
    {
      if (enpassant_file > 1 && (own_P & (enemy_checks_P >> 1)))
      {
        *movelist++ = uo_move_encode(square_enemy_checker - 1, square_enemy_checker + direction_forwards, uo_move_type__enpassant);
        ++count;
      }

      if (enpassant_file < 8 && (own_P & (enemy_checks_P << 1)))
      {
        *movelist++ = uo_move_encode(square_enemy_checker + 1, square_enemy_checker + direction_forwards, uo_move_type__enpassant);
        ++count;
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
      uo_bitboard block_B = own_B & block_diagonals;
      uo_bitboard block_R = own_R & block_lines;
      uo_bitboard block_Q = own_Q & (block_diagonals | block_lines);
      uo_bitboard block_N = own_N & uo_bitboard_moves_N(square_between, mask_enemy, mask_own);
      uo_bitboard block_P = own_P & uo_square_bitboard(square_between + direction_backwards);

      if ((bitboard_square_between & bitboard_fourth_rank)
        && (uo_bitboard_moves_P(square_between + (direction_backwards * 2), color_to_move, mask_own, mask_enemy) & bitboard_square_between))
      {
        block_P |= own_P & uo_square_bitboard(square_between + (direction_backwards * 2));
      }

      uo_bitboard block = block_N | block_B | block_R | block_Q | block_P;
      block &= ~pins_to_own_K;

      square_from = -1;
      while (uo_bitboard_next_square(block, &square_from))
      {
        uo_bitboard bitboard = uo_square_bitboard(square_from);

        if (own_P & bitboard)
        {
          if (bitboard & bitboard_seventh_rank)
          {
            *movelist++ = uo_move_encode(square_from, square_between, uo_move_type__promo_Q);
            ++count;
            *movelist++ = uo_move_encode(square_from, square_between, uo_move_type__promo_N);
            ++count;
            *movelist++ = uo_move_encode(square_from, square_between, uo_move_type__promo_B);
            ++count;
            *movelist++ = uo_move_encode(square_from, square_between, uo_move_type__promo_R);
            ++count;
          }
          else if ((bitboard & bitboard_second_rank) && (bitboard_square_between & bitboard_fourth_rank))
          {
            *movelist++ = uo_move_encode(square_from, square_between, uo_move_type__P_double_push);
            ++count;
          }
          else
          {
            *movelist++ = uo_move_encode(square_from, square_between, uo_move_type__quiet);
            ++count;
          }
        }
        else
        {
          *movelist++ = uo_move_encode(square_from, square_between, uo_move_type__quiet);
          ++count;
        }
      }
    }

    // Finally, let's consider king moves

    square_to = -1;
    while (uo_bitboard_next_square(moves_K, &square_to))
    {
      uo_bitboard enemy_checks_diagonals = uo_bitboard_attacks_B(square_to, occupied & ~own_K);
      uo_bitboard enemy_checks_lines = uo_bitboard_attacks_R(square_to, occupied & ~own_K);
      uo_bitboard enemy_checks_B = enemy_B & enemy_checks_diagonals;
      uo_bitboard enemy_checks_R = enemy_R & enemy_checks_lines;
      uo_bitboard enemy_checks_Q = enemy_Q & (enemy_checks_lines | enemy_checks_diagonals);
      uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(square_to);
      uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(square_to, color_to_move);
      uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(square_to);
      uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_B | enemy_checks_R | enemy_checks_Q | enemy_checks_P | enemy_checks_K;

      if (enemy_checks) continue;

      uo_move_type move_type = (uo_square_bitboard(square_to) & mask_enemy) ? uo_move_type__x : uo_move_type__quiet;
      *movelist++ = uo_move_encode(square_own_K, square_to, move_type);
      ++count;
    }

    return count;
  }

  // King is not in check. Let's list moves by piece type

  // First, non pinned pieces

  // Moves for non pinned knights
  square_from = -1;
  uo_bitboard non_pinned_N = own_N & ~pins_to_own_K;
  while (uo_bitboard_next_square(non_pinned_N, &square_from))
  {
    uo_bitboard moves_N = uo_bitboard_moves_N(square_from, mask_own, mask_enemy);

    square_to = -1;
    while (uo_bitboard_next_square(moves_N, &square_to))
    {
      uo_move_type move_type = (uo_square_bitboard(square_to) & mask_enemy) ? uo_move_type__x : uo_move_type__quiet;
      *movelist++ = uo_move_encode(square_from, square_to, move_type);
      ++count;
    }
  }

  // Moves for non pinned bishops
  square_from = -1;
  uo_bitboard non_pinned_B = own_B & ~pins_to_own_K;
  while (uo_bitboard_next_square(non_pinned_B, &square_from))
  {
    uo_bitboard moves_B = uo_bitboard_moves_B(square_from, mask_own, mask_enemy);

    square_to = -1;
    while (uo_bitboard_next_square(moves_B, &square_to))
    {
      uo_move_type move_type = (uo_square_bitboard(square_to) & mask_enemy) ? uo_move_type__x : uo_move_type__quiet;
      *movelist++ = uo_move_encode(square_from, square_to, move_type);
      ++count;
    }
  }

  // Moves for non pinned rooks
  square_from = -1;
  uo_bitboard non_pinned_R = own_R & ~pins_to_own_K;
  while (uo_bitboard_next_square(non_pinned_R, &square_from))
  {
    uo_bitboard moves_R = uo_bitboard_moves_R(square_from, mask_own, mask_enemy);

    square_to = -1;
    while (uo_bitboard_next_square(moves_R, &square_to))
    {
      uo_move_type move_type = (uo_square_bitboard(square_to) & mask_enemy) ? uo_move_type__x : uo_move_type__quiet;
      *movelist++ = uo_move_encode(square_from, square_to, move_type);
      ++count;
    }
  }

  // Moves for non pinned queens
  square_from = -1;
  uo_bitboard non_pinned_Q = own_Q & ~pins_to_own_K;
  while (uo_bitboard_next_square(non_pinned_Q, &square_from))
  {
    uo_bitboard moves_Q = uo_bitboard_moves_Q(square_from, mask_own, mask_enemy);

    square_to = -1;
    while (uo_bitboard_next_square(moves_Q, &square_to))
    {
      uo_move_type move_type = (uo_square_bitboard(square_to) & mask_enemy) ? uo_move_type__x : uo_move_type__quiet;
      *movelist++ = uo_move_encode(square_from, square_to, move_type);
      ++count;
    }
  }

  // Moves for non pinned pawns
  uo_bitboard non_pinned_P = own_P & ~pins_to_own_K;

  // Single pawn push
  uo_bitboard non_pinned_single_push_P = uo_bitboard_single_push_P(non_pinned_P, color_to_move, ~occupied);

  square_to = -1;
  while (uo_bitboard_next_square(non_pinned_single_push_P, &square_to))
  {
    square_from = square_to + direction_backwards;

    if (uo_square_bitboard(square_to) & bitboard_last_rank)
    {
      *movelist++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Q);
      ++count;
      *movelist++ = uo_move_encode(square_from, square_to, uo_move_type__promo_N);
      ++count;
      *movelist++ = uo_move_encode(square_from, square_to, uo_move_type__promo_B);
      ++count;
      *movelist++ = uo_move_encode(square_from, square_to, uo_move_type__promo_R);
      ++count;
    }
    else
    {
      *movelist++ = uo_move_encode(square_from, square_to, uo_move_type__quiet);
      ++count;
    }
  }

  // Double pawn push
  uo_bitboard non_pinned_double_push_P = uo_bitboard_double_push_P(non_pinned_P, color_to_move, ~occupied);

  square_to = -1;
  while (uo_bitboard_next_square(non_pinned_double_push_P, &square_to))
  {
    square_from = square_to + direction_backwards * 2;
    *movelist++ = uo_move_encode(square_from, square_to, uo_move_type__P_double_push);
    ++count;
  }

  // Captures to right
  uo_bitboard non_pinned_captures_right_P = uo_bitboard_captures_right_P(non_pinned_P, color_to_move, mask_enemy);

  square_to = -1;
  while (uo_bitboard_next_square(non_pinned_captures_right_P, &square_to))
  {
    square_from = square_to + direction_backwards - 1;

    if (uo_square_bitboard(square_to) & bitboard_last_rank)
    {
      *movelist++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Qx);
      ++count;
      *movelist++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Nx);
      ++count;
      *movelist++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Bx);
      ++count;
      *movelist++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Rx);
      ++count;
    }
    else
    {
      *movelist++ = uo_move_encode(square_from, square_to, uo_move_type__x);
      ++count;
    }
  }

  // Captures to left
  uo_bitboard non_pinned_captures_left_P = uo_bitboard_captures_left_P(non_pinned_P, color_to_move, mask_enemy);

  square_to = -1;
  while (uo_bitboard_next_square(non_pinned_captures_left_P, &square_to))
  {
    square_from = square_to + direction_backwards + 1;

    if (uo_square_bitboard(square_to) & bitboard_last_rank)
    {
      *movelist++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Qx);
      ++count;
      *movelist++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Nx);
      ++count;
      *movelist++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Bx);
      ++count;
      *movelist++ = uo_move_encode(square_from, square_to, uo_move_type__promo_Rx);
      ++count;
    }
    else
    {
      *movelist++ = uo_move_encode(square_from, square_to, uo_move_type__x);
      ++count;
    }
  }

  // Non pinned pawn en passant captures
  if (enpassant_file)
  {
    uo_bitboard enpassant = bitboard_enpassant_file & bitboard_fifth_rank;
    square_to = uo_ffs(enpassant) - 1 + direction_forwards;

    if (enpassant_file > 1 && (non_pinned_P & (enpassant >> 1)))
    {
      *movelist++ = uo_move_encode(square_to - 1 + direction_backwards, square_to, uo_move_type__enpassant);
      ++count;
    }

    if (enpassant_file < 8 && (non_pinned_P & (enpassant << 1)))
    {
      *movelist++ = uo_move_encode(square_to + 1 + direction_backwards, square_to, uo_move_type__enpassant);
      ++count;
    }
  }


  // Next, moves for pinned pieces

  // Moves for pinned bishops
  square_from = -1;
  uo_bitboard pinned_B = own_B & pins_to_own_K;
  while (uo_bitboard_next_square(pinned_B, &square_from))
  {
    uo_bitboard moves_B = uo_bitboard_moves_B(square_from, mask_own, mask_enemy);
    uo_bitboard bitboard_from = uo_square_bitboard(square_from);
    uo_bitboard occupied_after_move = occupied & ~bitboard_from;

    square_to = -1;
    while (uo_bitboard_next_square(moves_B, &square_to))
    {
      uo_bitboard bitboard_to = uo_square_bitboard(square_to);
      uo_bitboard enemy_checks_diagonals = uo_bitboard_attacks_B(square_own_K, occupied_after_move | bitboard_to) & ~bitboard_to;
      uo_bitboard enemy_checks_lines = uo_bitboard_attacks_R(square_own_K, occupied_after_move | bitboard_to) & ~bitboard_to;
      uo_bitboard enemy_checks_B = enemy_B & enemy_checks_diagonals;
      uo_bitboard enemy_checks_R = enemy_R & enemy_checks_lines;
      uo_bitboard enemy_checks_Q = enemy_Q & (enemy_checks_lines | enemy_checks_diagonals);
      uo_bitboard enemy_checks = enemy_checks_B | enemy_checks_R | enemy_checks_Q;

      if (enemy_checks) continue;

      uo_move_type move_type = (uo_square_bitboard(square_to) & mask_enemy) ? uo_move_type__x : uo_move_type__quiet;
      *movelist++ = uo_move_encode(square_from, square_to, move_type);
      ++count;
    }
  }

  // Moves for pinned rooks
  square_from = -1;
  uo_bitboard pinned_R = own_R & pins_to_own_K;
  while (uo_bitboard_next_square(pinned_R, &square_from))
  {
    uo_bitboard moves_R = uo_bitboard_moves_R(square_from, mask_own, mask_enemy);
    uo_bitboard bitboard_from = uo_square_bitboard(square_from);
    uo_bitboard occupied_after_move = occupied & ~bitboard_from;

    square_to = -1;
    while (uo_bitboard_next_square(moves_R, &square_to))
    {
      uo_bitboard bitboard_to = uo_square_bitboard(square_to);
      uo_bitboard enemy_checks_diagonals = uo_bitboard_attacks_B(square_own_K, occupied_after_move | bitboard_to) & ~bitboard_to;
      uo_bitboard enemy_checks_lines = uo_bitboard_attacks_R(square_own_K, occupied_after_move | bitboard_to) & ~bitboard_to;
      uo_bitboard enemy_checks_B = enemy_B & enemy_checks_diagonals;
      uo_bitboard enemy_checks_R = enemy_R & enemy_checks_lines;
      uo_bitboard enemy_checks_Q = enemy_Q & (enemy_checks_lines | enemy_checks_diagonals);
      uo_bitboard enemy_checks = enemy_checks_B | enemy_checks_R | enemy_checks_Q;

      if (enemy_checks) continue;

      uo_move_type move_type = (uo_square_bitboard(square_to) & mask_enemy) ? uo_move_type__x : uo_move_type__quiet;
      *movelist++ = uo_move_encode(square_from, square_to, move_type);
      ++count;
    }
  }

  // Moves for pinned queens
  square_from = -1;
  uo_bitboard pinned_Q = own_Q & pins_to_own_K;
  while (uo_bitboard_next_square(pinned_Q, &square_from))
  {
    uo_bitboard moves_Q = uo_bitboard_moves_Q(square_from, mask_own, mask_enemy);
    uo_bitboard bitboard_from = uo_square_bitboard(square_from);
    uo_bitboard occupied_after_move = occupied & ~bitboard_from;

    square_to = -1;
    while (uo_bitboard_next_square(moves_Q, &square_to))
    {
      uo_bitboard bitboard_to = uo_square_bitboard(square_to);
      uo_bitboard enemy_checks_diagonals = uo_bitboard_attacks_B(square_own_K, occupied_after_move | bitboard_to) & ~bitboard_to;
      uo_bitboard enemy_checks_lines = uo_bitboard_attacks_R(square_own_K, occupied_after_move | bitboard_to) & ~bitboard_to;
      uo_bitboard enemy_checks_B = enemy_B & enemy_checks_diagonals;
      uo_bitboard enemy_checks_R = enemy_R & enemy_checks_lines;
      uo_bitboard enemy_checks_Q = enemy_Q & (enemy_checks_lines | enemy_checks_diagonals);
      uo_bitboard enemy_checks = enemy_checks_B | enemy_checks_R | enemy_checks_Q;

      if (enemy_checks) continue;

      uo_move_type move_type = (uo_square_bitboard(square_to) & mask_enemy) ? uo_move_type__x : uo_move_type__quiet;
      *movelist++ = uo_move_encode(square_from, square_to, move_type);
      ++count;
    }
  }

  // Moves for pinned pawns
  square_from = -1;
  uo_bitboard pinned_P = own_P & pins_to_own_K;
  while (uo_bitboard_next_square(pinned_P, &square_from))
  {
    uo_bitboard moves_P = uo_bitboard_moves_P(square_from, color_to_move, mask_own, mask_enemy);
    uo_bitboard bitboard_from = uo_square_bitboard(square_from);
    uo_bitboard occupied_after_move = occupied & ~bitboard_from;

    square_to = -1;

    if (bitboard_from & bitboard_seventh_rank)
    {
      while (uo_bitboard_next_square(moves_P, &square_to))
      {
        uo_bitboard bitboard_to = uo_square_bitboard(square_to);
        uo_bitboard enemy_checks_diagonals = uo_bitboard_attacks_B(square_own_K, occupied_after_move | bitboard_to) & ~bitboard_to;
        uo_bitboard enemy_checks_lines = uo_bitboard_attacks_R(square_own_K, occupied_after_move | bitboard_to) & ~bitboard_to;
        uo_bitboard enemy_checks_B = enemy_B & enemy_checks_diagonals;
        uo_bitboard enemy_checks_R = enemy_R & enemy_checks_lines;
        uo_bitboard enemy_checks_Q = enemy_Q & (enemy_checks_lines | enemy_checks_diagonals);
        uo_bitboard enemy_checks = enemy_checks_B | enemy_checks_R | enemy_checks_Q;

        if (enemy_checks) continue;

        uo_move_type move_type = (uo_square_bitboard(square_to) & mask_enemy) ? uo_move_type__x : uo_move_type__quiet;

        *movelist++ = uo_move_encode(square_from, square_to, move_type | uo_move_type__promo_Q);
        ++count;
        *movelist++ = uo_move_encode(square_from, square_to, move_type | uo_move_type__promo_N);
        ++count;
        *movelist++ = uo_move_encode(square_from, square_to, move_type | uo_move_type__promo_B);
        ++count;
        *movelist++ = uo_move_encode(square_from, square_to, move_type | uo_move_type__promo_R);
        ++count;
      }
    }
    else
    {
      while (uo_bitboard_next_square(moves_P, &square_to))
      {
        uo_bitboard bitboard_to = uo_square_bitboard(square_to);
        uo_bitboard enemy_checks_diagonals = uo_bitboard_attacks_B(square_own_K, occupied_after_move | bitboard_to) & ~bitboard_to;
        uo_bitboard enemy_checks_lines = uo_bitboard_attacks_R(square_own_K, occupied_after_move | bitboard_to) & ~bitboard_to;
        uo_bitboard enemy_checks_B = enemy_B & enemy_checks_diagonals;
        uo_bitboard enemy_checks_R = enemy_R & enemy_checks_lines;
        uo_bitboard enemy_checks_Q = enemy_Q & (enemy_checks_lines | enemy_checks_diagonals);
        uo_bitboard enemy_checks = enemy_checks_B | enemy_checks_R | enemy_checks_Q;

        if (enemy_checks) continue;

        uo_move_type move_type = (uo_square_bitboard(square_to) & mask_enemy)
          ? uo_move_type__x
          : (square_to == square_from + direction_forwards * 2)
          ? uo_move_type__P_double_push
          : uo_move_type__quiet;

        *movelist++ = uo_move_encode(square_from, square_to, move_type);
        ++count;
      }
    }
  }

  // Pinned pawn en passant captures
  if (enpassant_file)
  {
    uo_bitboard bitboard_from = uo_square_bitboard(square_from);
    uo_bitboard occupied_after_move = occupied & ~bitboard_from;
    uo_bitboard enpassant = bitboard_enpassant_file & bitboard_fifth_rank;
    square_to = uo_ffs(enpassant) - 1 + direction_forwards;

    if (enpassant_file > 1 && (pinned_P & (enpassant >> 1)))
    {
      uo_bitboard enemy_checks_diagonals = uo_bitboard_attacks_B(square_to, occupied_after_move);
      uo_bitboard enemy_checks_lines = uo_bitboard_attacks_R(square_to, occupied_after_move);
      uo_bitboard enemy_checks_B = enemy_B & enemy_checks_diagonals;
      uo_bitboard enemy_checks_R = enemy_R & enemy_checks_lines;
      uo_bitboard enemy_checks_Q = enemy_Q & (enemy_checks_lines | enemy_checks_diagonals);
      uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(square_to);
      uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(square_to, color_to_move);
      uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_B | enemy_checks_R | enemy_checks_Q | enemy_checks_P;

      if (!enemy_checks)
      {
        *movelist++ = uo_move_encode(square_to - 1 + direction_backwards, square_to, uo_move_type__enpassant);
        ++count;
      }
    }

    if (enpassant_file < 8 && (pinned_P & (enpassant << 1)))
    {
      uo_bitboard enemy_checks_diagonals = uo_bitboard_attacks_B(square_to, occupied_after_move);
      uo_bitboard enemy_checks_lines = uo_bitboard_attacks_R(square_to, occupied_after_move);
      uo_bitboard enemy_checks_B = enemy_B & enemy_checks_diagonals;
      uo_bitboard enemy_checks_R = enemy_R & enemy_checks_lines;
      uo_bitboard enemy_checks_Q = enemy_Q & (enemy_checks_lines | enemy_checks_diagonals);
      uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(square_to);
      uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(square_to, color_to_move);
      uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_B | enemy_checks_R | enemy_checks_Q | enemy_checks_P;

      if (!enemy_checks)
      {
        *movelist++ = uo_move_encode(square_to + 1 + direction_backwards, square_to, uo_move_type__enpassant);
        ++count;
      }
    }
  }


  // Finally, let's consider king moves

  // Castling moves

  if (color_to_move == uo_piece__white)
  {
    if (uo_position_flags_castling_K(flags) && !(occupied & (uo_square_bitboard(uo_square__f1) | uo_square_bitboard(uo_square__g1))))
    {
      uo_bitboard enemy_checks_diagonals = uo_bitboard_attacks_B(uo_square__f1, occupied);
      uo_bitboard enemy_checks_lines = uo_bitboard_attacks_R(uo_square__f1, occupied);
      uo_bitboard enemy_checks_B = enemy_B & enemy_checks_diagonals;
      uo_bitboard enemy_checks_R = enemy_R & enemy_checks_lines;
      uo_bitboard enemy_checks_Q = enemy_Q & (enemy_checks_lines | enemy_checks_diagonals);
      uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(uo_square__f1);
      uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(uo_square__f1, color_to_move);
      uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(uo_square__f1);
      uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_B | enemy_checks_R | enemy_checks_Q | enemy_checks_P | enemy_checks_K;

      if (!enemy_checks)
      {
        uo_bitboard enemy_checks_diagonals = uo_bitboard_attacks_B(uo_square__g1, occupied);
        uo_bitboard enemy_checks_lines = uo_bitboard_attacks_R(uo_square__g1, occupied);
        uo_bitboard enemy_checks_B = enemy_B & enemy_checks_diagonals;
        uo_bitboard enemy_checks_R = enemy_R & enemy_checks_lines;
        uo_bitboard enemy_checks_Q = enemy_Q & (enemy_checks_lines | enemy_checks_diagonals);
        uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(uo_square__g1);
        uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(uo_square__g1, color_to_move);
        uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(uo_square__g1);
        uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_B | enemy_checks_R | enemy_checks_Q | enemy_checks_P | enemy_checks_K;

        if (!enemy_checks)
        {
          *movelist++ = uo_move_encode(square_own_K, uo_square__g1, uo_move_type__OO);
          ++count;
        }
      }
    }

    if (uo_position_flags_castling_Q(flags) && !(occupied & (uo_square_bitboard(uo_square__d1) | uo_square_bitboard(uo_square__c1) | uo_square_bitboard(uo_square__b1))))
    {
      uo_bitboard enemy_checks_diagonals = uo_bitboard_attacks_B(uo_square__d1, occupied);
      uo_bitboard enemy_checks_lines = uo_bitboard_attacks_R(uo_square__d1, occupied);
      uo_bitboard enemy_checks_B = enemy_B & enemy_checks_diagonals;
      uo_bitboard enemy_checks_R = enemy_R & enemy_checks_lines;
      uo_bitboard enemy_checks_Q = enemy_Q & (enemy_checks_lines | enemy_checks_diagonals);
      uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(uo_square__d1);
      uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(uo_square__d1, color_to_move);
      uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(uo_square__d1);
      uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_B | enemy_checks_R | enemy_checks_Q | enemy_checks_P | enemy_checks_K;

      if (!enemy_checks)
      {
        uo_bitboard enemy_checks_diagonals = uo_bitboard_attacks_B(uo_square__c1, occupied);
        uo_bitboard enemy_checks_lines = uo_bitboard_attacks_R(uo_square__c1, occupied);
        uo_bitboard enemy_checks_B = enemy_B & enemy_checks_diagonals;
        uo_bitboard enemy_checks_R = enemy_R & enemy_checks_lines;
        uo_bitboard enemy_checks_Q = enemy_Q & (enemy_checks_lines | enemy_checks_diagonals);
        uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(uo_square__c1);
        uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(uo_square__c1, color_to_move);
        uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(uo_square__c1);
        uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_B | enemy_checks_R | enemy_checks_Q | enemy_checks_P | enemy_checks_K;

        if (!enemy_checks)
        {
          *movelist++ = uo_move_encode(square_own_K, uo_square__c1, uo_move_type__OOO);
          ++count;
        }
      }
    }
  }
  else // black to move
  {
    if (uo_position_flags_castling_k(flags) && !(occupied & (uo_square_bitboard(uo_square__f8) | uo_square_bitboard(uo_square__g8))))
    {
      uo_bitboard enemy_checks_diagonals = uo_bitboard_attacks_B(uo_square__f8, occupied);
      uo_bitboard enemy_checks_lines = uo_bitboard_attacks_R(uo_square__f8, occupied);
      uo_bitboard enemy_checks_B = enemy_B & enemy_checks_diagonals;
      uo_bitboard enemy_checks_R = enemy_R & enemy_checks_lines;
      uo_bitboard enemy_checks_Q = enemy_Q & (enemy_checks_lines | enemy_checks_diagonals);
      uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(uo_square__f8);
      uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(uo_square__f8, color_to_move);
      uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(uo_square__f8);
      uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_B | enemy_checks_R | enemy_checks_Q | enemy_checks_P | enemy_checks_K;

      if (!enemy_checks)
      {
        uo_bitboard enemy_checks_diagonals = uo_bitboard_attacks_B(uo_square__g8, occupied);
        uo_bitboard enemy_checks_lines = uo_bitboard_attacks_R(uo_square__g8, occupied);
        uo_bitboard enemy_checks_B = enemy_B & enemy_checks_diagonals;
        uo_bitboard enemy_checks_R = enemy_R & enemy_checks_lines;
        uo_bitboard enemy_checks_Q = enemy_Q & (enemy_checks_lines | enemy_checks_diagonals);
        uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(uo_square__g8);
        uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(uo_square__g8, color_to_move);
        uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(uo_square__g8);
        uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_B | enemy_checks_R | enemy_checks_Q | enemy_checks_P | enemy_checks_K;

        if (!enemy_checks)
        {
          *movelist++ = uo_move_encode(square_own_K, uo_square__g8, uo_move_type__OO);
          ++count;
        }
      }
    }

    if (uo_position_flags_castling_q(flags) && !(occupied & (uo_square_bitboard(uo_square__d8) | uo_square_bitboard(uo_square__c8) | uo_square_bitboard(uo_square__b8))))
    {
      uo_bitboard enemy_checks_diagonals = uo_bitboard_attacks_B(uo_square__d8, occupied);
      uo_bitboard enemy_checks_lines = uo_bitboard_attacks_R(uo_square__d8, occupied);
      uo_bitboard enemy_checks_B = enemy_B & enemy_checks_diagonals;
      uo_bitboard enemy_checks_R = enemy_R & enemy_checks_lines;
      uo_bitboard enemy_checks_Q = enemy_Q & (enemy_checks_lines | enemy_checks_diagonals);
      uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(uo_square__d8);
      uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(uo_square__d8, color_to_move);
      uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(uo_square__d8);
      uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_B | enemy_checks_R | enemy_checks_Q | enemy_checks_P | enemy_checks_K;

      if (!enemy_checks)
      {
        uo_bitboard enemy_checks_diagonals = uo_bitboard_attacks_B(uo_square__c8, occupied);
        uo_bitboard enemy_checks_lines = uo_bitboard_attacks_R(uo_square__c8, occupied);
        uo_bitboard enemy_checks_B = enemy_B & enemy_checks_diagonals;
        uo_bitboard enemy_checks_R = enemy_R & enemy_checks_lines;
        uo_bitboard enemy_checks_Q = enemy_Q & (enemy_checks_lines | enemy_checks_diagonals);
        uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(uo_square__c8);
        uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(uo_square__c8, color_to_move);
        uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(uo_square__c8);
        uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_B | enemy_checks_R | enemy_checks_Q | enemy_checks_P | enemy_checks_K;

        if (!enemy_checks)
        {
          *movelist++ = uo_move_encode(square_own_K, uo_square__c8, uo_move_type__OOO);
          ++count;
        }
      }
    }
  }

  // Non castling king moves

  square_to = -1;
  while (uo_bitboard_next_square(moves_K, &square_to))
  {
    uo_bitboard enemy_checks_diagonals = uo_bitboard_attacks_B(square_to, occupied & ~own_K);
    uo_bitboard enemy_checks_lines = uo_bitboard_attacks_R(square_to, occupied & ~own_K);
    uo_bitboard enemy_checks_B = enemy_B & enemy_checks_diagonals;
    uo_bitboard enemy_checks_R = enemy_R & enemy_checks_lines;
    uo_bitboard enemy_checks_Q = enemy_Q & (enemy_checks_lines | enemy_checks_diagonals);
    uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_attacks_N(square_to);
    uo_bitboard enemy_checks_P = enemy_P & uo_bitboard_attacks_P(square_to, color_to_move);
    uo_bitboard enemy_checks_K = enemy_K & uo_bitboard_attacks_K(square_to);
    uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_B | enemy_checks_R | enemy_checks_Q | enemy_checks_P | enemy_checks_K;

    if (enemy_checks) continue;

    uo_move_type move_type = (uo_square_bitboard(square_to) & mask_enemy) ? uo_move_type__x : uo_move_type__quiet;
    *movelist++ = uo_move_encode(square_own_K, square_to, move_type);
    ++count;
  }

  return count;
}

uo_move uo_position_parse_move(uo_position *position, char str[5])
{
  if (!str) return 0;

  char *ptr = str;

  int file_from = ptr[0] - 'a';
  if (file_from < 0 || file_from > 7) return 0;
  int rank_from = ptr[1] - '1';
  if (rank_from < 0 || rank_from > 7) return 0;
  uo_square square_from = (rank_from << 3) + file_from;

  int file_to = ptr[2] - 'a';
  if (file_to < 0 || file_to > 7) return 0;
  int rank_to = ptr[3] - '1';
  if (rank_to < 0 || rank_to > 7) return 0;
  uo_square square_to = (rank_to << 3) + file_to;

  uo_bitboard bitboard_from = uo_square_bitboard(square_from);
  uo_bitboard bitboard_to = uo_square_bitboard(square_to);
  uo_move_type move_type = uo_move_type__quiet;
  uo_piece piece = uo_position_square_piece(position, square_from);
  uo_piece piece_captured = uo_position_square_piece(position, square_to);
  uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);

  if (piece_captured)
  {
    move_type |= uo_move_type__x;
  }
  else if (piece & uo_piece__K)
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
  else if (piece & uo_piece__P)
  {
    if (uo_square_file(square_from) != uo_square_file(square_to))
    {
      move_type = uo_move_type__enpassant;
    }
    else
    {
      if (piece & uo_piece__white)
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

  char promotion = ptr[4];
  switch (promotion)
  {
    case 'q': move_type |= uo_move_type__promo_Q; break;
    case 'r': move_type |= uo_move_type__promo_R; break;
    case 'b': move_type |= uo_move_type__promo_B; break;
    case 'n': move_type |= uo_move_type__promo_N; break;
    default: break;
  }

  return uo_move_encode(square_from, square_to, move_type);
}

#pragma region evaluation_features

static double uo_position_evaluation_feature_material_P(uo_position *position, uint8_t color)
{
  return uo_popcount(position->P & (color ? position->piece_color : ~position->piece_color));
}
static double uo_position_evaluation_feature_material_N(uo_position *position, uint8_t color)
{
  return uo_popcount(position->N & (color ? position->piece_color : ~position->piece_color));
}
static double uo_position_evaluation_feature_material_B(uo_position *position, uint8_t color)
{
  return uo_popcount(position->B & (color ? position->piece_color : ~position->piece_color));
}
static double uo_position_evaluation_feature_material_R(uo_position *position, uint8_t color)
{
  return uo_popcount(position->R & (color ? position->piece_color : ~position->piece_color));
}
static double uo_position_evaluation_feature_material_Q(uo_position *position, uint8_t color)
{
  return uo_popcount(position->Q & (color ? position->piece_color : ~position->piece_color));
}

#pragma endregion

static const double (*eval_features[])(uo_position *position, uint8_t color) = {
  uo_position_evaluation_feature_material_P,
  uo_position_evaluation_feature_material_N,
  uo_position_evaluation_feature_material_B,
  uo_position_evaluation_feature_material_R,
  uo_position_evaluation_feature_material_Q
};

#define UO_EVAL_FEATURE_COUNT (sizeof eval_features / sizeof *eval_features)

double weights[UO_EVAL_FEATURE_COUNT << 1] =
{
   1.0,
  -1.0,
   3.0,
  -3.0,
   3.0,
  -3.0,
   5.0,
  -5.0,
   9.0,
  -9.0
};

double uo_position_evaluate(uo_position *position)
{
  double inputs[UO_EVAL_FEATURE_COUNT << 1] = { 0 };
  double evaluation = 0;

  for (int i = 0; i < UO_EVAL_FEATURE_COUNT; ++i)
  {
    inputs[i << 1] = eval_features[i](position, uo_piece__white);
    inputs[(i << 1) + 1] = eval_features[i](position, uo_piece__black);
    evaluation += inputs[i << 1] * weights[i << 1];
    evaluation += inputs[(i << 1) + 1] * weights[(i << 1) + 1];
  }

  return evaluation;
}
