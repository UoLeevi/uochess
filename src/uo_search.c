#include "uo_search.h"
#include "uo_position.h"
#include "uo_evaluation.h"
#include "uo_thread.h"
#include "uo_engine.h"
#include "uo_misc.h"
#include "uo_global.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <assert.h>

const size_t quicksort_depth = 4; // arbitrary

typedef struct uo_search_info
{
  uo_engine_thread *thread;
  uint64_t nodes;
  uo_time time_start;
  uint16_t multipv;
  uint8_t depth;
  uint8_t seldepth;
  uint8_t tbhits;
  bool completed;
  uo_search_params *params;
} uo_search_info;

static inline void uo_search_stop_if_movetime_over(uo_search_info *info)
{
  uint64_t movetime = info->params->movetime;

  // TODO: make better decisions about how mutch time to use
  if (!movetime && (info->params->btime + info->params->wtime))
  {
    movetime = 3000;
  }

  if (movetime)
  {
    const double margin_msec = 1.0;
    double time_msec = uo_time_elapsed_msec(&info->time_start);

    if (time_msec + margin_msec > (double)movetime)
    {
      uo_engine_stop_search();
    }
  }
}

static int16_t uo_search_quiesce(uo_engine_thread *thread, int16_t a, int16_t b, int64_t move_count)
{
  int16_t stand_pat = uo_position_evaluate(&thread->position);

  if (stand_pat >= b)
  {
    return b;
  }

  if (a < stand_pat)
  {
    a = stand_pat;
  }

  thread->position.movelist.head += move_count;

  for (int64_t i = 0; i < move_count; ++i)
  {
    uo_move move = thread->position.movelist.head[i - move_count];

    if (uo_move_is_capture(move) || uo_move_is_promotion(move))
    {
      uo_position_make_move(&thread->position, move);
      int64_t next_move_count = uo_position_generate_moves(&thread->position);

      if (next_move_count == 0)
      {
        thread->position.movelist.head -= move_count;
        uo_position_unmake_move(&thread->position);
        return uo_position_is_check(&thread->position) ? -UO_SCORE_CHECKMATE : 0;
      }

      int16_t node_value = -uo_search_quiesce(thread, -b, -a, next_move_count);
      uo_position_unmake_move(&thread->position);

      if (node_value > UO_SCORE_MATE_IN_THRESHOLD)
      {
        --node_value;
      }
      else if (node_value < -UO_SCORE_MATE_IN_THRESHOLD)
      {
        ++node_value;
      }

      if (node_value >= b)
      {
        thread->position.movelist.head -= move_count;
        return b;
      }

      if (node_value > a)
      {
        a = node_value;
      }
    }
  }

  thread->position.movelist.head -= move_count;

  return a;
}

static int uo_search_moves_cmp(uo_engine_thread *thread, uo_move move_lhs, uo_move move_rhs)
{
  uo_square square_from_lhs = uo_move_square_from(move_lhs);
  uo_square square_to_lhs = uo_move_square_to(move_lhs);
  uo_square move_type_lhs = uo_move_get_type(move_lhs);
  int score_lhs = 0;

  if (move_type_lhs & uo_move_type__x)
  {
    uo_piece piece_lhs = thread->position.board[square_from_lhs];
    uo_piece piece_captured_lhs = thread->position.board[square_to_lhs];
    score_lhs = uo_piece_type(piece_captured_lhs) - uo_piece_type(piece_lhs) + uo_piece__B;
  }

  if (uo_bitboard_attacks_enemy_P(thread->position.P & thread->position.enemy) & uo_square_bitboard(square_to_lhs))
  {
    score_lhs -= 1;
  }
  else if (uo_position_is_check_move(&thread->position, move_lhs))
  {
    score_lhs += 2;
  }

  uo_square square_from_rhs = uo_move_square_from(move_rhs);
  uo_square square_to_rhs = uo_move_square_to(move_rhs);
  uo_square move_type_rhs = uo_move_get_type(move_rhs);
  int score_rhs = 0;

  if (move_type_rhs & uo_move_type__x)
  {
    uo_piece piece_rhs = thread->position.board[square_from_rhs];
    uo_piece piece_captured_rhs = thread->position.board[square_to_rhs];
    score_rhs = uo_piece_type(piece_captured_rhs) - uo_piece_type(piece_rhs) + uo_piece__B;
  }

  if (uo_bitboard_attacks_enemy_P(thread->position.P & thread->position.enemy) & uo_square_bitboard(square_to_rhs))
  {
    score_rhs -= 1;
  }
  else if (uo_position_is_check_move(&thread->position, move_rhs))
  {
    score_rhs += 2;
  }

  return score_lhs == score_rhs
    ? 0
    : score_lhs > score_rhs
    ? -1
    : 1;
}

static int uo_search_partition_moves(uo_engine_thread *thread, uo_move *movelist, int lo, int hi)
{
  int mid = (lo + hi) >> 1;
  uo_move temp;

  if (uo_search_moves_cmp(thread, movelist[mid], movelist[lo]) < 0)
  {
    temp = movelist[mid];
    movelist[mid] = movelist[lo];
    movelist[lo] = temp;
  }

  if (uo_search_moves_cmp(thread, movelist[hi], movelist[lo]) < 0)
  {
    temp = movelist[hi];
    movelist[hi] = movelist[lo];
    movelist[lo] = temp;
  }

  if (uo_search_moves_cmp(thread, movelist[mid], movelist[hi]) < 0)
  {
    temp = movelist[mid];
    movelist[mid] = movelist[hi];
    movelist[hi] = temp;
  }

  uo_move pivot = movelist[hi];

  for (int j = lo; j < hi - 1; ++j)
  {
    if (uo_search_moves_cmp(thread, movelist[j], pivot) < 0)
    {

      temp = movelist[lo];
      movelist[lo] = movelist[j];
      movelist[j] = temp;
      ++lo;
    }
  }

  temp = movelist[lo];
  movelist[lo] = movelist[hi];
  movelist[hi] = temp;
  return lo;
}

// Limited quicksort which prioritizes ordering beginning of movelist
static void uo_search_quicksort_moves(uo_engine_thread *thread, uo_move *movelist, int lo, int hi, int depth)
{
  if (lo >= hi || depth <= 0)
  {
    return;
  }

  int p = uo_search_partition_moves(thread, movelist, lo, hi);
  uo_search_quicksort_moves(thread, movelist, lo, p - 1, depth - 1);
  uo_search_quicksort_moves(thread, movelist, p + 1, hi, depth - 2);
}

static void uo_search_sort_moves(uo_engine_thread *thread, int64_t move_count)
{
  uo_engine_lock_ttable();
  uo_tentry *entry = uo_ttable_get(&engine.ttable, thread->position.key);
  uo_move bestmove = entry ? entry->bestmove : 0;
  uo_engine_unlock_ttable();
  uo_move *movelist = thread->position.movelist.head;

  int lo = 0;

  // If found, set pv move as first
  if (bestmove)
  {
    for (int i = 1; i < move_count; ++i)
    {
      if (movelist[i] == bestmove)
      {
        movelist[i] = movelist[0];
        movelist[0] = bestmove;
        lo = 1;
        break;
      }
    }
  }

  uo_search_quicksort_moves(thread, movelist, lo, move_count - 1, quicksort_depth);
}

// see: https://en.wikipedia.org/wiki/Negamax#Negamax_with_alpha_beta_pruning_and_transposition_tables
static int16_t uo_search_negamax(uo_engine_thread *thread, size_t depth, int16_t a, int16_t b, int16_t color, uo_search_info *info)
{
  int16_t a_orig = a;

  uo_engine_lock_ttable();
  uo_tentry *entry = uo_ttable_get(&engine.ttable, thread->position.key);
  if (entry && entry->depth >= depth)
  {
    int16_t score = color * entry->score;

    switch (entry->type)
    {
      case uo_tentry_type__exact:
        uo_engine_unlock_ttable();
        return score;

      case uo_tentry_type__lower_bound:
        a = score > a ? score : a;
        break;

      case uo_tentry_type__upper_bound:
        b = score < b ? score : b;
        break;

      default:
        assert(false);
    }

    if (a >= b)
    {
      uo_engine_unlock_ttable();
      return score;
    }
  }

  uo_engine_unlock_ttable();

  int64_t move_count = uo_position_generate_moves(&thread->position);

  if (move_count == 0)
  {
    return uo_position_is_check(&thread->position) ? -UO_SCORE_CHECKMATE : 0;
  }

  if (depth == 0)
  {
    return uo_search_quiesce(thread, a, b, move_count);
  }

  uo_search_stop_if_movetime_over(info);

  uo_search_sort_moves(thread, move_count);
  thread->position.movelist.head += move_count;

  uo_move bestmove;
  int16_t value = -UO_SCORE_CHECKMATE;

  for (int64_t i = 0; i < move_count; ++i)
  {
    uo_move move = thread->position.movelist.head[i - move_count];
    uo_position_make_move(&thread->position, move);
    int16_t node_value = -uo_search_negamax(thread, depth - 1, -b, -a, -color, info);
    uo_position_unmake_move(&thread->position);

    if (uo_engine_is_stopped())
    {
      thread->position.movelist.head -= move_count;
      return value;
    }

    if (node_value > UO_SCORE_MATE_IN_THRESHOLD)
    {
      --node_value;
    }
    else if (node_value < -UO_SCORE_MATE_IN_THRESHOLD)
    {
      ++node_value;
    }

    if (node_value > value)
    {
      value = node_value;
      bestmove = move;

      if (value > a)
      {
        a = value;
      }
    }

    if (a >= b)
    {
      break;
    }
  }

  thread->position.movelist.head -= move_count;

  uo_engine_lock_ttable();

  if (!entry)
  {
    ++info->nodes;
    entry = uo_ttable_set(&engine.ttable, thread->position.key);
  }

  entry->score = color * value;
  entry->depth = depth;
  entry->bestmove = bestmove;

  if (value > a_orig)
  {
    entry->type = uo_tentry_type__upper_bound;
  }
  else if (value >= b)
  {
    entry->type = uo_tentry_type__lower_bound;
  }
  else
  {
    entry->type = uo_tentry_type__exact;
  }

  uo_engine_unlock_ttable();

  return value;
}

static void uo_search_info_print(uo_search_info *info)
{
  uo_engine_thread *thread = info->thread;
  uo_engine_lock_stdout(engine);

  double time_msec = uo_time_elapsed_msec(&info->time_start);
  uint64_t nps = info->nodes / time_msec * 1000.0;

  for (int i = 0; i < info->multipv; ++i)
  {
    uo_position_update_checks_and_pins(&info->thread->position);

    printf("info depth %d ", info->depth);
    if (info->seldepth) printf("seldepth %d ", info->seldepth);
    printf("multipv %d ", info->multipv);

    uo_engine_lock_ttable();
    uo_tentry *next_entry = uo_ttable_get(&engine.ttable, thread->position.key);
    uo_engine_unlock_ttable();

    assert(next_entry);

    uo_tentry pv_entry = *next_entry;

    if (pv_entry.score > UO_SCORE_MATE_IN_THRESHOLD)
    {
      printf("score mate %d ", (UO_SCORE_CHECKMATE - pv_entry.score + 1) >> 1);
    }
    else if (pv_entry.score < -UO_SCORE_MATE_IN_THRESHOLD)
    {
      printf("score mate %d ", (UO_SCORE_CHECKMATE + pv_entry.score + 1) >> 1);
    }
    else
    {
      printf("score cp %d ", pv_entry.score);
    }

    printf("nodes %" PRIu64 " nps %" PRIu64 " tbhits %d time %.0f pv",
      info->nodes, nps, info->tbhits, time_msec);

    size_t pv_len = 1;

    size_t i = 0;

    for (; i < info->depth && next_entry; ++i)
    {
      uo_position_print_move(&thread->position, pv_entry.bestmove, buf);
      printf(" %s", buf);

      if (!uo_position_is_legal_move(&thread->position, pv_entry.bestmove))
      {
        uo_position_print_move(&thread->position, pv_entry.bestmove, buf);
        printf("\n\nillegal bestmove %s\n", buf);
        uo_position_print_diagram(&thread->position, buf);
        printf("\n%s", buf);
        uo_position_print_fen(&thread->position, buf);
        printf("\n");
        printf("Fen: %s\n", buf);
        printf("Key: %" PRIu64 "\n", thread->position.key);
        assert(false);
      }

      uo_position_make_move(&thread->position, pv_entry.bestmove);

      uo_engine_lock_ttable();
      next_entry = uo_ttable_get(&engine.ttable, thread->position.key);

      if (next_entry)
      {
        pv_entry = *next_entry;
      }

      uo_engine_unlock_ttable();
    }

    while (i--)
    {
      uo_position_unmake_move(&thread->position);
    }

    printf("\n");

    if (info->completed && info->multipv == 1)
    {
      uo_engine_lock_ttable();
      uo_tentry pv_entry = *uo_ttable_get(&engine.ttable, thread->position.key);
      uo_engine_unlock_ttable();
      uo_position_print_move(&thread->position, pv_entry.bestmove, buf);
      printf("bestmove %s\n", buf);
    }
  }

  uo_engine_unlock_stdout(engine);
  uo_position_update_checks_and_pins(&thread->position);
}

void *uo_engine_thread_run_negamax_search(void *arg)
{
  uo_engine_thread_work *work = arg;
  uo_engine_thread *thread = work->thread;
  uo_search_params *params = work->data;

  uo_search_info info = {
    .thread = thread,
    .depth = 1,
    .multipv = engine_options.multipv,
    .nodes = 0,
    .params = params
  };

  uo_time_now(&info.time_start);

  uo_engine_thread_load_position(thread);
  int16_t color = uo_position_flags_color_to_move(thread->position.flags) ? -1 : 1;

  uo_search_negamax(thread, 1, -UO_SCORE_CHECKMATE, UO_SCORE_CHECKMATE, color, &info);

  uo_engine_lock_ttable();
  uo_tentry *pv_entry = uo_ttable_get(&engine.ttable, thread->position.key);
  ++pv_entry->refcount;
  uo_engine_unlock_ttable();

  for (size_t depth = 2; depth <= params->depth; ++depth)
  {
    if (info.nodes)
    {
      uo_search_info_print(&info);
    }

    info.nodes = 0;
    uo_search_negamax(thread, depth, -UO_SCORE_CHECKMATE, UO_SCORE_CHECKMATE, color, &info);

    uo_search_stop_if_movetime_over(&info);
    if (uo_engine_is_stopped()) break;

    info.depth = depth;
  }

  info.completed = true;
  uo_search_info_print(&info);

  uo_engine_lock_ttable();
  --pv_entry->refcount;
  uo_engine_unlock_ttable();

  uo_engine_stop_search();
  info.params = NULL;
  free(params);
  return NULL;
}
