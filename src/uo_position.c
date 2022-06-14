#include "uo_position.h"
#include "uo_util.h"
#include "uo_move.h"
#include "uo_piece.h"
#include "uo_square.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

// see: https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation
// example fen: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
uo_position *uo_position_from_fen(uo_position *position, char *fen)
{
  uo_position_flags flags;
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

      uo_bitboard *bitboard = uo_position__piece_bitboard(position, piece);
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
  flags = uo_position_flags_update_halfmoves(halfmoves, true);
  position->fullmove = fullmove;

  position->flags = flags;

  return position;
}

char *uo_position_to_diagram(uo_position *position, char diagram[72])
{
  for (int i = 0; i < 64; ++i)
  {
    uo_piece piece = uo_position_square_piece(position, i);
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
  uo_position_flags flags_prev = position->flags;
  uo_position_flags flags_new = flags_prev;
  uint8_t color_to_move = uo_position_flags_color_to_move(flags_prev);
  uint8_t halfmoves = uo_position_flags_halfmoves(flags_prev);
  uint8_t castling = uo_position_flags_castling(flags_prev);
  uint8_t enpassant_file = uo_position_flags_enpassant_file(flags_prev);


  *bitboard &= ~bitboard_from;
  flags_new = uo_position_flags_update_enpassant_file(flags_new, 0);

  if (color_to_move == uo_piece__black)
  {
    position->piece_color |= bitboard_to;

    if (move_type == uo_move_type__P_double_push)
    {
      // en passant
      uo_bitboard adjecent_enemy_pawn = ((bitboard_to << 1) | (bitboard_to >> 1)) & uo_bitboard_rank[4] & ~position->piece_color & position->P;
      if (adjecent_enemy_pawn) {
        flags_new = uo_position_flags_update_enpassant_file(flags_new, uo_square_file(square_to) + 1);
      }
    }
    else if (move_type & uo_move_type__enpassant)
    {
      // en passant capture
      uo_bitboard enpassant = uo_square_bitboard(31 + enpassant_file);
      *bitboard &= ~enpassant;
    }
    else if (piece & uo_piece__K)
    {
      flags_new = uo_position_flags_update_castling_black(flags_new, false);
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
  else
  {
    position->piece_color &= ~bitboard_to;

    if (move_type == uo_move_type__P_double_push)
    {
      // en passant - white
      uo_bitboard adjecent_enemy_pawn = ((bitboard_to << 1) | (bitboard_to >> 1)) & uo_bitboard_rank[3] & position->piece_color & position->P;
      if (adjecent_enemy_pawn) {
        flags_new = uo_position_flags_update_enpassant_file(flags_new, uo_square_file(square_to) + 1);
      }
    }
    else if (move_type & uo_move_type__enpassant)
    {
      // en passant capture
      uo_bitboard enpassant = uo_square_bitboard(23 + enpassant_file);
      *bitboard &= ~enpassant;
    }
    else if (piece & uo_piece__K)
    {
      flags_new = uo_position_flags_update_castling_white(flags_new, false);
    }
    else if (square_from == uo_square__a8)
    {
      flags_new = uo_position_flags_update_castling_Q(flags_new, false);
    }
    else if (square_from == uo_square__h8)
    {
      flags_new = uo_position_flags_update_castling_K(flags_new, false);
    }
  }

  uo_piece piece_captured = 0;

  // capture
  if (move_type & uo_move_type__x)
  {
    flags_new = uo_position_flags_update_halfmoves(flags_new, 0);

    piece_captured = uo_position_square_piece(position, square_to);
    uo_bitboard *bitboard_captured = uo_position_piece_bitboard(position, piece_captured);
    *bitboard_captured &= ~bitboard_to;
  }
  // pawn move
  else if (piece & uo_piece__P)
  {
    flags_new = uo_position_flags_update_halfmoves(flags_new, 0);
  }

  // promotion
  if (move_type & uo_move_type__promo)
  {
    if (move_type & uo_move_type__promo_Q)
    {
      position->Q |= bitboard_to;
    }
    else if (move_type & uo_move_type__promo_R)
    {
      position->R |= bitboard_to;
    }
    else if (move_type & uo_move_type__promo_B)
    {
      position->B |= bitboard_to;
    }
    else // uo_move_type__promo_N
    {
      position->N |= bitboard_to;
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
  uint8_t color_to_move = uo_position_flags_color_to_move(position->flags);

  --position->fullmove;

  uo_piece piece;

  // promotion
  if (move_type & uo_move_type__promo)
  {
    if (move_type & uo_move_type__promo_Q)
    {
      position->Q &= ~bitboard_to;
    }
    else if (move_type & uo_move_type__promo_R)
    {
      position->R &= ~bitboard_to;
    }
    else if (move_type & uo_move_type__promo_B)
    {
      position->B &= ~bitboard_to;
    }
    else // uo_move_type__promo_N
    {
      position->N &= ~bitboard_to;
    }

    piece = uo_piece__P;
  }
  else
  {
    piece = uo_position_square_piece(position, square_to);
  }

  // capture
  if (move_type & uo_move_type__x)
  {
    uo_piece piece_captured = uo_move_ex_piece_captured(move);
    uo_bitboard *bitboard_captured = uo_position_piece_bitboard(position, piece_captured);
    *bitboard_captured |= bitboard_to;

    if (color_to_move == uo_piece__black)
    {
      if (move_type & uo_move_type__enpassant)
      {
        // en passant capture
        uo_bitboard enpassant = bitboard_to >> 8;
        position->P |= enpassant;
        position->piece_color &= ~enpassant;
      }
      else
      {
        position->piece_color &= ~bitboard_to;
      }
    }
    else
    {
      if (move_type & uo_move_type__enpassant)
      {
        // en passant capture
        uo_bitboard enpassant = bitboard_to << 8;
        position->P |= enpassant;
        position->piece_color |= enpassant;
      }
      else
      {
        position->piece_color |= bitboard_to;
      }
    }
  }

  uo_bitboard *bitboard = uo_position_piece_bitboard(position, piece);
  *bitboard |= bitboard_from;

  position->flags = flags_prev;
}

size_t uo_position_get_moves(uo_position *position, uo_move_ex *movelist)
{
  size_t count = 0;
  uo_square square;

  uo_position_flags flags = position->flags;
  uint8_t color_to_move = uo_position_flags_color_to_move(flags);
  uint8_t halfmoves = uo_position_flags_halfmoves(flags);
  uint8_t castling = uo_position_flags_castling(flags);
  uint8_t enpassant_file = uo_position_flags_enpassant_file(flags);

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

  if (color_to_move)
  {
    temp = mask_own;
    mask_own = mask_enemy;
    mask_enemy = temp;
    piece_enemy_P |= uo_piece__black;
  }
  else
  {
    piece_enemy_P |= uo_piece__black;
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
  uo_bitboard moves_K = uo_bitboard_moves(square_own_K, uo_piece__K, occupied);

  uo_bitboard enemy_checks_diagonals = uo_bitboard_moves(square_own_K, uo_piece__B, occupied);
  uo_bitboard enemy_checks_lines = uo_bitboard_moves(square_own_K, uo_piece__R, occupied);
  uo_bitboard enemy_checks_B = enemy_B & enemy_checks_diagonals;
  uo_bitboard enemy_checks_R = enemy_R & enemy_checks_lines;
  uo_bitboard enemy_checks_Q = enemy_Q & (enemy_checks_diagonals | enemy_checks_diagonals);
  uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_moves(square_own_K, uo_piece__N, 0);
  uo_bitboard enemy_checks_P = (enemy_P & ~uo_bitboard_file[file_own_K]) & uo_bitboard_moves(square_own_K, piece_enemy_P, occupied);
  uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_B | enemy_checks_R | enemy_checks_Q | enemy_checks_P;

  uo_bitboard pins_to_own_K = uo_bitboard_pins(square_own_K, occupied, enemy_B | enemy_Q, enemy_R | enemy_Q);

  if (enemy_checks)
  {
    uo_square square_enemy_checker = uo_ffs(enemy_checks);

    if (uo_ffs(enemy_checks >> square_enemy_checker))
    {
      // Double check, king must move

      square = -1;
      while (uo_bitboard_next_square(moves_K, &square))
      {
        uo_bitboard enemy_checks_diagonals = uo_bitboard_moves(square, uo_piece__B, occupied);
        uo_bitboard enemy_checks_lines = uo_bitboard_moves(square, uo_piece__R, occupied);
        uo_bitboard enemy_checks_B = enemy_B & enemy_checks_diagonals;
        uo_bitboard enemy_checks_R = enemy_R & enemy_checks_lines;
        uo_bitboard enemy_checks_Q = enemy_Q & (enemy_checks_diagonals | enemy_checks_diagonals);
        uo_bitboard enemy_checks_N = enemy_N & uo_bitboard_moves(square, uo_piece__N, 0);
        uo_bitboard enemy_checks_P = (enemy_P & ~uo_bitboard_file[uo_square_file(square)]) & uo_bitboard_moves(square, piece_enemy_P, occupied);
        uo_bitboard enemy_checks = enemy_checks_N | enemy_checks_B | enemy_checks_R | enemy_checks_Q | enemy_checks_P;

        if (enemy_checks) continue;

        uo_move_type move_type = (uo_square_bitboard(square) & mask_enemy) ? uo_move_type__x : uo_move_type__quiet;
        *movelist++ = uo_move_encode(square_own_K, square, move_type);
        ++count;
      }

      return count;
    }

    // Only one piece is giving a check

    // Let's first consider captures by each piece type

    uo_bitboard attack_checker_diagonals = uo_bitboard_moves(square_enemy_checker, uo_piece__B, occupied);
    uo_bitboard attack_checker_lines = uo_bitboard_moves(square_enemy_checker, uo_piece__R, occupied);
    uo_bitboard attack_checker_B = own_B & attack_checker_diagonals;
    uo_bitboard attack_checker_R = own_R & attack_checker_lines;
    uo_bitboard attack_checker_Q = own_Q & (attack_checker_diagonals | attack_checker_diagonals);
    uo_bitboard attack_checker_N = own_N & uo_bitboard_moves(square_enemy_checker, uo_piece__N, 0);
    uo_bitboard attack_checker_P = (own_P & ~uo_bitboard_file[uo_square_file(square_enemy_checker)]) & uo_bitboard_moves(square_enemy_checker, piece_own_P, occupied);
    uo_bitboard attack_checker = attack_checker_N | attack_checker_B | attack_checker_R | attack_checker_Q | attack_checker_P;
    attack_checker &= pins_to_own_K;

    square = -1;
    while (uo_bitboard_next_square(attack_checker, &square))
    {
      uo_move_type move_type = uo_move_type__x;

      if ((own_P & uo_square_bitboard(square)) & (uo_bitboard_rank[0] | uo_bitboard_rank[7]))
      {
        *movelist++ = uo_move_encode(square, square_enemy_checker, uo_move_type__promo_Qx);
        ++count;
        *movelist++ = uo_move_encode(square, square_enemy_checker, uo_move_type__promo_Nx);
        ++count;
        *movelist++ = uo_move_encode(square, square_enemy_checker, uo_move_type__promo_Bx);
        ++count;
        *movelist++ = uo_move_encode(square, square_enemy_checker, uo_move_type__promo_Rx);
        ++count;
      }
      else
      {
        *movelist++ = uo_move_encode(square, square_enemy_checker, uo_move_type__x);
        ++count;
      }
    }
  }
}
