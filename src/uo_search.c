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

typedef uint8_t uo_search_quiesce_flags;

#define uo_search_quiesce_flags__checks ((uint8_t)1)
#define uo_search_quiesce_flags__positive_sse ((uint8_t)2)
#define uo_search_quiesce_flags__non_negative_sse ((uint8_t)6)
#define uo_search_quiesce_flags__any_sse ((uint8_t)14)
#define uo_search_quiesce_flags__all_moves ((uint8_t)-1)

#define uo_search_quiesce_flags__root (uo_search_quiesce_flags__checks | uo_search_quiesce_flags__non_negative_sse)
#define uo_search_quiesce_flags__subsequent uo_search_quiesce_flags__root
#define uo_search_quiesce_flags__deep uo_search_quiesce_flags__positive_sse
#define uo_search_quiesce__deep_threshold 6

static inline bool uo_search_quiesce_should_examine_move(uo_engine_thread *thread, uo_move move, uo_search_quiesce_flags flags)
{
  if (flags == uo_search_quiesce_flags__all_moves)
  {
    return true;
  }

  int16_t sse = (flags & uo_search_quiesce_flags__any_sse) == uo_search_quiesce_flags__any_sse
    ? 1
    : uo_position_move_sse(&thread->position, move);

  bool sufficient_sse = sse > 0
    || (sse == 0 && ((flags & uo_search_quiesce_flags__non_negative_sse) == uo_search_quiesce_flags__non_negative_sse));

  if (sufficient_sse)
  {
    return false;
  }

  if (uo_move_is_promotion(move)) return true;
  if (uo_move_is_capture(move)) return true;

  if ((flags & uo_search_quiesce_flags__checks) == uo_search_quiesce_flags__checks
    && uo_position_is_check_move(&thread->position, move))
  {
    return true;
  }

  return false;
}

static int16_t uo_search_quiesce(uo_engine_thread *thread, int16_t alpha, int16_t beta, uo_search_quiesce_flags flags, uint8_t depth)
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

  if (uo_position_is_max_depth_reached(position))
  {
    return uo_position_evaluate(position);
  }

  size_t move_count = uo_position_generate_moves(position);
  bool is_check = uo_position_is_check(position);

  if (move_count == 0)
  {
    return is_check ? -UO_SCORE_CHECKMATE : 0;
  }

  bool is_quiescent = !is_check;
  int16_t static_evaluation;
  int16_t value;
  uo_move bestmove = 0;

  if (is_check)
  {
    flags = uo_search_quiesce_flags__all_moves;
    value = -UO_SCORE_CHECKMATE;
    uo_position_sort_moves(&thread->position, 0);
  }
  else
  {
    value = uo_position_evaluate(position);

    if (value >= beta)
    {
      return value;
    }

    // delta pruning
    if (position->Q)
    {
      int16_t delta = 1000;
      if (alpha - delta > value)
      {
        return alpha;
      }
    }

    if (value > alpha)
    {
      alpha = value;
    }

    uo_position_sort_tactical_moves(&thread->position);
  }

  uo_search_quiesce_flags flags_next = depth >= uo_search_quiesce__deep_threshold
    ? uo_search_quiesce_flags__deep
    : uo_search_quiesce_flags__subsequent;

  for (size_t i = 0; i < move_count; ++i)
  {
    uo_move move = position->movelist.head[i];

    if (uo_search_quiesce_should_examine_move(thread, move, flags))
    {
      is_quiescent = false;

      uo_position_make_move(position, move);
      int16_t node_value = -uo_search_quiesce(thread, -beta, -alpha, flags_next, depth + 1);
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

      if (uo_engine_is_stopped())
      {
        return value;
      }
    }
  }

  return value;
}

// see: https://en.wikipedia.org/wiki/Negamax#Negamax_with_alpha_beta_pruning_and_transposition_tables
// see: https://en.wikipedia.org/wiki/Principal_variation_search
static int16_t uo_search_principal_variation(uo_engine_thread *thread, size_t depth, int16_t alpha, int16_t beta)
{
  uo_position *position = &thread->position;

  if (uo_position_is_rule50_draw(position) || uo_position_is_repetition_draw(position))
  {
    return 0;
  }

  uo_abtentry entry = { alpha, beta, depth };
  if (uo_engine_lookup_entry(position, &entry))
  {
    return entry.value;
  }

  ++thread->info.nodes;

  if (depth == 0)
  {
    entry.value = uo_search_quiesce(thread, alpha, beta, uo_search_quiesce_flags__root, 0);
    return uo_engine_store_entry(position, &entry);
  }

  if (uo_position_is_max_depth_reached(position))
  {
    entry.value = uo_position_evaluate(position);
    return uo_engine_store_entry(position, &entry);
  }

  size_t move_count = position->stack->moves_generated
    ? position->stack->move_count
    : uo_position_generate_moves(position);

  if (move_count == 0)
  {
    entry.value = uo_position_is_check(position) ? -UO_SCORE_CHECKMATE : 0;
    return uo_engine_store_entry(position, &entry);
  }

  // Null move pruning
  if (depth > 3 && position->ply > 1 && uo_position_is_null_move_allowed(position) && uo_position_evaluate(position) > beta)
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

  uo_position_sort_moves(&thread->position, entry.bestmove);

  entry.bestmove = position->movelist.head[0];
  entry.depth = depth;
  ++position->bftable[uo_color(position->flags)][entry.bestmove & 0xFFF];

  uo_position_make_move(position, entry.bestmove);
  // search first move with full alpha/beta window
  entry.value = -uo_search_principal_variation(thread, depth - 1, -beta, -alpha);
  entry.value = uo_score_adjust_for_mate(entry.value);
  uo_position_unmake_move(position);

  if (entry.value > alpha)
  {
    position->hhtable[uo_color(position->flags)][entry.bestmove & 0xFFF] += 2 << depth;

    if (entry.value >= beta)
    {
      if (!uo_move_is_capture(entry.bestmove))
      {
        position->stack->search.killers[1] = position->stack->search.killers[0];
        position->stack->search.killers[0] = entry.bestmove;
      }

      return uo_engine_store_entry(position, &entry);
    }

    alpha = entry.value;
  }

  for (size_t i = 1; i < move_count; ++i)
  {
    uo_move move = position->movelist.head[i];
    ++position->bftable[uo_color(position->flags)][move & 0xFFF];

    uo_position_make_move(position, move);

    // search later moves for reduced depth
    // see: https://en.wikipedia.org/wiki/Late_move_reductions
    size_t depth_lmr = depth > 3 ? depth - 1 : depth;

    // search with null window
    int16_t node_value = -uo_search_principal_variation(thread, depth_lmr - 1, -alpha - 1, -alpha);
    node_value = uo_score_adjust_for_mate(node_value);

    if (node_value > alpha && node_value < beta)
    {
      // failed high, do a full re-search
      node_value = -uo_search_principal_variation(thread, depth_lmr - 1, -beta, -alpha);
      node_value = uo_score_adjust_for_mate(node_value);
    }

    uo_position_unmake_move(position);

    if (node_value > entry.value)
    {
      entry.value = node_value;
      entry.bestmove = move;
      entry.depth = depth_lmr;

      if (entry.value > alpha)
      {
        position->hhtable[uo_color(position->flags)][move & 0xFFF] += 2 << depth_lmr;

        if (entry.value >= beta)
        {
          if (!uo_move_is_capture(move))
          {
            position->stack->search.killers[1] = position->stack->search.killers[0];
            position->stack->search.killers[0] = move;
          }

          return uo_engine_store_entry(position, &entry);
        }

        alpha = entry.value;
      }
    }

    if (uo_engine_is_stopped())
    {
      return entry.value;
    }
  }

  return uo_engine_store_entry(position, &entry);
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
      printf("score mate %d ", -((UO_SCORE_CHECKMATE + score + 1) >> 1));
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

  // 2. Release obsolete pv entries
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

  // 3. Save current pv
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

static inline bool uo_search_adjust_alpha_beta(int16_t value, int16_t *alpha, int16_t *beta, size_t *fail_count)
{
  const int16_t aspiration_window_fail_threshold = 2;
  const int16_t aspiration_window_minimum = 40;
  const float aspiration_window_factor = 1.5;
  const int16_t aspiration_window_mate = (UO_SCORE_CHECKMATE - 1) / aspiration_window_factor;

  int16_t value_abs = value > 0 ? value : -value;
  int16_t aspiration_window = (float)value_abs * aspiration_window_factor;

  if (aspiration_window < aspiration_window_minimum)
  {
    aspiration_window = aspiration_window_minimum;
  }

  if (value <= *alpha)
  {
    if ((*fail_count)++ >= aspiration_window_fail_threshold)
    {
      *alpha = -UO_SCORE_CHECKMATE;
      return false;
    }

    if (value < -aspiration_window_mate)
    {
      *alpha = -UO_SCORE_CHECKMATE;
      return false;
    }

    *alpha = value - aspiration_window;
    return false;
  }

  if (value >= *beta)
  {
    if ((*fail_count)++ >= aspiration_window_fail_threshold)
    {
      *beta = UO_SCORE_CHECKMATE;
      return false;
    }

    if (value > aspiration_window_mate)
    {
      *beta = UO_SCORE_CHECKMATE;
      return false;
    }

    *beta = value + aspiration_window;
    return false;
  }

  *fail_count = 0;

  if (value < -aspiration_window_mate)
  {
    *alpha = -UO_SCORE_CHECKMATE;
    *beta = -aspiration_window_mate;
    return true;
  }

  if (value > aspiration_window_mate)
  {
    *alpha = aspiration_window_mate;
    *beta = UO_SCORE_CHECKMATE;
    return true;
  }

  *alpha = value - aspiration_window;
  *beta = value + aspiration_window;
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

  int16_t value = uo_search_principal_variation(thread, 1, alpha, beta);
  uo_engine_thread_update_pv(thread);
  uo_search_adjust_alpha_beta(value, &alpha, &beta, &aspiration_fail_count);
  thread->info.value = value;

  for (size_t depth = 2; depth <= params->depth; ++depth)
  {
    if (thread->info.nodes)
    {
      uo_search_info_print(&thread->info);
    }

    thread->info.nodes = 0;

    while (!uo_engine_is_stopped())
    {
      value = uo_search_principal_variation(thread, depth, alpha, beta);

      if (uo_search_adjust_alpha_beta(value, &alpha, &beta, &aspiration_fail_count))
      {
        uo_engine_thread_update_pv(thread);
        thread->info.value = value;
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

  int16_t alpha = params->alpha;
  int16_t beta = params->beta;
  int16_t value = uo_search_quiesce(thread, alpha, beta, uo_search_quiesce_flags__root, 0);

  double time_msec = uo_time_elapsed_msec(&thread->info.time_start);
  uint64_t nps = thread->info.nodes / time_msec * 1000.0;

  uo_engine_lock_ttable();
  uint64_t hashfull = (engine.ttable.count * 1000) / (engine.ttable.hash_mask + 1);
  uo_engine_unlock_ttable();

  uo_engine_lock_stdout();

  printf("info qsearch ");
  if (thread->info.seldepth) printf("seldepth %d ", thread->info.seldepth);

  if (value > UO_SCORE_MATE_IN_THRESHOLD)
  {
    printf("score mate %d ", (UO_SCORE_CHECKMATE - value + 1) >> 1);
  }
  else if (value < -UO_SCORE_MATE_IN_THRESHOLD)
  {
    printf("score mate %d ", -((UO_SCORE_CHECKMATE + value + 1) >> 1));
  }
  else
  {
    printf("score cp %d ", value);
  }

  if (value >= beta)
  {
    printf("lowerbound ");
  }
  else if (value <= alpha)
  {
    printf("upperbound ");
  }

  printf("nodes %" PRIu64 " nps %" PRIu64 " hashfull %" PRIu64 " tbhits %d time %.0f",
    thread->info.nodes, nps, hashfull, thread->info.tbhits, time_msec);

  printf("\n");

  uo_engine_unlock_stdout();

  uo_engine_stop_search();

  return NULL;
}
