#define DECOMP64

#include "uo_tb.h"
#include "uo_zobrist.h"
#include "uo_piece.h"
#include "uo_def.h"
#include "uo_util.h"
#include "tbcore.h"

int TBlargest = 0;

#include "tbcore.c"

// Given a position with 6 or fewer pieces, produce a text string
// of the form KQPvKRP, where "KQP" represents pieces for side to move
// or if mirror == true, the opposing side
static void uo_tb_print_filename(uo_position *position, char *str, bool mirror)
{
  size_t piece_count_Q = uo_popcnt(position->Q);
  size_t piece_count_own_Q = uo_popcnt(position->Q & position->own);
  size_t piece_count_enemy_Q = piece_count_Q - piece_count_own_Q;
  size_t piece_count_R = uo_popcnt(position->R);
  size_t piece_count_own_R = uo_popcnt(position->R & position->own);
  size_t piece_count_enemy_R = piece_count_R - piece_count_own_R;
  size_t piece_count_B = uo_popcnt(position->B);
  size_t piece_count_own_B = uo_popcnt(position->B & position->own);
  size_t piece_count_enemy_B = piece_count_B - piece_count_own_B;
  size_t piece_count_N = uo_popcnt(position->N);
  size_t piece_count_own_N = uo_popcnt(position->N & position->own);
  size_t piece_count_enemy_N = piece_count_N - piece_count_own_N;
  size_t piece_count_P = uo_popcnt(position->P);
  size_t piece_count_own_P = uo_popcnt(position->P & position->own);
  size_t piece_count_enemy_P = piece_count_P - piece_count_own_P;

  if (mirror)
  {
    *str++ = 'K';
    for (size_t i = 0; i < piece_count_enemy_Q; ++i) *str++ = 'Q';
    for (size_t i = 0; i < piece_count_enemy_R; ++i) *str++ = 'R';
    for (size_t i = 0; i < piece_count_enemy_B; ++i) *str++ = 'B';
    for (size_t i = 0; i < piece_count_enemy_N; ++i) *str++ = 'N';
    for (size_t i = 0; i < piece_count_enemy_P; ++i) *str++ = 'P';
    *str++ = 'v';
  }

  *str++ = 'K';
  for (size_t i = 0; i < piece_count_own_Q; ++i) *str++ = 'Q';
  for (size_t i = 0; i < piece_count_own_R; ++i) *str++ = 'R';
  for (size_t i = 0; i < piece_count_own_B; ++i) *str++ = 'B';
  for (size_t i = 0; i < piece_count_own_N; ++i) *str++ = 'N';
  for (size_t i = 0; i < piece_count_own_P; ++i) *str++ = 'P';

  if (!mirror)
  {
    *str++ = 'v';
    *str++ = 'K';
    for (size_t i = 0; i < piece_count_enemy_Q; ++i) *str++ = 'Q';
    for (size_t i = 0; i < piece_count_enemy_R; ++i) *str++ = 'R';
    for (size_t i = 0; i < piece_count_enemy_B; ++i) *str++ = 'B';
    for (size_t i = 0; i < piece_count_enemy_N; ++i) *str++ = 'N';
    for (size_t i = 0; i < piece_count_enemy_P; ++i) *str++ = 'P';
  }

  *str++ = '\0';
}

uint64_t uo_tb_material_key_KvK;

static void uo_tb_init_material_keys()
{
  uint64_t key = 0;
  key ^= uo_zobkey(uo_piece__q, 0);
  key ^= uo_zobkey(uo_piece__r, 0);
  key ^= uo_zobkey(uo_piece__b, 0);
  key ^= uo_zobkey(uo_piece__n, 0);
  key ^= uo_zobkey(uo_piece__p, 0);
  key ^= uo_zobkey(uo_piece__Q, 0);
  key ^= uo_zobkey(uo_piece__R, 0);
  key ^= uo_zobkey(uo_piece__B, 0);
  key ^= uo_zobkey(uo_piece__N, 0);
  key ^= uo_zobkey(uo_piece__P, 0);
  uo_tb_material_key_KvK = key;
}

static uint64_t uo_tb_calculate_material_key(uo_position *position, bool mirror)
{
  size_t piece_count_Q = uo_popcnt(position->Q);
  size_t piece_count_own_Q = uo_popcnt(position->Q & position->own);
  size_t piece_count_enemy_Q = piece_count_Q - piece_count_own_Q;
  size_t piece_count_R = uo_popcnt(position->R);
  size_t piece_count_own_R = uo_popcnt(position->R & position->own);
  size_t piece_count_enemy_R = piece_count_R - piece_count_own_R;
  size_t piece_count_B = uo_popcnt(position->B);
  size_t piece_count_own_B = uo_popcnt(position->B & position->own);
  size_t piece_count_enemy_B = piece_count_B - piece_count_own_B;
  size_t piece_count_N = uo_popcnt(position->N);
  size_t piece_count_own_N = uo_popcnt(position->N & position->own);
  size_t piece_count_enemy_N = piece_count_N - piece_count_own_N;
  size_t piece_count_P = uo_popcnt(position->P);
  size_t piece_count_own_P = uo_popcnt(position->P & position->own);
  size_t piece_count_enemy_P = piece_count_P - piece_count_own_P;

  uint64_t key = 0;
  if (mirror)
  {
    key ^= uo_zobkey(uo_piece__q, piece_count_own_Q);
    key ^= uo_zobkey(uo_piece__r, piece_count_own_R);
    key ^= uo_zobkey(uo_piece__b, piece_count_own_B);
    key ^= uo_zobkey(uo_piece__n, piece_count_own_N);
    key ^= uo_zobkey(uo_piece__p, piece_count_own_P);
    key ^= uo_zobkey(uo_piece__Q, piece_count_enemy_Q);
    key ^= uo_zobkey(uo_piece__R, piece_count_enemy_R);
    key ^= uo_zobkey(uo_piece__B, piece_count_enemy_B);
    key ^= uo_zobkey(uo_piece__N, piece_count_enemy_N);
    key ^= uo_zobkey(uo_piece__P, piece_count_enemy_P);
  }
  else
  {
    key ^= uo_zobkey(uo_piece__Q, piece_count_own_Q);
    key ^= uo_zobkey(uo_piece__R, piece_count_own_R);
    key ^= uo_zobkey(uo_piece__B, piece_count_own_B);
    key ^= uo_zobkey(uo_piece__N, piece_count_own_N);
    key ^= uo_zobkey(uo_piece__P, piece_count_own_P);
    key ^= uo_zobkey(uo_piece__q, piece_count_enemy_Q);
    key ^= uo_zobkey(uo_piece__r, piece_count_enemy_R);
    key ^= uo_zobkey(uo_piece__b, piece_count_enemy_B);
    key ^= uo_zobkey(uo_piece__n, piece_count_enemy_N);
    key ^= uo_zobkey(uo_piece__p, piece_count_enemy_P);
  }

  return key;
}

static uint64_t uo_tb_calculate_material_key_from_pcs(int *pcs, int mirror)
{
  size_t piece_count_own_Q = pcs[5];
  size_t piece_count_enemy_Q = pcs[13];
  size_t piece_count_own_R = pcs[4];
  size_t piece_count_enemy_R = pcs[12];
  size_t piece_count_own_B = pcs[3];
  size_t piece_count_enemy_B = pcs[11];
  size_t piece_count_own_N = pcs[2];
  size_t piece_count_enemy_N = pcs[10];
  size_t piece_count_own_P = pcs[1];
  size_t piece_count_enemy_P = pcs[9];

  uint64_t key = 0;
  if (mirror)
  {
    key ^= uo_zobkey(uo_piece__q, piece_count_own_Q);
    key ^= uo_zobkey(uo_piece__r, piece_count_own_R);
    key ^= uo_zobkey(uo_piece__b, piece_count_own_B);
    key ^= uo_zobkey(uo_piece__n, piece_count_own_N);
    key ^= uo_zobkey(uo_piece__p, piece_count_own_P);
    key ^= uo_zobkey(uo_piece__Q, piece_count_enemy_Q);
    key ^= uo_zobkey(uo_piece__R, piece_count_enemy_R);
    key ^= uo_zobkey(uo_piece__B, piece_count_enemy_B);
    key ^= uo_zobkey(uo_piece__N, piece_count_enemy_N);
    key ^= uo_zobkey(uo_piece__P, piece_count_enemy_P);
  }
  else
  {
    key ^= uo_zobkey(uo_piece__Q, piece_count_own_Q);
    key ^= uo_zobkey(uo_piece__R, piece_count_own_R);
    key ^= uo_zobkey(uo_piece__B, piece_count_own_B);
    key ^= uo_zobkey(uo_piece__N, piece_count_own_N);
    key ^= uo_zobkey(uo_piece__P, piece_count_own_P);
    key ^= uo_zobkey(uo_piece__q, piece_count_enemy_Q);
    key ^= uo_zobkey(uo_piece__r, piece_count_enemy_R);
    key ^= uo_zobkey(uo_piece__b, piece_count_enemy_B);
    key ^= uo_zobkey(uo_piece__n, piece_count_enemy_N);
    key ^= uo_zobkey(uo_piece__p, piece_count_enemy_P);
  }

  return key;
}

// p[i] is to contain the square 0-63 (A1-H8) for a piece of type
// pc[i], where 1 = white pawn, ..., 14 = black king
// Pieces of the same type are guaranteed to be consecutive.
static inline int uo_tb_fill_squares(uo_position *position, uint8_t *pc, bool flip_color, int mirror, int *p, int i)
{
  uint8_t color = (pc[i] >> 3) ^ flip_color;
  uo_piece piece = (pc[i] & 0x07) << 1;
  uo_bitboard bitboard = *uo_position_piece_bitboard(position, piece) & (color ? position->enemy : position->own);
  while (bitboard)
  {
    uo_square square = uo_bitboard_next_square(&bitboard);
    p[i++] = square ^ mirror;
  }

  return i;
}

// probe_wdl_table and probe_dtz_table require similar adaptations.
static int uo_tb_probe_wdl_table(uo_position *position, int *success)
{
  struct TBEntry *ptr;
  struct TBHashEntry *hash_entry;
  uint64 idx;
  int i;
  ubyte res;
  int p[TBPIECES];

  // Obtain the position's material signature key.
  uint64 key = uo_tb_calculate_material_key(position, false);

  // Test for KvK.
  if (key == uo_tb_material_key_KvK) return 0;

  hash_entry = TB_hash[key >> (64 - TBHASHBITS)];

  for (i = 0; i < HSHMAX; ++i)
  {
    if (hash_entry[i].key == key) break;
  }

  if (i == HSHMAX)
  {
    *success = 0;
    return 0;
  }

  ptr = hash_entry[i].ptr;

  if (!ptr->ready)
  {
    uo_mutex_lock(TB_mutex);
    if (!ptr->ready)
    {
      char filename[16];
      uo_tb_print_filename(position, filename, ptr->key != key);
      if (!init_table_wdl(ptr, filename))
      {
        hash_entry[i].key = 0ULL;
        *success = 0;
        uo_mutex_unlock(TB_mutex);
        return 0;
      }
      //// Memory barrier to ensure entry->ready = 1 is not reordered.
      //__asm__ __volatile__ ("" ::: "memory");
      ptr->ready = 1;
    }
    uo_mutex_unlock(TB_mutex);
  }

  bool flip = !ptr->symmetric && key != ptr->key;

  if (!ptr->has_pawns)
  {
    struct TBEntry_piece *entry = (struct TBEntry_piece *)ptr;
    ubyte *pc = entry->pieces[flip];

    for (i = 0; i < entry->num;)
    {
      i = uo_tb_fill_squares(position, pc, flip, 0, p, i);
    }

    idx = encode_piece(entry, entry->norm[flip], p, entry->factor[flip]);
    res = decompress_pairs(entry->precomp[flip], idx);
  }
  else
  {
    struct TBEntry_pawn *entry = (struct TBEntry_pawn *)ptr;
    int i = uo_tb_fill_squares(position, entry->file[0].pieces[0], flip, flip ? 56 : 0, p, 0);
    int f = pawn_file(entry, p);
    ubyte *pc = entry->file[f].pieces[flip];

    while (i < entry->num)
    {
      i = uo_tb_fill_squares(position, pc, flip, flip ? 56 : 0, p, i);
    }

    idx = encode_pawn(entry, entry->file[f].norm[flip], p, entry->file[f].factor[flip]);
    res = decompress_pairs(entry->file[f].precomp[flip], idx);
  }

  return (int)res - 2;
}

// The value of wdl MUST correspond to the WDL value of the position without
// en passant rights.
static int uo_tb_probe_dtz_table(uo_position *position, int wdl, int *success)
{
  struct TBEntry *ptr;
  uint64 idx;
  int i, res;
  int p[TBPIECES];

  // Obtain the position's material signature key.
  uint64 key = uo_tb_calculate_material_key(position, false);

  if (DTZ_table[0].key1 != key && DTZ_table[0].key2 != key)
  {
    for (i = 1; i < DTZ_ENTRIES; ++i)
    {
      if (DTZ_table[i].key1 == key || DTZ_table[i].key2 == key) break;
    }

    if (i < DTZ_ENTRIES)
    {
      struct DTZTableEntry table_entry = DTZ_table[i];

      for (; i > 0; i--)
      {
        DTZ_table[i] = DTZ_table[i - 1];
      }

      DTZ_table[0] = table_entry;
    }
    else
    {
      struct TBHashEntry *hash_entry = TB_hash[key >> (64 - TBHASHBITS)];
      for (i = 0; i < HSHMAX; ++i)
      {
        if (hash_entry[i].key == key) break;
      }

      if (i == HSHMAX)
      {
        *success = 0;
        return 0;
      }

      ptr = hash_entry[i].ptr;
      char filename[16];
      bool mirror = (ptr->key != key);
      uo_tb_print_filename(position, filename, mirror);

      if (DTZ_table[DTZ_ENTRIES - 1].entry)
      {
        free_dtz_entry(DTZ_table[DTZ_ENTRIES - 1].entry);
      }

      for (i = DTZ_ENTRIES - 1; i > 0; i--)
      {
        DTZ_table[i] = DTZ_table[i - 1];
      }

      uint64 key_mirror = uo_tb_calculate_material_key(position, true);
      load_dtz_table(filename, mirror ? key_mirror : key, !mirror ? key_mirror : key);
    }
  }

  ptr = DTZ_table[0].entry;
  if (!ptr)
  {
    *success = 0;
    return 0;
  }

  bool flip = !ptr->symmetric && key != ptr->key;

  if (!ptr->has_pawns)
  {
    struct DTZEntry_piece *entry = (struct DTZEntry_piece *)ptr;
    if ((entry->flags & 1) != flip && !entry->symmetric)
    {
      *success = -1;
      return 0;
    }

    ubyte *pc = entry->pieces;

    for (i = 0; i < entry->num;)
    {
      i = uo_tb_fill_squares(position, pc, flip, 0, p, i);
    }

    idx = encode_piece((struct TBEntry_piece *)entry, entry->norm, p, entry->factor);
    res = decompress_pairs(entry->precomp, idx);

    if (entry->flags & 2)
    {
      res = entry->map[entry->map_idx[wdl_to_map[wdl + 2]] + res];
    }

    if (!(entry->flags & pa_flags[wdl + 2]) || (wdl & 1))
    {
      res *= 2;
    }
  }
  else
  {
    struct DTZEntry_pawn *entry = (struct DTZEntry_pawn *)ptr;
    int i = uo_tb_fill_squares(position, entry->file[0].pieces, flip, flip ? 56 : 0, p, 0);
    int f = pawn_file((struct TBEntry_pawn *)entry, p);

    if ((entry->flags[f] & 1) != flip)
    {
      *success = -1;
      return 0;
    }

    ubyte *pc = entry->file[f].pieces;

    while (i < entry->num)
    {
      i = uo_tb_fill_squares(position, pc, flip, flip ? 56 : 0, p, i);
    }

    idx = encode_pawn((struct TBEntry_pawn *)entry, entry->file[f].norm, p, entry->file[f].factor);
    res = decompress_pairs(entry->file[f].precomp, idx);

    if (entry->flags[f] & 2)
    {
      res = entry->map[entry->map_idx[f][wdl_to_map[wdl + 2]] + res];
    }

    if (!(entry->flags[f] & pa_flags[wdl + 2]) || (wdl & 1))
    {
      res *= 2;
    }
  }

  return res;
}

static int uo_tb_probe_alpha_beta(uo_position *position, int alpha, int beta, int *success)
{
  int value;

  // Generate legal moves
  size_t move_count = position->stack->moves_generated
    ? position->stack->move_count
    : uo_position_generate_moves(position);

  for (size_t i = 0; i < move_count; ++i)
  {
    uo_move move = position->movelist.head[i];
    if (!uo_move_is_capture(move)) continue;

    uo_position_make_move(position, move);
    value = -uo_tb_probe_alpha_beta(position, -beta, -alpha, success);
    uo_position_unmake_move(position);

    if (*success == 0) return 0;

    if (value > alpha)
    {
      if (value >= beta)
      {
        return value;
      }
      alpha = value;
    }
  }

  value = uo_tb_probe_wdl_table(position, success);

  return alpha >= value ? alpha : value;
}

// Probe the WDL table for a particular position.
//
// If *success != 0, the probe was successful.
//
// If *success == 2, the position has a winning capture, or the position
// is a cursed win and has a cursed winning capture, or the position
// has an ep capture as only best move.
// This is used in probe_dtz().
//
// The return value is from the point of view of the side to move:
// -2 : loss
// -1 : loss, but draw under 50-move rule
//  0 : draw
//  1 : win, but draw under 50-move rule
//  2 : win
int uo_tb_probe_wdl(uo_position *position, int *success)
{
  *success = 1;

  // Generate legal moves
  size_t move_count = position->stack->moves_generated
    ? position->stack->move_count
    : uo_position_generate_moves(position);

  int best_cap = -3;
  int best_ep = -3;

  // We do capture resolution, letting best_cap keep track of the best
  // capture without ep rights and letting best_ep keep track of still
  // better ep captures if they exist.

  for (size_t i = 0; i < move_count; ++i)
  {
    uo_move move = position->movelist.head[i];
    if (!uo_move_is_capture(move)) continue;

    uo_position_make_move(position, move);
    int value = -uo_tb_probe_alpha_beta(position, -2, -best_cap, success);
    uo_position_unmake_move(position);

    if (*success == 0) return 0;

    if (value > best_cap)
    {
      if (value == 2)
      {
        *success = 2;
        return 2;
      }

      if (!uo_move_is_enpassant(move))
      {
        best_cap = value;
      }
      else if (value > best_ep)
      {
        best_ep = value;
      }
    }
  }

  int value = uo_tb_probe_wdl_table(position, success);
  if (*success == 0) return 0;

  // Now max(v, best_cap) is the WDL value of the position without ep rights.
  // If the position without ep rights is not stalemate or no ep captures
  // exist, then the value of the position is max(v, best_cap, best_ep).
  // If the position without ep rights is stalemate and best_ep > -3,
  // then the value of the position is best_ep (and we will have v == 0).

  if (best_ep > best_cap)
  {
    if (best_ep > value)
    {
      // ep capture (possibly cursed losing) is best.
      *success = 2;
      return best_ep;
    }

    best_cap = best_ep;
  }

  // Now max(v, best_cap) is the WDL value of the position unless
  // the position without ep rights is stalemate and best_ep > -3.

  if (best_cap >= value)
  {
    // No need to test for the stalemate case here: either there are
    // non-ep captures, or best_cap == best_ep >= v anyway.
    *success = 1 + (best_cap > 0);
    return best_cap;
  }

  // Now handle the stalemate case.
  if (best_ep > -3 && value == 0)
  {
    // Check for stalemate in the position with ep captures.
    size_t i = 0;
    for (; i < move_count; ++i)
    {
      uo_move move = position->movelist.head[i];
      if (!uo_move_is_enpassant(move)) break;
    }

    if (i == move_count)
    {
      // Stalemate detected.
      *success = 2;
      return best_ep;
    }
  }

  // Stalemate / en passant not an issue, so value is the correct value.
  return value;
}

static int wdl_to_dtz[] = {
  -1, -101, 0, 101, 1
};

// Probe the DTZ table for a particular position.
// If *success != 0, the probe was successful.
// The return value is from the point of view of the side to move:
//         n < -100 : loss, but draw under 50-move rule
// -100 <= n < -1   : loss in n ply (assuming 50-move counter == 0)
//         0	      : draw
//     1 < n <= 100 : win in n ply (assuming 50-move counter == 0)
//   100 < n        : win, but draw under 50-move rule
//
// If the position is mate, -1 is returned instead of 0.
//
// The return value n can be off by 1: a return value -n can mean a loss
// in n+1 ply and a return value +n can mean a win in n+1 ply. This
// cannot happen for tables with positions exactly on the "edge" of
// the 50-move rule.
//
// This means that if dtz > 0 is returned, the position is certainly
// a win if dtz + 50-move-counter <= 99. Care must be taken that the engine
// picks moves that preserve dtz + 50-move-counter <= 99.
//
// If n = 100 immediately after a capture or pawn move, then the position
// is also certainly a win, and during the whole phase until the next
// capture or pawn move, the inequality to be preserved is
// dtz + 50-movecounter <= 100.
//
// In short, if a move is available resulting in dtz + 50-move-counter <= 99,
// then do not accept moves leading to dtz + 50-move-counter == 100.
//
int uo_tb_probe_dtz(uo_position *position, int *success)
{
  int wdl = uo_tb_probe_wdl(position, success);
  if (*success == 0) return 0;

  // If draw, then dtz = 0.
  if (wdl == 0) return 0;

  // Check for winning (cursed) capture or ep capture as only best move.
  if (*success == 2) return wdl_to_dtz[wdl + 2];

  // If winning, check for a winning pawn move.
  if (wdl > 0)
  {
    // Generate legal moves
    size_t move_count = position->stack->moves_generated
      ? position->stack->move_count
      : uo_position_generate_moves(position);

    for (size_t i = 0; i < move_count; ++i)
    {
      uo_move move = position->movelist.head[i];

      if (uo_move_is_capture(move)) continue;

      uo_square square_from = uo_move_square_from(move);
      uo_piece piece = position->board[square_from];
      if (piece != uo_piece__P) continue;

      uo_position_make_move(position, move);
      int value = -uo_tb_probe_wdl(position, success);
      uo_position_unmake_move(position);

      if (*success == 0) return 0;
      if (value == wdl) return wdl_to_dtz[wdl + 2];
    }
  }

  // If we are here, we know that the best move is not an ep capture.
  // In other words, the value of wdl corresponds to the WDL value of
  // the position without ep rights. It is therefore safe to probe the
  // DTZ table with the current value of wdl.

  int dtz = uo_tb_probe_dtz_table(position, wdl, success);

  if (*success >= 0)
  {
    return wdl_to_dtz[wdl + 2] + ((wdl > 0) ? dtz : -dtz);
  }

  // *success < 0 means we need to probe DTZ for the other side to move.
  int best;

  if (wdl > 0)
  {
    best = INT32_MAX;
    // If wdl > 0, we already generated all moves.
  }
  else
  {
    // If (cursed) loss, the worst case is a losing capture or pawn move
    // as the "best" move, leading to dtz of -1 or -101.
    // In case of mate, this will cause -1 to be returned.
    best = wdl_to_dtz[wdl + 2];
  }

  // Generate legal moves
  size_t move_count = position->stack->moves_generated
    ? position->stack->move_count
    : uo_position_generate_moves(position);

  for (size_t i = 0; i < move_count; ++i)
  {
    // We can skip pawn moves and captures.
    // If wdl > 0, we already caught them. If wdl < 0, the initial value
    // of best already takes account of them.

    uo_move move = position->movelist.head[i];

    if (uo_move_is_capture(move)) continue;

    uo_square square_from = uo_move_square_from(move);
    uo_piece piece = position->board[square_from];
    if (piece == uo_piece__P) continue;

    uo_position_make_move(position, move);
    int value = -uo_tb_probe_dtz(position, success);
    uo_position_unmake_move(position);

    if (*success == 0) return 0;

    if (wdl > 0)
    {
      if (value > 0 && value + 1 < best)
      {
        best = value + 1;
      }
    }
    else
    {
      if (value - 1 < best)
      {
        best = value - 1;
      }
    }
  }

  return best;
}

//// Use the DTZ tables to filter out moves that don't preserve the win or draw.
//// If the position is lost, but DTZ is fairly high, only keep moves that
//// maximise DTZ.
////
//// If *success != 0, the probe was successful.
//int uo_tb_root_probe(uo_position *position, int *success)
//{
//  int dtz = uo_tb_probe_dtz(position, success);
//
//  if (!*success) return 0;
//
//  // Generate legal moves
//  size_t move_count = position->stack->moves_generated
//    ? position->stack->move_count
//    : uo_position_generate_moves(position);
//
//  // Probe each move
//  for (size_t i = 0; i < move_count; ++i)
//  {
//    uo_move move = position->movelist.head[i];
//    uo_bitboard checks = uo_position_move_checks(position, move, NULL);
//
//    int value = 0;
//
//    //if (checks && dtz > 0)
//    //{
//    //  value = 1;
//    //}
//    //else
//    {
//      uo_position_update_next_move_checks(position, checks);
//      uo_position_make_move(position, move);
//
//      if (uo_position_flags_rule50(position->flags) != 0)
//      {
//        value = -uo_tb_probe_dtz(position, success);
//
//        if (value > 0)
//        {
//          value++;
//        }
//        else if (value < 0)
//        {
//          value--;
//        }
//      }
//      else
//      {
//        value = -uo_tb_probe_wdl(position, success);
//        value = wdl_to_dtz[value + 2];
//      }
//
//      uo_position_unmake_move(position);
//    }
//
//    if (!*success) return 0;
//
//    position->movelist.move_scores[i] = value;
//  }
//
//  // Obtain 50-move counter for the root position.
//  int rule50 = uo_position_flags_rule50(position->flags);
//
//  // Use 50-move counter to determine whether the root position is
//  // won, lost or drawn.
//  int wdl = 0;
//  if (dtz > 0)
//  {
//    wdl = (dtz + rule50 <= 100) ? 2 : 1;
//  }
//  else if (dtz < 0)
//  {
//    wdl = (-dtz + rule50 <= 100) ? -2 : -1;
//  }
//
//  // Now be a bit smart about filtering out moves.
//  if (dtz > 0)
//  { // Winning (or 50-move rule draw)
//    int best = 0xffff;
//    for (size_t i = 0; i < move_count; ++i)
//    {
//      int value = position->movelist.move_scores[i];
//      if (value > 0 && value < best)
//      {
//        best = value;
//      }
//    }
//
//    int max = best;
//
//    //// If the current phase has not seen repetitions, then try all moves
//    //// that stay safely within the 50-move budget, if there are any.
//    //if (!position->stack->repetitions && best + rule50 <= 99)
//    //{
//    //  max = 99 - rule50;
//    //}
//
//    for (size_t i = 0; i < move_count; ++i)
//    {
//      int value = position->movelist.move_scores[i];
//      if (value < 0 || value > max)
//      {
//        position->movelist.move_scores[i] = uo_move_score_skip;
//      }
//    }
//  }
//  else if (dtz < 0)
//  { // Losing
//    int best = 0;
//    for (size_t i = 0; i < move_count; ++i)
//    {
//      int value = position->movelist.move_scores[i];
//      if (value < best)
//      {
//        best = value;
//      }
//    }
//
//    // Try all moves, unless we approach or have a 50-move rule draw.
//    if (-best * 2 + rule50 < 100)
//    {
//      uo_position_quicksort_moves(position, position->movelist.head, 0, move_count - 1);
//      uo_position_sort_skipped_moves(position, position->movelist.head, 0, move_count - 1);
//      position->stack->moves_sorted = true;
//
//      return wdl;
//    }
//
//    for (size_t i = 0; i < move_count; ++i)
//    {
//      if (position->movelist.move_scores[i] != best)
//      {
//        position->movelist.move_scores[i] = uo_move_score_skip;
//      }
//    }
//  }
//  else
//  { // Drawing
//    // Try all moves that preserve the draw.
//    for (size_t i = 0; i < move_count; ++i)
//    {
//      if (position->movelist.move_scores[i] != 0)
//      {
//        position->movelist.move_scores[i] = uo_move_score_skip;
//      }
//    }
//  }
//
//  uo_position_quicksort_moves(position, position->movelist.head, 0, move_count - 1);
//  uo_position_sort_skipped_moves(position, position->movelist.head, 0, move_count - 1);
//  position->stack->moves_sorted = true;
//
//  return wdl;
//}

// Use the DTZ tables to filter out moves that don't preserve the win or draw.
// If the position is lost, but DTZ is fairly high, only keep moves that
// maximise DTZ.
//
// If *success != 0, the probe was successful.
int uo_tb_root_probe_dtz(uo_position *position, int *success)
{
  int dtz = uo_tb_probe_dtz(position, success);
  if (!*success) return 0;

  // Generate legal moves
  size_t move_count = position->stack->moves_generated
    ? position->stack->move_count
    : uo_position_generate_moves(position);

  int16_t *move_scores = position->movelist.move_scores;
  uo_move bestmove = 0;

  // If drawing, let's filter out losing moves
  if (dtz == 0)
  {
    for (size_t i = 0; i < move_count; ++i)
    {
      uo_move move = position->movelist.head[i];

      uo_position_make_move(position, move);
      int wdl_move = -uo_tb_probe_wdl(position, success);
      uo_position_unmake_move(position);

      if (!*success) return 0;
      move_scores[i] = wdl_move < 0 ? uo_move_score_skip : wdl_move;
    }
  }

  // If winning let's filter out all moves which do not preserve the win 
  else if (dtz > 0)
  {
    for (size_t i = 0; i < move_count; ++i)
    {
      uo_move move = position->movelist.head[i];

      bool is_pawn_move = uo_position_move_piece(position, move) == uo_piece__P;
      bool is_capture = uo_move_is_capture(move);

      uo_position_make_move(position, move);
      int dtz_move = -uo_tb_probe_dtz(position, success);
      uo_position_unmake_move(position);

      if (!*success) return 0;

      if (dtz_move < 0)
      {
        // Filter out losing moves
        move_scores[i] = uo_move_score_skip;
        continue;
      }

      if (dtz_move > dtz
        && !is_pawn_move
        && !is_capture)
      {
        // Filter out moves which seem not to make progress
        move_scores[i] = uo_move_score_skip;
        continue;
      }

      if (dtz_move == 0)
      {
        if (!uo_position_is_check(position))
        {
          move_scores[i] = uo_move_score_skip;
          continue;
        }

        if (uo_position_generate_moves(position) > 0)
        {
          move_scores[i] = uo_move_score_skip;
          continue;
        }

        // Move is checkmate. No need to look for other moves.
        move_scores[i] = 2000;

        ++i;
        for (; i < move_count; ++i)
        {
          move_scores[i] = uo_move_score_skip;
        }

        break;
      }

      // Move is making progress. Let's score it.
      move_scores[i] = is_capture * 1000 + is_pawn_move * 100 - dtz_move;
    }
  }

  // If losing, let's do nothing
  else // dtz < 0
  {
    // If winning let's filter out all moves which do not preserve the win 
  }

  uo_position_quicksort_moves(position, position->movelist.head, 0, move_count - 1);
  uo_position_sort_skipped_moves(position, position->movelist.head, 0, move_count - 1);
  position->stack->moves_sorted = true;

  return dtz;
}


bool uo_tb_init(uo_tb *tb)
{
  tb->score_wdl_draw = tb->rule50;
  uo_tb_init_material_keys();
  init_tablebases(tb->dir);
  return true;
}
