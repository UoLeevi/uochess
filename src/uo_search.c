﻿#include "uo_search.h"
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

typedef struct uo_parallel_search_params
{
  uo_engine_thread *thread;
  uo_search_queue queue;
  size_t depth;
  int16_t alpha;
  int16_t beta;
  uo_move move;
} uo_parallel_search_params;

static inline void uo_search_stop_if_movetime_over(uo_engine_thread *thread)
{
  uo_search_info *info = &thread->info;
  uo_search_params *params = &engine.search_params;
  uint64_t movetime = params->movetime;

  // TODO: make better decisions about how much time to use
  if (!movetime && params->time_own)
  {
    movetime = 2500 + (params->time_own - params->time_enemy) / 2;

    size_t min_depth = 9;

    if (movetime > params->time_own / 8)
    {
      movetime = params->time_own / 8 + 1;
      min_depth = 1;
    }

    uint16_t score_abs = info->value < 0 ? -info->value : info->value;

    if (score_abs < UO_SCORE_MATE_IN_THRESHOLD - uo_piece__Q)
    {
      if (info->depth <= min_depth) return;
    }
  }

  if (movetime)
  {
    const double margin_msec = 1.0;
    double time_msec = uo_time_elapsed_msec(&thread->info.time_start);

    if (time_msec + margin_msec > (double)movetime)
    {
      if (!info->bestmove) return;
      uo_engine_stop_search();
    }
  }
}

bool uo_engine_thread_is_pv_ok(uo_engine_thread *thread)
{
  uo_move *pv = thread->info.pv;
  while (*pv)
  {
    ++pv;
  }

  for (int i = 0; i < 20; ++i)
  {
    if (*pv++) return false;
  }

  return true;
}
//
//static void uo_engine_thread_update_pv(uo_engine_thread *thread)
//{
//  assert(uo_engine_thread_is_pv_ok(thread));
//
//  uo_position *position = &thread->position;
//  uo_move *pv = thread->info.pv;
//
//  // TODO: better handling for repetitions
//  size_t max_depth = thread->info.depth;
//
//  uo_engine_lock_ttable();
//
//  // 1. Get entry
//  uo_tentry *pv_entry = uo_ttable_get(&engine.ttable, position);
//
//  // 2. Release obsolete pv entries
//  size_t ply = position->ply;
//  size_t i = ply;
//  while (pv[i + 1])
//  {
//    uo_position_make_move(position, pv[i++]);
//    uo_tentry *entry = uo_ttable_get(&engine.ttable, position);
//    if (entry) entry->thread_id = 0;
//  }
//
//  while (ply < i)
//  {
//    pv[i--] = 0;
//    uo_position_unmake_move(position);
//  }
//
//  // 3. Save current pv
//
//  while (pv_entry && pv_entry->bestmove && i <= max_depth)
//  {
//    pv_entry->thread_id = thread->id;
//    pv[i++] = pv_entry->bestmove;
//    uo_position_make_move(position, pv_entry->bestmove);
//    pv_entry = uo_ttable_get(&engine.ttable, position);
//  }
//
//  pv[i] = 0;
//
//  while (ply < i--)
//  {
//    uo_position_unmake_move(position);
//  }
//
//  uo_engine_unlock_ttable();
//
//  assert(uo_engine_thread_is_pv_ok(thread));
//}
//
//static void uo_engine_thread_release_pv(uo_engine_thread *thread)
//{
//  assert(uo_engine_thread_is_pv_ok(thread));
//
//  uo_position *position = &thread->position;
//  uo_move *pv = thread->info.pv;
//
//  uo_engine_lock_ttable();
//
//  uo_tentry *pv_entry = uo_ttable_get(&engine.ttable, position);
//  assert(pv_entry->key == (uint32_t)position->key);
//
//  size_t i = 0;
//  while (pv[i])
//  {
//    pv_entry->thread_id = 0;
//    uo_position_make_move(position, pv[i++]);
//    pv_entry = uo_ttable_get(&engine.ttable, position);
//  }
//
//  uo_engine_unlock_ttable();
//
//  while (i)
//  {
//    pv[--i] = 0;
//    uo_position_unmake_move(position);
//  }
//
//  assert(memcmp(pv, (uint8_t[20]) { 0 }, 20) == 0);
//}

static void uo_search_print_info(uo_engine_thread *thread)
{
  uo_search_info *info = &thread->info;
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

typedef uint8_t uo_search_quiesce_flags;

#define uo_search_quiesce_flags__checks ((uint8_t)1)
#define uo_search_quiesce_flags__positive_sse ((uint8_t)2)
#define uo_search_quiesce_flags__non_negative_sse ((uint8_t)6)
#define uo_search_quiesce_flags__any_sse ((uint8_t)14)
#define uo_search_quiesce_flags__all_moves ((uint8_t)-1)

static inline uo_search_quiesce_flags uo_search_quiesce_determine_flags(uo_engine_thread *thread, uint8_t depth)
{
  uo_position *position = &thread->position;
  uo_search_quiesce_flags flags = uo_search_quiesce_flags__positive_sse;
  uint8_t material_percentage = uo_position_material_percentage(position);

  // In endgame, let's examine more checks and captures.
  // The less material is on the board, the more checks should be examined.
  size_t material_weighted_depth = depth * (material_percentage * material_percentage) / 1000;

  if (material_weighted_depth < 5)
  {
    flags = uo_search_quiesce_flags__non_negative_sse | uo_search_quiesce_flags__checks;
  }
  else if (material_weighted_depth < 10)
  {
    flags = uo_search_quiesce_flags__non_negative_sse;
  }

  return flags;
}

static inline bool uo_search_quiesce_should_examine_move(uo_engine_thread *thread, uo_move move, uo_search_quiesce_flags flags)
{
  if ((flags & uo_search_quiesce_flags__any_sse) == uo_search_quiesce_flags__positive_sse && !uo_move_is_capture(move))
  {
    return false;
  }

  uo_position *position = &thread->position;

  int16_t sse = (flags & uo_search_quiesce_flags__any_sse) == uo_search_quiesce_flags__any_sse
    ? 1
    : uo_position_move_sse(position, move);

  bool sufficient_sse = sse > 0
    || (sse == 0 && ((flags & uo_search_quiesce_flags__non_negative_sse) == uo_search_quiesce_flags__non_negative_sse));

  if (!sufficient_sse)
  {
    return false;
  }

  if (uo_move_is_promotion(move)) return true;
  if (uo_move_is_capture(move)) return true;

  if ((flags & uo_search_quiesce_flags__checks) == uo_search_quiesce_flags__checks)
  {
    uo_bitboard checks = uo_position_move_checks(position, move);
    if (checks)
    {
      uo_position_update_next_move_checks(position, checks);
      return true;
    }
  }

  return false;
}

static int16_t uo_search_quiesce(uo_engine_thread *thread, int16_t alpha, int16_t beta, uint8_t depth)
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

  if (is_check)
  {
    int16_t value = -UO_SCORE_CHECKMATE;
    uo_position_sort_moves(&thread->position, 0);

    for (size_t i = 0; i < move_count; ++i)
    {
      uo_move move = position->movelist.head[i];
      uo_position_make_move(position, move);
      int16_t node_value = -uo_search_quiesce(thread, -beta, -alpha, depth + 1);
      node_value = uo_score_adjust_for_mate(node_value);
      uo_position_unmake_move(position);

      if (node_value > value)
      {
        value = node_value;

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

      if (thread->owner && uo_atomic_load(&thread->cutoff))
      {
        return value;
      }
    }

    return value;
  }

  int16_t value = uo_position_evaluate(position);

  if (value >= beta)
  {
    return value;
  }

  // delta pruning
  if (position->Q)
  {
    int16_t delta = 600;
    if (alpha > value + delta)
    {
      return alpha;
    }
  }

  if (value > alpha)
  {
    alpha = value;
  }

  uo_search_quiesce_flags flags = uo_search_quiesce_determine_flags(thread, depth);
  uo_position_sort_tactical_moves(&thread->position);

  for (size_t i = 0; i < move_count; ++i)
  {
    uo_move move = position->movelist.head[i];

    if (uo_search_quiesce_should_examine_move(thread, move, flags))
    {
      uo_position_make_move(position, move);
      int16_t node_value = -uo_search_quiesce(thread, -beta, -alpha, depth + 1);
      node_value = uo_score_adjust_for_mate(node_value);
      uo_position_unmake_move(position);

      if (node_value > value)
      {
        value = node_value;

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

      if (thread->owner && uo_atomic_load(&thread->cutoff))
      {
        return value;
      }
    }
  }

  return value;
}

static inline void uo_search_cutoff_parallel_search(const uo_engine_thread *thread, uo_search_queue *queue)
{
  for (size_t i = 0; i < UO_PARALLEL_MAX_COUNT; ++i)
  {
    uo_engine_thread *parallel_thread = queue->threads[i];
    if (parallel_thread)
    {
      queue->threads[i] = NULL;
      uo_atomic_lock(&parallel_thread->busy);
      if (parallel_thread->owner == thread)
      {
        uo_atomic_store(&parallel_thread->cutoff, 1);
      }
      uo_atomic_unlock(&parallel_thread->busy);
    }
  }
}

static inline bool uo_search_try_delegate_parallel_search(uo_engine_thread *thread, uo_parallel_search_params *params)
{
  uo_search_queue_init(&params->queue);

  uo_engine_thread *parallel_thread = uo_engine_run_thread_if_available(uo_engine_thread_run_parallel_principal_variation_search, params);

  if (!parallel_thread)
  {
    return false;
  }

  uo_atomic_increment(&params->queue.pending_count);

  for (size_t i = 0; i < UO_PARALLEL_MAX_COUNT; ++i)
  {
    if (!params->queue.threads[i])
    {
      params->queue.threads[i] = parallel_thread;
      break;
    }
  }

  uo_atomic_lock(&parallel_thread->busy);
  uo_atomic_unlock(&parallel_thread->busy);

  return true;
}

// see: https://en.wikipedia.org/wiki/Negamax#Negamax_with_alpha_beta_pruning_and_transposition_tables
// see: https://en.wikipedia.org/wiki/Principal_variation_search
static int16_t uo_search_principal_variation(uo_engine_thread *thread, size_t depth, int16_t alpha, int16_t beta, uo_move *pline, bool pv)
{
  uo_position *position = &thread->position;

  if (uo_position_is_rule50_draw(position) || uo_position_is_repetition_draw(position))
  {
    return 0;
  }

  uo_abtentry entry = { alpha, beta, depth };
  if (uo_engine_lookup_entry(position, &entry))
  {
    //if (pv && entry.value > alpha)
    //{
    //  uo_engine_thread_update_pv(thread);
    //}

    return entry.value;
  }

  if (thread->owner && uo_atomic_load(&thread->cutoff))
  {
    return entry.value;
  }

  ++thread->info.nodes;

  if (depth == 0)
  {
    entry.value = uo_search_quiesce(thread, alpha, beta, 0);
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

  uo_move *line = uo_alloca(depth * sizeof * line);
  line[0] = 0;

  // Null move pruning
  if (!pv
    && depth > 3
    && position->ply > 1
    && uo_position_is_null_move_allowed(position)
    && uo_position_evaluate(position) > beta)
  {
    uo_position_make_null_move(position);
    // depth * 3/4
    size_t depth_nmp = depth * 3 >> 2;
    int16_t pass_value = -uo_search_principal_variation(thread, depth_nmp, -beta, -beta + 1, line, false);
    uo_position_unmake_null_move(position);

    if (pass_value >= beta)
    {
      return pass_value;
    }
  }

  uo_position_sort_moves(&thread->position, pline[0] ? pline[0] : entry.bestmove);

  entry.bestmove = position->movelist.head[0];
  entry.depth = depth;

  uo_position_update_butterfly_heuristic(position, entry.bestmove);

  uo_position_make_move(position, entry.bestmove);
  // search first move with full alpha/beta window
  entry.value = -uo_search_principal_variation(thread, depth - 1, -beta, -alpha, pline + 1, pv);
  entry.value = uo_score_adjust_for_mate(entry.value);
  uo_position_unmake_move(position);

  if (entry.value > alpha)
  {
    uo_position_update_history_heuristic(position, entry.bestmove, depth);

    if (entry.value >= beta)
    {
      uo_position_update_killers(position, entry.bestmove);
      return uo_engine_store_entry(position, &entry);
    }

    alpha = entry.value;
    pline[0] = entry.bestmove;

    //if (pv)
    //{
    //  uo_engine_store_entry(position, &entry);
    //  uo_engine_thread_update_pv(thread);
    //}
  }

  if (uo_engine_is_stopped())
  {
    return entry.value;
  }

  if (thread->owner && uo_atomic_load(&thread->cutoff))
  {
    return entry.value;
  }

  size_t parallel_search_count = 0;
  uo_parallel_search_params params = {
    .thread = thread,
    .queue = {.init = 0 },
    .alpha = alpha,
    .beta = beta,
  };

  for (size_t i = 1; i < move_count; ++i)
  {
    uo_move move = params.move = position->movelist.head[i];
    uo_position_update_butterfly_heuristic(position, move);

    // search later moves for reduced depth
    // see: https://en.wikipedia.org/wiki/Late_move_reductions
    size_t depth_lmr = params.depth = depth <= 3 ? depth : depth - 1;

    bool can_delegate = (depth_lmr >= UO_PARALLEL_MIN_DEPTH) && (move_count - i > UO_PARALLEL_MIN_MOVE_COUNT) && (parallel_search_count < UO_PARALLEL_MAX_COUNT);

    if (can_delegate && uo_search_try_delegate_parallel_search(thread, &params))
    {
      ++parallel_search_count;
      continue;
    }

    uo_position_make_move(position, move);

    // search with null window
    int16_t node_value = -uo_search_principal_variation(thread, depth_lmr - 1, -alpha - 1, -alpha, line, false);
    node_value = uo_score_adjust_for_mate(node_value);

    if (node_value > alpha && node_value < beta)
    {
      // failed high, do a full re-search
      node_value = -uo_search_principal_variation(thread, depth_lmr - 1, -beta, -alpha, line, false);
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
        uo_position_update_history_heuristic(position, move, depth_lmr);

        if (entry.value >= beta)
        {
          uo_position_update_killers(position, move);

          if (parallel_search_count > 0)
          {
            uo_search_cutoff_parallel_search(thread, &params.queue);
          }

          return uo_engine_store_entry(position, &entry);
        }

        alpha = params.alpha = entry.value;
        pline[0] = entry.bestmove;
        memcpy(pline + 1, line, depth * sizeof * line);

        if (pv)
        {
          //uo_engine_store_entry(position, &entry);
          //uo_engine_thread_update_pv(thread);

          if (depth > 8 && position->ply == 0)
          {
            uo_search_print_info(thread);
          }
        }
      }
    }

    if (uo_engine_is_stopped())
    {
      if (parallel_search_count > 0)
      {
        uo_search_cutoff_parallel_search(thread, &params.queue);
      }

      return entry.value;
    }

    if (thread->owner && uo_atomic_load(&thread->cutoff))
    {
      if (parallel_search_count > 0)
      {
        uo_search_cutoff_parallel_search(thread, &params.queue);

        uo_search_queue_item result;

        while (uo_search_queue_try_get_result(&params.queue, &result))
        {
          thread->info.nodes += result.nodes;
        }
      }

      return entry.value;
    }

    if (parallel_search_count > 0)
    {
      uo_search_queue_item result;

      while (uo_search_queue_try_get_result(&params.queue, &result))
      {
        --parallel_search_count;

        for (size_t i = 0; i < UO_PARALLEL_MAX_COUNT; ++i)
        {
          if (params.queue.threads[i] == result.thread)
          {
            params.queue.threads[i] = NULL;
            break;
          }
        }

        size_t depth_lmr = result.depth;
        uo_move move = result.move;
        int16_t node_value = result.value;
        thread->info.nodes += result.nodes;

        if (node_value > entry.value)
        {
          entry.value = node_value;
          entry.bestmove = move;
          entry.depth = depth_lmr;

          if (entry.value > alpha)
          {
            uo_position_update_history_heuristic(position, move, depth_lmr);

            if (entry.value >= beta)
            {
              uo_position_update_killers(position, move);

              uo_search_cutoff_parallel_search(thread, &params.queue);

              while (uo_search_queue_try_get_result(&params.queue, &result))
              {
                thread->info.nodes += result.nodes;
              }

              return uo_engine_store_entry(position, &entry);
            }

            alpha = params.alpha = entry.value;
            pline[0] = entry.bestmove;
            pline[1] = 0;

            if (pv)
            {
              //uo_engine_store_entry(position, &entry);
              //uo_engine_thread_update_pv(thread);

              if (depth > 8 && position->ply == 0)
              {
                uo_search_print_info(thread);
              }
            }
          }
        }
      }
    }
  }

  if (parallel_search_count > 0)
  {
    uo_search_queue_item result;

    while (uo_search_queue_get_result(&params.queue, &result))
    {
      --parallel_search_count;

      for (size_t i = 0; i < UO_PARALLEL_MAX_COUNT; ++i)
      {
        if (params.queue.threads[i] == result.thread)
        {
          params.queue.threads[i] = NULL;
          break;
        }
      }

      size_t depth_lmr = result.depth;
      uo_move move = result.move;
      int16_t node_value = result.value;
      thread->info.nodes += result.nodes;

      if (node_value > entry.value)
      {
        entry.value = node_value;
        entry.bestmove = move;
        entry.depth = depth_lmr;

        if (entry.value > alpha)
        {
          uo_position_update_history_heuristic(position, move, depth_lmr);

          if (entry.value >= beta)
          {
            uo_position_update_killers(position, move);

            uo_search_cutoff_parallel_search(thread, &params.queue);

            while (uo_search_queue_try_get_result(&params.queue, &result))
            {
              thread->info.nodes += result.nodes;
            }

            return uo_engine_store_entry(position, &entry);
          }

          alpha = params.alpha = entry.value;
          pline[0] = entry.bestmove;
          pline[1] = 0;

          if (pv)
          {
            //uo_engine_store_entry(position, &entry);
            //uo_engine_thread_update_pv(thread);

            if (depth > 8 && position->ply == 0)
            {
              uo_search_print_info(thread);
            }
          }
        }
      }
    }
  }

  return uo_engine_store_entry(position, &entry);
}

static void uo_search_print_currmove(uo_engine_thread *thread, uo_move currmove, size_t currmovenumber)
{
  uo_position *position = &thread->position;

  uo_engine_lock_stdout();
  uo_position_print_move(position, currmove, buf);
  printf("info depth %d currmove %s currmovenumber %zu", thread->info.depth, buf, currmovenumber);
  uo_engine_unlock_stdout();
}

static inline void uo_engine_thread_load_position(uo_engine_thread *thread)
{
  uo_mutex_lock(engine.position_mutex);
  uo_position_copy(&thread->position, &engine.position);
  uo_mutex_unlock(engine.position_mutex);
}

void *uo_engine_thread_start_timer(void *arg)
{
  uo_engine_thread *thread = arg;
  uo_engine_thread *owner = thread->data;

  uo_atomic_unlock(&thread->busy);

  while (!uo_engine_is_stopped())
  {
    uo_sleep_msec(100);
    uo_search_stop_if_movetime_over(owner);
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

void *uo_engine_thread_run_parallel_principal_variation_search(void *arg)
{
  uo_engine_thread *thread = arg;

  uo_position *position = &thread->position;
  uo_parallel_search_params *params = thread->data;
  uo_engine_thread *owner = thread->owner = params->thread;
  uo_search_queue *queue = &params->queue;
  size_t depth = params->depth;
  int16_t alpha = params->alpha;
  int16_t beta = params->beta;
  uo_move move = params->move;

  uo_position_copy(&thread->position, &owner->position);
  uo_atomic_unlock(&thread->busy);

  uo_position_make_move(&thread->position, move);

  thread->info = (uo_search_info){
    .depth = depth,
    .multipv = engine_options.multipv,
    .nodes = 0
  };

  uo_move *line = uo_alloca(depth * sizeof * line);
  line[0] = 0;

  // search with null window
  int16_t value = -uo_search_principal_variation(thread, depth - 1, -alpha - 1, -alpha, line, false);
  value = uo_score_adjust_for_mate(value);

  if (value > alpha && value < beta)
  {
    // failed high, do a full re-search
    value = -uo_search_principal_variation(thread, depth - 1, -beta, -alpha, line, false);
    value = uo_score_adjust_for_mate(value);
  }

  uo_search_queue_item result = {
    .thread = thread,
    .nodes = thread->info.nodes,
    .depth = depth,
    .value = value,
    .move = move
  };

  uo_atomic_lock(&thread->busy);

  if (!uo_atomic_load(&thread->cutoff))
  {
    uo_search_queue_post_result(queue, &result);
  }

  thread->owner = NULL;
  uo_atomic_unlock(&thread->busy);

  uo_atomic_store(&thread->cutoff, 0);

  return NULL;
}

void *uo_engine_thread_run_principal_variation_search(void *arg)
{
  uo_engine_thread *thread = arg;
  uo_position *position = &thread->position;
  uo_search_params *params = &engine.search_params;

  uo_atomic_unlock(&thread->busy);

  thread->info = (uo_search_info){
    .depth = 1,
    .multipv = engine_options.multipv,
    .nodes = 0
  };

  uo_move *line = thread->info.pv;

  uo_time_now(&thread->info.time_start);
  uo_engine_thread_load_position(thread);
  uo_engine_run_thread(uo_engine_thread_start_timer, thread);

  int16_t alpha = params->alpha;
  int16_t beta = params->beta;
  size_t aspiration_fail_count = 0;

  int16_t value = uo_search_principal_variation(thread, 1, alpha, beta, line, true);
  uo_search_adjust_alpha_beta(value, &alpha, &beta, &aspiration_fail_count);
  thread->info.value = value;

  for (size_t depth = 2; depth <= params->depth; ++depth)
  {
    thread->info.depth = depth;

    if (thread->info.nodes)
    {
      uo_search_print_info(thread);
    }

    thread->info.nodes = 0;

    while (!uo_engine_is_stopped())
    {
      value = uo_search_principal_variation(thread, depth, alpha, beta, line, true);

      if (uo_search_adjust_alpha_beta(value, &alpha, &beta, &aspiration_fail_count))
      {
        thread->info.value = value;
        break;
      }
    }

    if (uo_engine_is_stopped()) break;
  }

  thread->info.completed = true;
  uo_search_print_info(thread);

  uo_engine_lock_position();
  for (size_t i = 0; i < 4096; ++i)
  {
    engine.position.hhtable[0][i] = position->hhtable[0][i] >> 1;
    engine.position.bftable[0][i] = position->bftable[0][i] >> 1;
  }
  uo_engine_unlock_position();

  //uo_engine_thread_release_pv(thread);
  uo_engine_stop_search();

  return NULL;
}

void *uo_engine_thread_run_quiescence_search(void *arg)
{
  uo_engine_thread *thread = arg;
  uo_position *position = &thread->position;
  uo_search_params *params = &engine.search_params;

  uo_atomic_unlock(&thread->busy);

  thread->info = (uo_search_info){
    .depth = 0,
    .multipv = engine_options.multipv,
    .nodes = 0
  };

  uo_time_now(&thread->info.time_start);
  uo_engine_thread_load_position(thread);
  uo_engine_run_thread(uo_engine_thread_start_timer, thread);

  int16_t alpha = params->alpha;
  int16_t beta = params->beta;
  int16_t value = uo_search_quiesce(thread, alpha, beta, 0);

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
