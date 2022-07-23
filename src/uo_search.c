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

inline static uint16_t uo_search_calculate_move_score(uo_engine_thread *thread, uo_move move)
{
  uo_position *position = &thread->position;
  uo_square square_from = uo_move_square_from(move);
  uo_square square_to = uo_move_square_to(move);
  uo_square move_type = uo_move_get_type(move);
  uo_piece piece = position->board[square_from];

  if (move_type == uo_move_type__enpassant)
  {
    return 3000;
  }
  else if (move_type & uo_move_type__promo)
  {
    return 3000;
  }
  else if (move_type & uo_move_type__x)
  {
    uo_piece piece_captured = position->board[square_to];
    return 2000 + up_piece_value(piece_captured) - up_piece_value(piece) / 10;
  }
  else
  {
    if (uo_position_is_killer_move(position, move))
    {
      return 1000;
    }

    // relative history heuristic
    uint8_t color = uo_color(position->flags);
    uint16_t index = move & 0xFFF;
    uint32_t hhscore = position->hhtable[color][index] << 4;
    uint32_t bfscore = position->bftable[color][index] + 1;
    return hhscore / bfscore;
  }
}

static size_t uo_search_sort_and_prune_moves(uo_engine_thread *thread, uo_move bestmove)
{
  uo_position *position = &thread->position;
  uo_move *movelist = position->movelist.head;
  size_t move_count = position->stack->move_count;

  int lo = bestmove ? 1 : 0;

  // set bestmove as first
  if (bestmove && movelist[0] != bestmove)
  {
    for (size_t i = 1; i < move_count; ++i)
    {
      if (movelist[i] == bestmove)
      {
        movelist[i] = movelist[0];
        movelist[0] = bestmove;
        break;
      }
    }
  }

  size_t j = move_count;
  size_t i = lo;

  while (i < move_count)
  {
    uo_move move = movelist[i];
    uint16_t score = uo_search_calculate_move_score(thread, move);
    position->movelist.move_scores[i] = score;
    ++i;
  }

  const size_t quicksort_depth = 4; // arbitrary
  uo_search_quicksort_moves(thread, movelist, lo, j - 1, quicksort_depth);

  return j;
}

static inline void uo_search_stop_if_movetime_over(uo_engine_thread *thread)
{
  uo_search_info *info = &thread->info;
  uo_search_params *params = &engine.search_params;
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
    double time_msec = uo_time_elapsed_msec(&thread->info.time_start);

    if (time_msec + margin_msec > (double)movetime)
    {
      uo_engine_stop_search();
    }
  }
}

static int16_t uo_search_quiesce(uo_engine_thread *thread, int16_t alpha, int16_t beta)
{
  uo_search_info *info = &thread->info;
  uo_position *position = &thread->position;

  if (info->seldepth < position->ply)
  {
    info->seldepth = position->ply;
  }

  if (uo_position_is_rule50_draw(position) || uo_position_is_repetition_draw(position))
  {
    return 0;
  }

  ++info->nodes;

  uo_engine_unlock_ttable();

  if (uo_position_is_max_depth_reached(position))
  {
    return uo_position_evaluate(position);
  }

  size_t move_count = uo_position_generate_moves(position);

  if (move_count == 0)
  {
    return uo_position_is_check(position) ? -UO_SCORE_CHECKMATE : 0;
  }

  bool is_check = uo_position_is_check(position);
  bool is_quiescent = !is_check;
  int16_t static_evaluation;
  uo_move bestmove = 0;
  int16_t value = -UO_SCORE_CHECKMATE;

  if (!is_check)
  {
    static_evaluation = uo_position_evaluate(position);
    const int16_t stand_pat_margin = -1;
    value = static_evaluation + stand_pat_margin;

    if (value >= beta)
    {
      return value;
    }

    if (value > alpha)
    {
      alpha = value;
    }
  }

  for (size_t i = 0; i < move_count; ++i)
  {
    uo_move move = position->movelist.head[i];

    if (is_check || uo_move_is_promotion(move) || (uo_move_is_capture(move) && uo_position_capture_gain(position, move) > 0))
    {
      is_quiescent = false;

      uo_position_make_move(position, move);
      int16_t node_value = -uo_search_quiesce(thread, -beta, -alpha);
      node_value = uo_score_adjust_for_mate(node_value);
      uo_position_unmake_move(position);

      if (node_value > value)
      {
        value = node_value;
        bestmove = move;

        if (value > alpha)
        {
          if (value >= beta)
          {
            return value;
          }

          alpha = value;
        }
      }

      if (value == alpha && uo_engine_is_stopped())
      {
        return value;
      }
    }
  }

  if (is_quiescent)
  {
    return static_evaluation;
  }

  return value;
}

static int16_t uo_search_zero_window(uo_engine_thread *thread, size_t depth, int16_t beta)
{
  uo_search_info *info = &thread->info;
  uo_position *position = &thread->position;
  int16_t alpha = beta - 1;

  if (uo_position_is_rule50_draw(position) || uo_position_is_repetition_draw(position))
  {
    return 0;
  }

  uo_engine_lock_ttable();
  uo_tentry *entry = uo_ttable_get(&engine.ttable, position);

  if (entry && entry->depth >= depth)
  {
    int16_t value = entry->value;

    //if (entry->type == uo_tentry_type__exact)
    //{
    //  uo_engine_unlock_ttable();
    //  return value;
    //}

    if (entry->type == uo_tentry_type__lower_bound && value > alpha)
    {
      uo_engine_unlock_ttable();
      return beta;
    }
  }
  else
  {
    ++info->nodes;
  }

  uo_engine_unlock_ttable();

  if (uo_position_is_max_depth_reached(position))
  {
    return uo_position_evaluate(position);
  }

  size_t move_count = uo_position_generate_moves(position);

  if (move_count == 0)
  {
    return uo_position_is_check(position) ? -UO_SCORE_CHECKMATE : 0;
  }

  uo_search_sort_and_prune_moves(thread, entry ? entry->bestmove : 0);

  if (depth == 0)
  {
    // Search depth reached. Let's perform quiescence search.

    bool is_check = uo_position_is_check(position);
    bool is_quiescent = !is_check;
    int16_t static_evaluation;
    int16_t value = -UO_SCORE_CHECKMATE;

    if (!is_check)
    {
      static_evaluation = uo_position_evaluate(position);
      const int16_t stand_pat_margin = -1;
      value = static_evaluation + stand_pat_margin;

      if (value >= beta)
      {
        uo_engine_ttable_store(entry, position, 0, beta, 0, uo_tentry_type__lower_bound);
        return beta;
      }

      if (value > alpha)
      {
        alpha = value;
      }
    }

    for (size_t i = 0; i < move_count; ++i)
    {
      uo_move move = position->movelist.head[i];

      if (is_check || uo_move_is_promotion(move) || uo_move_is_capture(move))
      {
        is_quiescent = false;

        uo_position_make_move(position, move);
        int16_t node_value = -uo_search_quiesce(thread, -beta, -beta + 1);
        node_value = uo_score_adjust_for_mate(node_value);
        uo_position_unmake_move(position);

        if (node_value > value)
        {
          value = node_value;

          if (value >= beta)
          {
            // fail-hard beta-cutoff
            return beta;
          }
        }

        if (value == alpha && uo_engine_is_stopped())
        {
          return value;
        }
      }
    }

    if (is_quiescent)
    {
      return static_evaluation;
    }

    return value;
  }

  int16_t value = -UO_SCORE_CHECKMATE;
  uo_move bestmove;
  uint8_t bestmove_depth;

  for (size_t i = 0; i < move_count; ++i)
  {
    // search later moves for reduced depth
    // see: https://en.wikipedia.org/wiki/Late_move_reductions
    float move_percentile = 1.0 - (float)i / (float)move_count;
    size_t depth_lmr = (1 + depth + (size_t)(move_percentile * depth)) >> 1;

    uo_move move = position->movelist.head[i];

    uo_position_make_move(position, move);
    int16_t node_value = -uo_search_zero_window(thread, depth_lmr - 1, -beta + 1);
    node_value = uo_score_adjust_for_mate(node_value);
    uo_position_unmake_move(position);

    if (node_value > value)
    {
      value = node_value;
      bestmove = move;
      bestmove_depth = depth_lmr;

      if (value >= beta)
      {
        return beta;
      }
    }

    if (value == alpha && uo_engine_is_stopped())
    {
      return value;
    }
  }

  return value;
}

// see: https://en.wikipedia.org/wiki/Negamax#Negamax_with_alpha_beta_pruning_and_transposition_tables
// see: https://en.wikipedia.org/wiki/Principal_variation_search
static int16_t uo_search_principal_variation(uo_engine_thread *thread, size_t depth, int16_t alpha, int16_t beta)
{
  uo_search_info *info = &thread->info;
  uo_position *position = &thread->position;
  uo_tentry *entry;
  int16_t value;
  uo_move bestmove = 0;

  if (uo_position_is_rule50_draw(position) || uo_position_is_repetition_draw(position))
  {
    return 0;
  }

  if (uo_engine_lookup_entry(position, &entry, &bestmove, &value, &alpha, &beta, depth))
  {
    return value;
  }

  ++info->nodes;

  if (uo_position_is_max_depth_reached(position))
  {
    return uo_position_evaluate(position);
  }

  size_t move_count = position->stack->moves_generated
    ? position->stack->move_count
    : uo_position_generate_moves(position);

  if (move_count == 0)
  {
    return uo_position_is_check(position) ? -UO_SCORE_CHECKMATE : 0;
  }

  // Null move pruning
  if (depth > 3 && uo_position_is_null_move_allowed(position) && uo_position_evaluate(position) > beta)
  {
    uo_position_make_null_move(position);
    // depth * 3/4
    size_t depth_nmp = depth * 3 >> 2;
    int16_t pass_value = -uo_search_principal_variation(thread, depth_nmp, -beta, -beta + 1);
    uo_position_unmake_null_move(position);

    if (pass_value >= beta)
    {
      return pass_value;
    }
  }

  uo_search_sort_and_prune_moves(thread, bestmove);

  if (depth == 0)
  {
    // TODO: Create separate multi-level quiescence search functions

    // Search depth reached. Let's perform quiescence search.

    bool is_check = uo_position_is_check(position);
    bool is_quiescent = !is_check;
    int16_t static_evaluation;
    uo_move bestmove = 0;
    value = -UO_SCORE_CHECKMATE;

    if (!is_check)
    {
      static_evaluation = uo_position_evaluate(position);
      const int16_t stand_pat_margin = -1;
      value = static_evaluation + stand_pat_margin;

      if (value >= beta)
      {
        return beta;
      }

      if (value > alpha)
      {
        alpha = value;
      }
    }

    for (size_t i = 0; i < move_count; ++i)
    {
      uo_move move = position->movelist.head[i];
      if (is_check || uo_move_is_promotion(move) || uo_move_is_capture(move))
      {
        is_quiescent = false;

        uo_position_make_move(position, move);
        int16_t node_value = -uo_search_quiesce(thread, -beta, -alpha);
        node_value = uo_score_adjust_for_mate(node_value);
        uo_position_unmake_move(position);

        if (node_value > value)
        {
          value = node_value;
          bestmove = move;

          if (value > alpha)
          {
            if (value >= beta)
            {
              return value;
            }

            alpha = value;
          }
        }

        if (value == alpha && uo_engine_is_stopped())
        {
          return value;
        }
      }
    }

    if (is_quiescent)
    {
      return static_evaluation;
    }

    return value;
  }

  bestmove = position->movelist.head[0];
  uint8_t bestmove_depth = depth;
  ++position->bftable[uo_color(position->flags)][bestmove & 0xFFF];

  uo_position_make_move(position, bestmove);
  // search first move with full alpha/beta window
  value = -uo_search_principal_variation(thread, depth - 1, -beta, -alpha);
  value = uo_score_adjust_for_mate(value);
  uo_position_unmake_move(position);

  if (value > alpha)
  {
    position->hhtable[uo_color(position->flags)][bestmove & 0xFFF] += 2 << depth;

    if (value >= beta)
    {
      if (!uo_move_is_capture(bestmove))
      {
        position->stack->search.killers[1] = position->stack->search.killers[0];
        position->stack->search.killers[0] = bestmove;
      }

      uo_engine_store_entry(position, entry, bestmove, value, alpha, beta, depth);
      return value;
    }

    alpha = value;
  }

  for (size_t i = 1; i < move_count; ++i)
  {
    // search later moves for reduced depth
    // see: https://en.wikipedia.org/wiki/Late_move_reductions
    float move_percentile = 1.0 - (float)i / (float)move_count;
    size_t depth_lmr = (1 + depth + (size_t)(move_percentile * depth)) >> 1;

    uo_move move = position->movelist.head[i];
    ++position->bftable[uo_color(position->flags)][move & 0xFFF];

    uo_position_make_move(position, move);

    // search with null window
    //int16_t node_value = -uo_search_zero_window(thread, depth_lmr - 1, -alpha);
    int16_t node_value = -uo_search_principal_variation(thread, depth_lmr - 1, -alpha - 1, -alpha);
    node_value = uo_score_adjust_for_mate(node_value);

    if (node_value > alpha && node_value < beta)
    {
      // failed high, do a full re-search
      node_value = -uo_search_principal_variation(thread, depth_lmr - 1, -beta, -alpha);
      node_value = uo_score_adjust_for_mate(node_value);
    }

    uo_position_unmake_move(position);

    if (node_value > value)
    {
      value = node_value;
      bestmove = move;
      bestmove_depth = depth_lmr;

      if (value > alpha)
      {
        position->hhtable[uo_color(position->flags)][move & 0xFFF] += 2 << depth_lmr;

        if (value >= beta)
        {
          if (!uo_move_is_capture(move))
          {
            position->stack->search.killers[1] = position->stack->search.killers[0];
            position->stack->search.killers[0] = move;
          }

          uo_engine_store_entry(position, entry, bestmove, value, alpha, beta, bestmove_depth);
          return value;
        }

        alpha = value;
      }
    }

    if (value == alpha && uo_engine_is_stopped())
    {
      return value;
    }
  }

  uo_engine_store_entry(position, entry, bestmove, value, alpha, beta, bestmove_depth);
  return value;
}

static void uo_search_info_print(uo_search_info *info)
{
  uo_engine_thread *thread = info->thread;
  uo_position *position = &thread->position;

  double time_msec = uo_time_elapsed_msec(&info->time_start);
  uint64_t nps = info->nodes / time_msec * 1000.0;

  uo_engine_lock_ttable();
  uint64_t hashfull = (engine.ttable.count * 1000) / (engine.ttable.hash_mask + 1);
  uo_engine_unlock_ttable();

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

    printf("nodes %" PRIu64 " nps %" PRIu64 " hashfull %" PRIu64 " tbhits %d time %.0f",
      info->nodes, nps, hashfull, info->tbhits, time_msec);

    size_t pv_len = 1;

    size_t i = 0;
    uo_move move = info->bestmove;

    uint8_t color = uo_color(position->flags);
    uint16_t side_xor = 0;

    if (move)
    {
      printf(" pv");
      while (move)
      {
        uo_position_print_move(position, move ^ side_xor, buf);
        side_xor ^= 0xE38;
        printf(" %s", buf);
        move = info->pv[++i];
      }
    }

    printf("\n");

    if (info->completed && info->multipv == 1)
    {
      uo_position_print_move(position, info->bestmove, buf);
      printf("bestmove %s\n", buf);
    }
  }

  uo_engine_unlock_stdout();
}

static void uo_search_info_currmove_print(uo_search_info *info, uo_move currmove, size_t currmovenumber)
{
  uo_engine_thread *thread = info->thread;
  uo_position *position = &thread->position;

  uo_engine_lock_stdout();
  uo_position_print_move(position, currmove, buf);
  printf("info depth %d currmove %s currmovenumber %zu", info->depth, buf, currmovenumber);
  uo_engine_unlock_stdout();
}

static void uo_engine_thread_release_pv(uo_engine_thread *thread)
{
  uo_tentry *pv_entry = thread->entry;

  if (!pv_entry) return;

  uo_position *position = &thread->position;
  uo_move *pv = thread->info.pv;

  uo_engine_lock_ttable();

  assert(pv_entry->key == (uint32_t)position->key);

  size_t i = 0;
  while (pv[i + 1])
  {
    --pv_entry->refcount;
    uo_position_make_move(position, pv[i++]);
    pv_entry = uo_ttable_get(&engine.ttable, position);
  }

  while (i)
  {
    pv[--i] = 0;
    uo_position_unmake_move(position);
  }

  uo_engine_unlock_ttable();

  thread->entry = NULL;
}

static void uo_engine_thread_update_pv(uo_engine_thread *thread)
{
  uo_position *position = &thread->position;
  uo_tentry *pv_entry = thread->entry;
  uo_move *pv = thread->info.pv;

  // TODO: better handling for repetitions
  size_t max_depth = thread->info.depth;
  if (thread->info.seldepth > max_depth) max_depth = thread->info.seldepth;
  if (max_depth >= UO_MAX_PLY) max_depth = UO_MAX_PLY - 1;

  uo_engine_lock_ttable();

  // 1. Get entry if not set
  if (!pv_entry)
  {
    thread->entry = pv_entry = uo_ttable_get(&engine.ttable, position);
    ++pv_entry->refcount;
  }

  assert(pv_entry->key == (uint32_t)position->key);

  // 2. Update score
  thread->info.value = pv_entry->value;

  // 3. Release obsolete pv entries
  size_t i = 0;
  while (pv[i + 1])
  {
    uo_position_make_move(position, pv[i++]);
    uo_tentry *entry = uo_ttable_get(&engine.ttable, position);
    if (entry) --entry->refcount;
  }

  while (i)
  {
    pv[--i] = 0;
    uo_position_unmake_move(position);
  }

  // 4. Save current pv
  pv[i++] = pv_entry->bestmove;
  uo_position_make_move(position, pv_entry->bestmove);
  pv_entry = uo_ttable_get(&engine.ttable, position);

  while (pv_entry && pv_entry->bestmove && i <= max_depth)
  {
    ++pv_entry->refcount;
    pv[i++] = pv_entry->bestmove;
    uo_position_make_move(position, pv_entry->bestmove);
    pv_entry = uo_ttable_get(&engine.ttable, position);
  }

  pv[i] = 0;

  while (i--)
  {
    uo_position_unmake_move(position);
  }

  uo_engine_unlock_ttable();
}

static inline void uo_engine_thread_load_position(uo_engine_thread *thread)
{
  uo_mutex_lock(engine.position_mutex);
  uo_position_copy(&thread->position, &engine.position);
  uo_mutex_unlock(engine.position_mutex);
}

void *uo_engine_thread_start_timer(void *arg)
{
  uo_engine_thread_work *work = arg;
  uo_engine_thread *thread = work->data;

  while (!uo_engine_is_stopped())
  {
    uo_sleep_msec(100);
    uo_search_stop_if_movetime_over(thread);
  }

  return NULL;
}

static inline bool uo_search_adjust_alpha_beta(int16_t score, int16_t *alpha, int16_t *beta, size_t *fail_count)
{
  const int16_t aspiration_window_fail_threshold = 2;
  const int16_t aspiration_window_minimum = 40;
  const float aspiration_window_factor = 1.5;
  const int16_t aspiration_window_mate = (UO_SCORE_CHECKMATE - 1) / aspiration_window_factor;

  int16_t score_abs = score > 0 ? score : -score;
  int16_t aspiration_window = (float)score_abs * aspiration_window_factor;

  if (aspiration_window < aspiration_window_minimum)
  {
    aspiration_window = aspiration_window_minimum;
  }

  if (score <= *alpha)
  {
    *fail_count++;

    if (*fail_count >= aspiration_window_fail_threshold)
    {
      *alpha = -UO_SCORE_CHECKMATE;
      return false;
    }

    if (score < -aspiration_window_mate)
    {
      *alpha = -UO_SCORE_CHECKMATE;
      return false;
    }

    *alpha = score - aspiration_window;
    return false;
  }

  if (score >= *beta)
  {
    *fail_count++;

    if (*fail_count >= aspiration_window_fail_threshold)
    {
      *beta = UO_SCORE_CHECKMATE;
      return false;
    }

    if (score > aspiration_window_mate)
    {
      *beta = UO_SCORE_CHECKMATE;
      return false;
    }

    *beta = score + aspiration_window;
    return false;
  }

  *fail_count = 0;

  if (score < -aspiration_window_mate)
  {
    *alpha = -UO_SCORE_CHECKMATE;
    *beta = -aspiration_window_mate;
    return true;
  }

  if (score > aspiration_window_mate)
  {
    *alpha = aspiration_window_mate;
    *beta = UO_SCORE_CHECKMATE;
    return true;
  }

  *alpha = score - aspiration_window;
  *beta = score + aspiration_window;
  return true;
}

void *uo_engine_thread_run_principal_variation_search(void *arg)
{
  uo_engine_thread_work *work = arg;
  uo_engine_thread *thread = work->thread;
  uo_position *position = &thread->position;
  uo_search_params *params = &engine.search_params;

  thread->info = (uo_search_info){
    .thread = thread,
    .depth = 1,
    .multipv = engine_options.multipv,
    .nodes = 0
  };

  uo_time_now(&thread->info.time_start);
  uo_engine_thread_load_position(thread);
  uo_engine_queue_work(uo_engine_thread_start_timer, thread);

  int16_t alpha = params->alpha;
  int16_t beta = params->beta;
  size_t aspiration_fail_count = 0;

  int16_t score = uo_search_principal_variation(thread, 1, alpha, beta);
  uo_engine_thread_update_pv(thread);
  uo_search_adjust_alpha_beta(score, &alpha, &beta, &aspiration_fail_count);

  for (size_t depth = 2; depth <= params->depth; ++depth)
  {
    if (thread->info.nodes)
    {
      uo_search_info_print(&thread->info);
    }

    thread->info.nodes = 0;

    while (!uo_engine_is_stopped())
    {
      score = uo_search_principal_variation(thread, depth, alpha, beta);

      if (uo_search_adjust_alpha_beta(score, &alpha, &beta, &aspiration_fail_count))
      {
        uo_engine_thread_update_pv(thread);
        break;
      }
    }

    if (uo_engine_is_stopped()) break;

    thread->info.depth = depth;
  }

  thread->info.completed = true;
  uo_search_info_print(&thread->info);

  uo_engine_lock_position();
  for (size_t i = 0; i < 4096; ++i)
  {
    engine.position.hhtable[0][i] = position->hhtable[0][i] >> 1;
    engine.position.bftable[0][i] = position->bftable[0][i] >> 1;
  }
  uo_engine_unlock_position();

  uo_engine_thread_release_pv(thread);
  uo_engine_stop_search();

  return NULL;
}

void *uo_engine_thread_run_quiescence_search(void *arg)
{
  uo_engine_thread_work *work = arg;
  uo_engine_thread *thread = work->thread;
  uo_position *position = &thread->position;
  uo_search_params *params = &engine.search_params;

  thread->info = (uo_search_info){
    .thread = thread,
    .depth = 0,
    .multipv = engine_options.multipv,
    .nodes = 0
  };

  uo_time_now(&thread->info.time_start);
  uo_engine_thread_load_position(thread);
  uo_engine_queue_work(uo_engine_thread_start_timer, thread);

  uint8_t entry_type;
  uo_move bestmove = 0;
  int16_t quiescence_value;
  int16_t alpha = params->alpha;
  int16_t beta = params->beta;

  do
  {
    if (uo_position_is_rule50_draw(position) || uo_position_is_repetition_draw(position))
    {
      entry_type = uo_tentry_type__exact;
      quiescence_value = 0;
      goto return_quiescence;
    }

    size_t move_count = position->stack->moves_generated
      ? position->stack->move_count
      : uo_position_generate_moves(position);

    if (move_count == 0)
    {
      entry_type = uo_tentry_type__exact;
      quiescence_value = uo_position_is_check(position) ? -UO_SCORE_CHECKMATE : 0;
      goto return_quiescence;
    }

    bool is_check = uo_position_is_check(position);
    bool is_quiescent = !is_check;
    int16_t static_evaluation;
    int16_t value = -UO_SCORE_CHECKMATE;

    if (!is_check)
    {
      static_evaluation = uo_position_evaluate(position);
      const int16_t stand_pat_margin = -1;
      value = static_evaluation + stand_pat_margin;

      if (value >= beta)
      {
        entry_type = uo_tentry_type__lower_bound;
        quiescence_value = beta;
        goto return_quiescence;
      }

      if (value > alpha)
      {
        alpha = value;
      }
    }

    for (size_t i = 0; i < move_count; ++i)
    {
      uo_move move = position->movelist.head[i];

      if (is_check || uo_move_is_promotion(move) || uo_move_is_capture(move))
      {
        is_quiescent = false;

        uo_position_make_move(position, move);
        int16_t node_value = -uo_search_quiesce(thread, -beta, -alpha);
        node_value = uo_score_adjust_for_mate(node_value);
        uo_position_unmake_move(position);

        if (node_value > value)
        {
          value = node_value;
          bestmove = move;

          if (value > alpha)
          {
            if (value >= beta)
            {
              entry_type = uo_tentry_type__lower_bound;
              quiescence_value = value;
              goto return_quiescence;
            }

            alpha = value;
          }
        }
      }

      if (value == alpha && uo_engine_is_stopped())
      {
        entry_type = uo_tentry_type__upper_bound;
        quiescence_value = alpha;
        goto return_quiescence;
      }
    }

    if (is_quiescent)
    {
      quiescence_value = static_evaluation;
      entry_type = uo_tentry_type__exact;
      goto return_quiescence;
    }

    entry_type = value == alpha ? uo_tentry_type__exact : uo_tentry_type__upper_bound;
    quiescence_value = value;
    goto return_quiescence;
  } while (false);

return_quiescence:;

  double time_msec = uo_time_elapsed_msec(&thread->info.time_start);
  uint64_t nps = thread->info.nodes / time_msec * 1000.0;

  uo_engine_lock_ttable();
  uint64_t hashfull = (engine.ttable.count * 1000) / (engine.ttable.hash_mask + 1);
  uo_engine_unlock_ttable();

  uo_engine_lock_stdout();

  printf("info qsearch ");
  if (thread->info.seldepth) printf("seldepth %d ", thread->info.seldepth);

  int16_t score = quiescence_value;

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

  if (entry_type & uo_tentry_type__lower_bound)
  {
    printf("lowerbound ");
  }
  else if (entry_type & uo_tentry_type__upper_bound)
  {
    printf("upperbound ");
  }

  printf("nodes %" PRIu64 " nps %" PRIu64 " hashfull %" PRIu64 " tbhits %d time %.0f",
    thread->info.nodes, nps, hashfull, thread->info.tbhits, time_msec);

  if (bestmove)
  {
    uo_position_print_move(position, bestmove, buf);
    printf(" pv %s", buf);
  }

  printf("\n");

  uo_engine_unlock_stdout();

  uo_engine_stop_search();

  return NULL;
}
