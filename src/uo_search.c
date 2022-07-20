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
  uo_move bestmove;
  int16_t value;
} uo_search_info;

static int uo_search_partition_moves(uo_engine_thread *thread, uo_move *movelist, int lo, int hi)
{
  int16_t *move_scores = thread->position.movelist.move_scores;
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

inline static int8_t uo_search_calculate_move_score(uo_engine_thread *thread, uo_move move, uo_search_info *info)
{
  uo_position *position = &thread->position;
  uo_square square_from = uo_move_square_from(move);
  uo_square square_to = uo_move_square_to(move);
  uo_square move_type = uo_move_get_type(move);
  uo_piece piece = position->board[square_from];

  int8_t score = 0;

  if (uo_bitboard_attacks_P(square_from, uo_color_own) & (position->P & position->enemy))
  {
    score += 4;
  }

  if (move_type == uo_move_type__enpassant)
  {
    score += 1;
  }
  else if (move_type & uo_move_type__promo)
  {
    score += 1;
  }
  else if (move_type & uo_move_type__x)
  {
    score += uo_position_capture_gain(&thread->position, move) / 8;
  }
  else if (move_type & uo_move_type__OO)
  {
    score += 2;
  }
  else if (piece == uo_piece__K)
  {
    if (!uo_position_is_check(&thread->position))
    {
      if (position->Q & position->enemy)
      {
        score -= 1;
      }
    }
  }
  else if (square_from < 8 && square_to >= 8)
  {
    score += 1;
  }

  if (uo_bitboard_attacks_enemy_P(position->P & position->enemy) & uo_square_bitboard(square_to))
  {
    score -= 1;
  }

  return score;
}

static size_t uo_search_sort_and_prune_moves(uo_engine_thread *thread, uo_search_info *info)
{
  uo_move bestmove = info->bestmove;
  uo_position *position = &thread->position;
  uo_move *movelist = position->movelist.head;
  size_t move_count = position->stack->move_count;

  int lo = 0;

  // If found, set pv move as first
  if (bestmove)
  {
    for (size_t i = 1; i < move_count; ++i)
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

  // If there are only few moves, let's search them all
  if (move_count < 10) return move_count;

  // If it is our turn, then let's be more heavy handed with pruning moves
  int8_t pruning_threshold = (uo_color(position->ply) == uo_color_own ? -100 : -200) + position->ply;

  if (pruning_threshold < 0) pruning_threshold = 0;

  size_t j = move_count;
  size_t i = lo;

  do
  {
    uo_move move = movelist[i];
    int8_t score = uo_search_calculate_move_score(thread, move, info);

    if (score >= pruning_threshold)
    {
      position->movelist.move_scores[i] = score;
      ++i;
    }
    else
    {
      --j;
      position->movelist.move_scores[j] = score;
      movelist[i] = movelist[j];
      movelist[j] = move;
    }
  } while (i < j);

  while (j < 10)
  {
    i = j;
    j = move_count;
    pruning_threshold += 2;

    do
    {
      uo_move move = movelist[i];
      int8_t score = position->movelist.move_scores[i];

      if (score >= pruning_threshold)
      {
        ++i;
      }
      else
      {
        --j;
        position->movelist.move_scores[j] = score;
        movelist[i] = movelist[j];
        movelist[j] = move;
      }
    } while (i < j);
  }

  const size_t quicksort_depth = 4; // arbitrary
  uo_search_quicksort_moves(thread, movelist, lo, j - 1, quicksort_depth);

  return j;
}

static inline void uo_search_stop_if_movetime_over(uo_engine_thread *thread, uo_search_info *info)
{
  uo_search_params *params = info->params;
  uint64_t movetime = params->movetime;

  // TODO: make better decisions about how much time to use
  if (!movetime && params->time_own)
  {
    movetime = 3000 + (params->time_own - params->time_enemy) / 2;

    if (movetime > params->time_own / 8)
    {
      movetime = params->time_own / 8 + 1;
    }
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

static int16_t uo_search_quiesce(uo_engine_thread *thread, int16_t alpha, int16_t beta, uo_search_info *info)
{
  uo_search_stop_if_movetime_over(thread, info);

  if (uo_engine_is_stopped())
  {
    return 0;
  }

  uo_position *position = &thread->position;
  size_t move_count = position->stack->move_count;

  if (info->seldepth < position->ply)
  {
    info->seldepth = position->ply;
  }

  if (uo_position_is_check(&thread->position))
  {
    int16_t value = -UO_SCORE_CHECKMATE;

    for (int64_t i = 0; i < move_count; ++i)
    {
      assert(position->update_status.moves_generated);
      uo_move move = position->movelist.head[i];

      uo_position_make_move(position, move);

      if (uo_position_is_rule50_draw(position) || uo_position_is_repetition_draw(position))
      {
        uo_position_unmake_move(position);
        return 0;
      }

      if (uo_position_is_max_depth_reached(position))
      {
        uo_position_unmake_move(position);
        return uo_position_evaluate(position);
      }

      size_t next_move_count = uo_position_generate_moves(position);

      if (next_move_count == 0)
      {
        bool mate = uo_position_is_check(position);
        uo_position_unmake_move(position);
        return mate ? -UO_SCORE_CHECKMATE : 0;
      }

      int16_t node_value = -uo_search_quiesce(thread, -beta, -alpha, info);
      node_value = uo_score_adjust_for_mate(node_value);

      uo_position_unmake_move(position);

      if (node_value > value)
      {
        value = node_value;

        if (value > alpha)
        {
          alpha = value;
        }

        if (value >= beta)
        {
          break;
        }
      }
    }

    return value;
  }

  int16_t value = uo_position_evaluate(position);

  if (value >= beta)
  {
    return value;
  }

  if (value > alpha)
  {
    alpha = value;
  }

  for (int64_t i = 0; i < move_count; ++i)
  {
    assert(position->update_status.moves_generated);
    uo_move move = position->movelist.head[i];

    bool critical_move = uo_move_is_promotion(move)
      || uo_position_move_threatens_material(position, move)
      || (uo_move_is_capture(move) && uo_position_capture_gain(position, move) >= 0);

    if (!critical_move)
    {
      continue;
    }

    uo_position_make_move(position, move);

    if (uo_position_is_rule50_draw(position) || uo_position_is_repetition_draw(position))
    {
      uo_position_unmake_move(position);
      return 0;
    }

    if (uo_position_is_max_depth_reached(position))
    {
      uo_position_unmake_move(position);
      return uo_position_evaluate(position);
    }

    int64_t next_move_count = uo_position_generate_moves(position);

    if (next_move_count == 0)
    {
      bool mate = uo_position_is_check(position);
      uo_position_unmake_move(position);
      return mate ? -UO_SCORE_CHECKMATE : 0;
    }

    int16_t node_value = -uo_search_quiesce(thread, -beta, -alpha, info);
    node_value = uo_score_adjust_for_mate(node_value);

    uo_position_unmake_move(position);

    if (node_value > value)
    {
      value = node_value;

      if (value > alpha)
      {
        alpha = value;
      }

      if (value >= beta)
      {
        break;
      }
    }
  }

  return value;
}

// see: https://en.wikipedia.org/wiki/Negamax#Negamax_with_alpha_beta_pruning_and_transposition_tables
static int16_t uo_search_negamax(uo_engine_thread *thread, size_t depth, int16_t alpha, int16_t beta, uo_search_info *info)
{
  uo_position *position = &thread->position;
  int16_t alpha_original = alpha;

  uo_engine_lock_ttable();
  uo_tentry *entry = uo_ttable_get(&engine.ttable, thread->position.key);

  if (entry && entry->depth >= depth)
  {
    int16_t value = entry->value;
    uo_engine_unlock_ttable();

    switch (entry->type)
    {
      case uo_tentry_type__exact:
        return value;

      case uo_tentry_type__lower_bound:
        if (value > alpha)
        {
          alpha = value;
        }

        break;

      case uo_tentry_type__upper_bound:
        if (value < beta)
        {
          beta = value;
        }

        break;

      default:
        assert(false);
    }

    if (alpha >= beta)
    {
      return value;
    }
  }
  else
  {
    uo_engine_unlock_ttable();
  }

  if (uo_position_is_rule50_draw(position) || uo_position_is_repetition_draw(position))
  {
    return 0;
  }

  if (uo_position_is_max_depth_reached(position))
  {
    return uo_position_evaluate(position);
  }

  size_t move_count = uo_position_generate_moves(position);

  if (move_count == 0)
  {
    return uo_position_is_check(position) ? -UO_SCORE_CHECKMATE : 0;
  }

  // Null move pruning
  if (depth > 3 && uo_position_is_null_move_allowed(position))
  {
    uo_position_make_null_move(position);
    int16_t pass_value = -uo_search_negamax(thread, depth - 2, -beta, -alpha, info);
    uo_position_unmake_null_move(position);

    if (pass_value >= beta)
    {
      return pass_value;
    }
  }

  size_t reduced_move_count = uo_search_sort_and_prune_moves(thread, info);

  if (depth == 0)
  {
    return uo_search_quiesce(thread, alpha, beta, info);
  }

  uo_search_stop_if_movetime_over(thread, info);

  uo_move bestmove;
  int16_t value = -UO_SCORE_CHECKMATE;

  for (size_t i = 0; i < reduced_move_count; ++i)
  {
    uo_move move = thread->position.movelist.head[i];
    uo_position_make_move(position, move);
    int16_t node_value = -uo_search_negamax(thread, depth - 1, -beta, -alpha, info);
    node_value = uo_score_adjust_for_mate(node_value);
    uo_position_unmake_move(position);

    if (node_value > value)
    {
      value = node_value;
      bestmove = move;

      if (value > alpha)
      {
        alpha = value;
      }

      if (value >= beta)
      {
        break;
      }
    }

    if (uo_engine_is_stopped())
    {
      return value;
    }
  }

  if (alpha < -50)
  {
    for (size_t i = reduced_move_count; i < move_count; ++i)
    {
      uo_move move = thread->position.movelist.head[i];
      uo_position_make_move(position, move);
      int16_t node_value = -uo_search_negamax(thread, depth - 1, -beta, -alpha, info);
      node_value = uo_score_adjust_for_mate(node_value);
      uo_position_unmake_move(position);

      if (node_value > value)
      {
        value = node_value;
        bestmove = move;

        if (value > alpha)
        {
          alpha = value;
        }

        if (value >= beta)
        {
          break;
        }
      }

      if (uo_engine_is_stopped())
      {
        return value;
      }
    }
  }

  if (uo_engine_is_stopped())
  {
    return value;
  }

  uo_engine_lock_ttable();

  if (!entry)
  {
    ++info->nodes;
    entry = uo_ttable_set(&engine.ttable, position->key);
  }

  entry->depth = depth;
  entry->bestmove = bestmove;
  entry->value = value;

  if (alpha >= beta)
  {
    entry->type = uo_tentry_type__lower_bound;
  }
  else if (alpha != alpha_original)
  {
    entry->type = uo_tentry_type__upper_bound;
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

  double time_msec = uo_time_elapsed_msec(&info->time_start);
  uint64_t nps = info->nodes / time_msec * 1000.0;

  uo_engine_lock_stdout();

  for (int i = 0; i < info->multipv; ++i)
  {
    printf("info depth %d ", info->depth);
    if (info->seldepth) printf("seldepth %d ", info->seldepth);
    printf("multipv %d ", info->multipv);

    int16_t score = info->value;

    if (score > UO_SCORE_MATE_IN_THRESHOLD)
    {
      printf("score mate %d ", (UO_SCORE_CHECKMATE - score + 1) >> 1);
    }
    else if (score < -UO_SCORE_MATE_IN_THRESHOLD)
    {
      printf("score mate %d ", (UO_SCORE_CHECKMATE + score + 1) >> 1);
    }
    else
    {
      printf("score cp %d ", score);
    }

    uo_position_print_move(&thread->position, info->bestmove, buf);
    printf("nodes %" PRIu64 " nps %" PRIu64 " tbhits %d time %.0f pv %s",
      info->nodes, nps, info->tbhits, time_msec, buf);

    size_t pv_len = 1;

    size_t i = 1;

    uo_position_make_move(&thread->position, info->bestmove);

    uo_engine_lock_ttable();
    uo_tentry *next_entry = uo_ttable_get(&engine.ttable, thread->position.key);
    uo_tentry pv_entry;
    if (next_entry) pv_entry = *next_entry;
    uo_engine_unlock_ttable();

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
        fflush(stdout);
        assert(false);
      }

      uo_position_make_move(&thread->position, pv_entry.bestmove);

      uo_engine_lock_ttable();
      next_entry = uo_ttable_get(&engine.ttable, thread->position.key);
      if (next_entry) pv_entry = *next_entry;
      uo_engine_unlock_ttable();
    }

    while (i--)
    {
      uo_position_unmake_move(&thread->position);
    }

    printf("\n");

    if (info->completed && info->multipv == 1)
    {
      uo_position_print_move(&thread->position, info->bestmove, buf);
      printf("bestmove %s\n", buf);
    }
  }

  uo_engine_unlock_stdout();
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
  uo_search_negamax(thread, 1, -UO_SCORE_CHECKMATE, UO_SCORE_CHECKMATE, &info);

  uo_engine_lock_ttable();
  uo_tentry *pv_entry = uo_ttable_get(&engine.ttable, thread->position.key);
  info.bestmove = pv_entry->bestmove;
  info.value = pv_entry->value;
  ++pv_entry->refcount;
  uo_engine_unlock_ttable();

  for (size_t depth = 2; depth <= params->depth; ++depth)
  {
    if (info.nodes)
    {
      uo_search_info_print(&info);
    }

    info.nodes = 0;
    uo_search_negamax(thread, depth, -UO_SCORE_CHECKMATE, UO_SCORE_CHECKMATE, &info);

    uo_search_stop_if_movetime_over(thread, &info);
    if (uo_engine_is_stopped()) break;

    info.depth = depth;

    uo_engine_lock_ttable();
    info.bestmove = pv_entry->bestmove;
    info.value = pv_entry->value;
    uo_engine_unlock_ttable();
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
