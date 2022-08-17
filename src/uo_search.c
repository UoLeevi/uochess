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
  int64_t movetime = params->movetime;

  // TODO: make better decisions about how much time to use
  if (!movetime && params->time_own)
  {
    int64_t movetime_max = uo_min(params->time_own / 8, 5000 + (params->time_own + params->time_inc_own - params->time_enemy) / 2);
    int64_t bestmove_age_reduction = movetime_max / 12;
    int16_t bestmove_age = info->depth - info->bestmove_change_depth;
    movetime = uo_max(movetime_max - bestmove_age_reduction * bestmove_age, 1);
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

  size_t tentry_count = uo_atomic_load(&engine.ttable.count);
  uint64_t hashfull = (tentry_count * 1000) / (engine.ttable.hash_mask + 1);

  uo_engine_lock_stdout();

  for (int i = 0; i < info->multipv; ++i)
  {
    printf("info depth %d ", info->depth);
    if (info->seldepth) printf("seldepth %d ", info->seldepth);
    printf("multipv %d ", info->multipv);

    int16_t score = info->value;

    if (score > uo_score_mate_in_threshold)
    {
      printf("score mate %d ", (uo_score_checkmate - score + 1) >> 1);
    }
    else if (score < -uo_score_mate_in_threshold)
    {
      printf("score mate %d ", -((uo_score_checkmate + score + 1) >> 1));
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
      printf("bestmove %s", buf);

      if (info->pv[1])
      {
        uo_position_print_move(position, info->pv[1] ^ 0xE38, buf);
        printf(" ponder %s", buf);
      }

      printf("\n");
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
  size_t material_weighted_depth = depth * material_percentage / 100;

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
//
//static int16_t uo_search_quiesce(uo_engine_thread *thread, int16_t alpha, int16_t beta, uint8_t depth)
//{
//  uo_search_info *info = &thread->info;
//  uo_position *position = &thread->position;
//
//  if (info->seldepth < position->ply)
//  {
//    info->seldepth = position->ply;
//  }
//
//  if (uo_position_is_rule50_draw(position) || uo_position_is_repetition_draw(position))
//  {
//    return 0;
//  }
//
//  ++info->nodes;
//
//  if (uo_position_is_max_depth_reached(position))
//  {
//    return uo_position_evaluate(position);
//  }
//
//  size_t move_count = uo_position_generate_moves(position);
//  bool is_check = uo_position_is_check(position);
//
//  if (move_count == 0)
//  {
//    return is_check ? -UO_SCORE_CHECKMATE : 0;
//  }
//
//  if (is_check)
//  {
//    int16_t value = -UO_SCORE_CHECKMATE;
//    uo_position_sort_moves(&thread->position, 0);
//
//    for (size_t i = 0; i < move_count; ++i)
//    {
//      uo_move move = position->movelist.head[i];
//      uo_position_make_move(position, move);
//      int16_t node_value = -uo_search_quiesce(thread, -beta, -alpha, depth + 1);
//      node_value = uo_score_adjust_for_mate(node_value);
//      uo_position_unmake_move(position);
//
//      if (node_value > value)
//      {
//        value = node_value;
//
//        if (value > alpha)
//        {
//          if (value >= beta)
//          {
//            return value;
//          }
//
//          alpha = value;
//        }
//      }
//
//      if (uo_engine_is_stopped())
//      {
//        return value;
//      }
//
//      if (thread->owner && uo_atomic_load(&thread->cutoff))
//      {
//        return value;
//      }
//    }
//
//    return value;
//  }
//
//  int16_t value = uo_position_evaluate(position);
//
//  if (value >= beta)
//  {
//    return value;
//  }
//
//  // delta pruning
//  if (position->Q)
//  {
//    int16_t delta = 600;
//    if (alpha > value + delta)
//    {
//      return alpha;
//    }
//  }
//
//  if (value > alpha)
//  {
//    alpha = value;
//  }
//
//  uo_search_quiesce_flags flags = uo_search_quiesce_determine_flags(thread, depth);
//  uo_position_sort_tactical_moves(&thread->position);
//
//  for (size_t i = 0; i < move_count; ++i)
//  {
//    uo_move move = position->movelist.head[i];
//
//    if (uo_search_quiesce_should_examine_move(thread, move, flags))
//    {
//      uo_position_make_move(position, move);
//      int16_t node_value = -uo_search_quiesce(thread, -beta, -alpha, depth + 1);
//      node_value = uo_score_adjust_for_mate(node_value);
//      uo_position_unmake_move(position);
//
//      if (node_value > value)
//      {
//        value = node_value;
//
//        if (value > alpha)
//        {
//          if (value >= beta)
//          {
//            return value;
//          }
//
//          alpha = value;
//        }
//      }
//
//      if (uo_engine_is_stopped())
//      {
//        return value;
//      }
//
//      if (thread->owner && uo_atomic_load(&thread->cutoff))
//      {
//        return value;
//      }
//    }
//  }
//
//  return value;
//}

static bool uo_search_quiesce(uo_engine_thread *thread, uo_alphabeta *entry)
{
  uo_search_info *info = &thread->info;
  uo_position *position = &thread->position;

  // Step 1. Update searched node count and selective search depth information
  ++info->nodes;
  info->seldepth = uo_max(info->seldepth, position->ply);

  // Next two steps can be skipped on the first quisence search node
  if (entry->depth > 0)
  {
    // Step 2. Check for draw by 50 move rule or threefold repetition
    if (uo_position_is_rule50_draw(position) || uo_position_is_repetition_draw(position))
    {
      entry->value = 0;
      entry->type = uo_alphabeta_type__draw;
      return true;
    }

    // Step 3. If search is stopped, return unknown value
    if (uo_engine_thread_is_stopped(thread))
    {
      entry->value = 0;
      entry->type = uo_alphabeta_type__incomplete;
      return false;
    }
  }

  // Step 4. If maximum search depth is reached return static evaluation
  if (uo_position_is_max_depth_reached(position))
  {
    entry->value = uo_position_evaluate(position);
    entry->type = uo_alphabeta_type__exact;
    return true;
  }

  // Step 5. Generate legal moves
  size_t move_count = position->stack->moves_generated
    ? position->stack->move_count
    : uo_position_generate_moves(position);

  // Step 6. Determine if position is check
  bool is_check = uo_position_is_check(position);

  // Step 7. If there are no legal moves, return draw or checkmate
  if (move_count == 0)
  {
    entry->value = uo_position_is_check(position) ? -uo_score_checkmate : 0;
    entry->type = uo_alphabeta_type__exact;
    return true;
  }

  // Step 8. Initialize child node entry
  uo_alphabeta move_entry = {
    .alpha = -entry->beta,
    .beta = -entry->alpha,
    .depth = entry->depth + 1
  };

  // Step 9. If position is check, perform quiesence search for all moves
  if (is_check)
  {
    // Step 9.1. Initialize score to be checkmate
    entry->value = -uo_score_checkmate;

    // Step 9.2. Sort moves
    uo_position_sort_moves(&thread->position, 0);

    // Step 9.3. Search each move
    for (size_t i = 0; i < move_count; ++i)
    {
      uo_move move = position->movelist.head[i];
      uo_position_make_move(position, move);
      bool completed = uo_search_quiesce(thread, &move_entry);
      uo_position_unmake_move(position);

      if (!completed)
      {
        entry->type = uo_alphabeta_type__incomplete;
        return false;
      }

      move_entry.value = uo_score_adjust_for_mate(move_entry.value);

      if (-move_entry.value > entry->value)
      {
        entry->value = -move_entry.value;
        if (entry->value > entry->alpha)
        {
          if (entry->value >= entry->beta)
          {
            entry->type = uo_alphabeta_type__lower_bound;
            return true;
          }

          entry->type = uo_alphabeta_type__exact;
          entry->alpha = entry->value;
        }
      }
    }

    return true;
  }

  // Step 10. Position is not check. Initialize score to static evaluation. "Stand pat"
  entry->value = uo_position_evaluate(position);
  entry->type = uo_alphabeta_type__lower_bound;

  // Step 11. Cutoff if static evaluation is higher or equal to beta.
  if (entry->value >= entry->beta)
  {
    return true;
  }

  // Step 12. Delta pruning
  if (position->Q)
  {
    int16_t delta = uo_piece_value(uo_piece__Q);
    if (entry->alpha > entry->value + delta)
    {
      return true;
    }
  }

  // Step 13. Update alpha
  entry->alpha = uo_max(entry->value, entry->alpha);

  // Step 14. Determie which moves should be searched
  uo_search_quiesce_flags flags = uo_search_quiesce_determine_flags(thread, entry->depth);

  // Step 15. Sort tactical moves
  uo_position_sort_tactical_moves(&thread->position);

  // Step 16. Search interesting tactical moves and checks
  for (size_t i = 0; i < move_count; ++i)
  {
    uo_move move = position->movelist.head[i];

    if (uo_search_quiesce_should_examine_move(thread, move, flags))
    {
      uo_position_make_move(position, move);
      bool completed = uo_search_quiesce(thread, &move_entry);
      uo_position_unmake_move(position);

      if (!completed)
      {
        entry->type = uo_alphabeta_type__incomplete;
        return false;
      }

      move_entry.value = uo_score_adjust_for_mate(move_entry.value);

      if (-move_entry.value > entry->value)
      {
        entry->value = -move_entry.value;
        if (entry->value > entry->alpha)
        {
          if (entry->value >= entry->beta)
          {
            entry->type = uo_alphabeta_type__lower_bound;
            return true;
          }

          entry->type = uo_alphabeta_type__exact;
          entry->alpha = entry->value;
        }
      }
    }
  }

  return true;
}

static inline void uo_search_cutoff_parallel_search(uo_engine_thread *thread, uo_search_queue *queue)
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

static inline bool uo_search_try_delegate_parallel_search(uo_parallel_search_params *params)
{
  return uo_search_queue_try_enqueue(&params->queue, uo_engine_thread_run_parallel_principal_variation_search, params);
}

#define uo_allocate_line(depth) uo_alloca(((depth) + 2) * sizeof(uo_move))

static inline void uo_pv_update(uo_move *pline, uo_move bestmove, uo_move *line, size_t move_count)
{
  pline[0] = bestmove;
  pline[move_count + 1] = 0;

  if (move_count)
  {
    memcpy(pline + 1, line, move_count * sizeof * line);
  }
}
//
//// see: https://en.wikipedia.org/wiki/Negamax#Negamax_with_alpha_beta_pruning_and_transposition_tables
//// see: https://en.wikipedia.org/wiki/Principal_variation_search
//static int16_t uo_search_principal_variation(uo_engine_thread *thread, size_t depth, int16_t alpha, int16_t beta, uo_move *pline, bool pv)
//{
//  uo_position *position = &thread->position;
//
//  if (uo_position_is_rule50_draw(position) || uo_position_is_repetition_draw(position))
//  {
//    return 0;
//  }
//
//  uo_abtentry entry = { alpha, beta, depth };
//  if (uo_engine_lookup_entry(position, &entry))
//  {
//    //if (pv && entry.value > alpha)
//    //{
//    //  uo_engine_thread_update_pv(thread);
//    //}
//
//    uo_pv_update(pline, entry.bestmove, NULL, 0);
//    return entry.value;
//  }
//
//  if (thread->owner && uo_atomic_load(&thread->cutoff))
//  {
//    return entry.value;
//  }
//
//  ++thread->info.nodes;
//
//  if (depth == 0)
//  {
//    entry.value = uo_search_quiesce(thread, alpha, beta, 0);
//    return uo_engine_store_entry(position, &entry);
//  }
//
//  if (uo_position_is_max_depth_reached(position))
//  {
//    entry.value = uo_position_evaluate(position);
//    return uo_engine_store_entry(position, &entry);
//  }
//
//  size_t move_count = position->stack->moves_generated
//    ? position->stack->move_count
//    : uo_position_generate_moves(position);
//
//  if (move_count == 0)
//  {
//    entry.value = uo_position_is_check(position) ? -UO_SCORE_CHECKMATE : 0;
//    return uo_engine_store_entry(position, &entry);
//  }
//
//  uo_move *line = uo_allocate_line(depth);
//  line[0] = 0;
//
//  // Null move pruning
//  if (!pv
//    && depth > 3
//    && position->ply > 1
//    && uo_position_is_null_move_allowed(position)
//    && uo_position_evaluate(position) > beta)
//  {
//    uo_position_make_null_move(position);
//    // depth * 3/4 - 1
//    size_t depth_nmp = (depth * 3 >> 2) - 1;
//    int16_t pass_value = -uo_search_principal_variation(thread, depth_nmp, -beta, -beta + 1, line, false);
//    uo_position_unmake_null_move(position);
//
//    if (pass_value >= beta)
//    {
//      return pass_value;
//    }
//  }
//
//  uo_position_sort_moves(&thread->position, pline[0] ? pline[0] : entry.bestmove);
//
//  entry.bestmove = position->movelist.head[0];
//  entry.depth = depth;
//
//  uo_position_update_butterfly_heuristic(position, entry.bestmove);
//
//  uo_position_make_move(position, entry.bestmove);
//  // search first move with full alpha/beta window
//  entry.value = -uo_search_principal_variation(thread, depth - 1, -beta, -alpha, pline + 1, pv);
//  entry.value = uo_score_adjust_for_mate(entry.value);
//  uo_position_unmake_move(position);
//
//  if (uo_engine_is_stopped())
//  {
//    return uo_engine_store_entry(position, &entry);
//  }
//
//  if (entry.value > alpha)
//  {
//    uo_position_update_history_heuristic(position, entry.bestmove, depth);
//
//    if (entry.value >= beta)
//    {
//      uo_position_update_killers(position, entry.bestmove);
//      return uo_engine_store_entry(position, &entry);
//    }
//
//    alpha = entry.value;
//    uo_pv_update(pline, entry.bestmove, NULL, 0);
//
//    //if (pv)
//    //{
//    //  uo_engine_store_entry(position, &entry);
//    //  uo_engine_thread_update_pv(thread);
//    //}
//  }
//
//  if (uo_engine_is_stopped())
//  {
//    return uo_engine_store_entry(position, &entry);
//  }
//
//  if (thread->owner && uo_atomic_load(&thread->cutoff))
//  {
//    return uo_engine_store_entry(position, &entry);
//  }
//
//  size_t parallel_search_count = 0;
//  uo_parallel_search_params params = {
//    .thread = thread,
//    .queue = {.init = 0 },
//    .alpha = -beta,
//    .beta = -alpha,
//    .depth = depth - 1
//  };
//
//  for (size_t i = 1; i < move_count; ++i)
//  {
//    uo_move move = params.move = position->movelist.head[i];
//    uo_position_update_butterfly_heuristic(position, move);
//
//    // search later moves for reduced depth
//    // see: https://en.wikipedia.org/wiki/Late_move_reductions
//    size_t depth_reduction = depth > 3 && !pv && !uo_position_is_check(position) ? 1 : 0;
//
//    if (depth_reduction)
//    {
//      if (uo_move_is_capture(move) && uo_position_move_sse(position, move) < 0)
//      {
//        depth_reduction += depth > 4 ? 2 : 1;
//      }
//
//      uo_bitboard checks = uo_position_move_checks(position, move);
//
//      if (checks)
//      {
//        uo_position_update_next_move_checks(position, checks);
//        --depth_reduction;
//      }
//    }
//
//    uo_position_make_move(position, move);
//
//    size_t depth_lmr = depth - depth_reduction;
//
//    // search with null window for reduced depth
//    int16_t node_value = -uo_search_principal_variation(thread, depth_lmr - 1, -alpha - 1, -alpha, line, false);
//    node_value = uo_score_adjust_for_mate(node_value);
//
//    if (node_value > alpha && node_value < beta)
//    {
//      // failed high, do a full re-search
//      bool can_delegate = (depth >= UO_PARALLEL_MIN_DEPTH) && (move_count - i > UO_PARALLEL_MIN_MOVE_COUNT) && (parallel_search_count < UO_PARALLEL_MAX_COUNT);
//
//      if (can_delegate && uo_search_try_delegate_parallel_search(&params))
//      {
//        uo_position_unmake_move(position);
//        ++parallel_search_count;
//        continue;
//      }
//
//      depth_lmr = depth;
//      node_value = -uo_search_principal_variation(thread, depth - 1, -beta, -alpha, line, false);
//      node_value = uo_score_adjust_for_mate(node_value);
//    }
//
//    uo_position_unmake_move(position);
//
//    if (node_value > entry.value)
//    {
//      entry.value = node_value;
//      entry.bestmove = move;
//      entry.depth = depth_lmr;
//
//      if (entry.value > alpha)
//      {
//        uo_position_update_history_heuristic(position, move, depth_lmr);
//
//        if (entry.value >= beta)
//        {
//          uo_position_update_killers(position, move);
//
//          if (parallel_search_count > 0)
//          {
//            uo_search_cutoff_parallel_search(thread, &params.queue);
//
//            uo_search_queue_item result;
//            while (uo_search_queue_get_result(&params.queue, &result))
//            {
//              thread->info.nodes += result.nodes;
//              node_value = -result.value;
//              if (result.move && node_value > entry.value)
//              {
//                entry.value = node_value;
//                entry.bestmove = result.move;
//              }
//            }
//          }
//
//          return uo_engine_store_entry(position, &entry);
//        }
//
//        alpha = entry.value;
//        params.beta = -alpha;
//        uo_pv_update(pline, entry.bestmove, line, depth_lmr);
//
//        if (pv && position->ply == 0 && thread->owner == NULL)
//        {
//          thread->info.bestmove_change_depth = depth_lmr;
//
//          if (depth_lmr > 8)
//          {
//            uo_search_print_info(thread);
//          }
//        }
//      }
//    }
//
//    if (uo_engine_is_stopped())
//    {
//      if (parallel_search_count > 0)
//      {
//        uo_search_cutoff_parallel_search(thread, &params.queue);
//        uo_search_queue_item result;
//        while (uo_search_queue_get_result(&params.queue, &result))
//        {
//          thread->info.nodes += result.nodes;
//          node_value = -result.value;
//          if (result.move && node_value > entry.value)
//          {
//            entry.value = node_value;
//            entry.bestmove = result.move;
//          }
//        }
//      }
//
//      return uo_engine_store_entry(position, &entry);
//    }
//
//    if (thread->owner && uo_atomic_load(&thread->cutoff))
//    {
//      if (parallel_search_count > 0)
//      {
//        uo_search_cutoff_parallel_search(thread, &params.queue);
//        uo_search_queue_item result;
//        while (uo_search_queue_get_result(&params.queue, &result))
//        {
//          thread->info.nodes += result.nodes;
//          node_value = -result.value;
//          if (result.move && node_value > entry.value)
//          {
//            entry.value = node_value;
//            entry.bestmove = result.move;
//          }
//        }
//      }
//
//      return uo_engine_store_entry(position, &entry);
//    }
//
//    if (parallel_search_count > 0)
//    {
//      uo_search_queue_item result;
//
//      while (uo_search_queue_try_get_result(&params.queue, &result))
//      {
//        --parallel_search_count;
//
//        for (size_t i = 0; i < UO_PARALLEL_MAX_COUNT; ++i)
//        {
//          if (params.queue.threads[i] == result.thread)
//          {
//            params.queue.threads[i] = NULL;
//            break;
//          }
//        }
//
//        size_t depth_lmr = result.depth;
//        uo_move move = result.move;
//        int16_t node_value = -result.value;
//        thread->info.nodes += result.nodes;
//
//        if (node_value > entry.value)
//        {
//          entry.value = node_value;
//          entry.bestmove = move;
//          entry.depth = depth_lmr;
//
//          if (entry.value > alpha)
//          {
//            uo_position_update_history_heuristic(position, move, depth_lmr);
//
//            if (entry.value >= beta)
//            {
//              uo_position_update_killers(position, move);
//
//              uo_search_cutoff_parallel_search(thread, &params.queue);
//              uo_search_queue_item result;
//              while (uo_search_queue_get_result(&params.queue, &result))
//              {
//                thread->info.nodes += result.nodes;
//                node_value = -result.value;
//                if (result.move && node_value > entry.value)
//                {
//                  entry.value = node_value;
//                  entry.bestmove = result.move;
//                }
//              }
//
//              return uo_engine_store_entry(position, &entry);
//            }
//
//            alpha = entry.value;
//            params.beta = -alpha;
//            uo_pv_update(pline, entry.bestmove, NULL, 0);
//
//            if (pv && position->ply == 0 && thread->owner == NULL)
//            {
//              thread->info.bestmove_change_depth = depth_lmr;
//
//              if (depth_lmr > 8)
//              {
//                uo_search_print_info(thread);
//              }
//            }
//          }
//        }
//      }
//    }
//  }
//
//  if (parallel_search_count > 0)
//  {
//    uo_search_queue_item result;
//
//    while (uo_search_queue_get_result(&params.queue, &result))
//    {
//      --parallel_search_count;
//
//      size_t depth_lmr = result.depth;
//      uo_move move = result.move;
//      int16_t node_value = -result.value;
//      thread->info.nodes += result.nodes;
//
//      if (node_value > entry.value)
//      {
//        entry.value = node_value;
//        entry.bestmove = move;
//        entry.depth = depth_lmr;
//
//        if (entry.value > alpha)
//        {
//          uo_position_update_history_heuristic(position, move, depth_lmr);
//
//          if (entry.value >= beta)
//          {
//            uo_position_update_killers(position, move);
//
//            uo_search_cutoff_parallel_search(thread, &params.queue);
//            uo_search_queue_item result;
//            while (uo_search_queue_get_result(&params.queue, &result))
//            {
//              thread->info.nodes += result.nodes;
//              node_value = -result.value;
//              if (result.move && node_value > entry.value)
//              {
//                entry.value = node_value;
//                entry.bestmove = result.move;
//              }
//            }
//            return uo_engine_store_entry(position, &entry);
//          }
//
//          alpha = entry.value;
//          params.beta = -alpha;
//          pline[0] = entry.bestmove;
//          pline[1] = 0;
//
//          if (pv && position->ply == 0 && thread->owner == NULL)
//          {
//            thread->info.bestmove_change_depth = depth_lmr;
//
//            if (depth_lmr > 8)
//            {
//              uo_search_print_info(thread);
//            }
//          }
//        }
//      }
//    }
//  }
//
//  return uo_engine_store_entry(position, &entry);
//}

static bool uo_search_principal_variation(uo_engine_thread *thread, uo_alphabeta *entry)
{
  uo_search_info *info = &thread->info;
  uo_position *position = &thread->position;

  // Step 1. Check for draw by 50 move rule or threefold repetition
  if (uo_position_is_rule50_draw(position) || uo_position_is_repetition_draw(position))
  {
    ++info->nodes;
    entry->value = 0;
    entry->type = uo_alphabeta_type__draw;
    return true;
  }

  // Step 2. Lookup position from transposition table and return if exact score for equal or higher depth is found
  if (uo_engine_lookup_entry(position, entry))
  {
    return true;
  }

  // Step 3. If search is stopped, return unknown value
  if (uo_engine_thread_is_stopped(thread))
  {
    entry->value = 0;
    entry->type = uo_alphabeta_type__incomplete;
    return false;
  }

  // Step 4. If specified search depth is reached, perform quiescence search and store and return evaluation if search was completed
  if (entry->depth == 0)
  {
    bool completed = uo_search_quiesce(thread, entry);
    if (!completed)
    {
      entry->type = uo_alphabeta_type__incomplete;
      return false;
    }

    uo_engine_store_entry(position, entry);
    return true;
  }

  // Step 5. Increment searched node count
  ++info->nodes;

  // Step 6. If maximum search depth is reached return static evaluation
  if (uo_position_is_max_depth_reached(position))
  {
    entry->value = uo_position_evaluate(position);
    entry->type = uo_alphabeta_type__exact;
    uo_engine_store_entry(position, entry);
    return true;
  }

  // Step 7. Generate legal moves
  size_t move_count = position->stack->moves_generated
    ? position->stack->move_count
    : uo_position_generate_moves(position);

  // Step 8. If there are no legal moves, return draw or checkmate
  if (move_count == 0)
  {
    entry->value = uo_position_is_check(position) ? -uo_score_checkmate : 0;
    entry->type = uo_alphabeta_type__exact;
    uo_engine_store_entry(position, entry);
    return true;
  }

  // Step 9. Allocate search entry and principal variation line on the stack
  uo_move *line = uo_allocate_line(entry->depth);
  line[0] = 0;

  uo_alphabeta move_entry = { .line = line };

  // Step 10. Null move pruning
  if (!entry->pv
    && entry->depth > 3
    && position->ply > 1
    && uo_position_is_null_move_allowed(position)
    && uo_position_evaluate(position) > entry->beta)
  {
    // depth * 3/4 - 1
    move_entry.depth = (entry->depth * 3 >> 2) - 1;
    move_entry.alpha = -entry->beta;
    move_entry.beta = -entry->beta + 1;

    uo_position_make_null_move(position);
    bool completed = uo_search_principal_variation(thread, &move_entry);
    uo_position_unmake_null_move(position);

    if (!completed)
    {
      entry->type = uo_alphabeta_type__incomplete;
      return false;
    }

    if (-move_entry.value > entry->beta)
    {
      entry->value = -move_entry.value;
      entry->type = uo_alphabeta_type__lower_bound;
      uo_engine_store_entry(position, entry);
      return true;
    }
  }

  // Step 11. Sort moves and place pv move or transposition table move as first
  uo_move move = entry->line[0] ? entry->line[0] : entry->ttmove;
  uo_position_sort_moves(&thread->position, move);


  // Step 12. Perform full alpha-beta search for the first move
  move = position->movelist.head[0];
  move_entry.depth = entry->depth - 1;
  move_entry.alpha = -entry->beta;
  move_entry.beta = -entry->alpha;
  move_entry.pv = entry->pv;

  // set the default type for entry as upper bound
  entry->type = uo_alphabeta_type__upper_bound;

  uo_position_make_move(position, move);
  bool completed = uo_search_principal_variation(thread, &move_entry);
  uo_position_unmake_move(position);

  if (!completed)
  {
    entry->type = uo_alphabeta_type__incomplete;
    return false;
  }

  entry->value = uo_score_adjust_for_mate(-move_entry.value);
  uo_position_update_butterfly_heuristic(position, move);

  if (entry->value > entry->alpha)
  {
    uo_position_update_history_heuristic(position, move, move_entry.depth);

    if (entry->value >= entry->beta)
    {
      entry->type = uo_alphabeta_type__lower_bound;
      uo_engine_store_entry(position, entry);
      return true;
    }

    entry->type = uo_alphabeta_type__exact;
    entry->alpha = entry->value;
    uo_pv_update(entry->line, move, line, move_entry.depth);
  }

  // 13. Initialize parallel search parameters and result queue
  size_t parallel_search_count = 0;
  uo_parallel_search_params params = {
    .thread = thread,
    .queue = {.init = 0 },
    .alpha = -entry->beta,
    .beta = -entry->alpha
  };

  // 14. Search rest of the moves with null window and reduced depth unless they fail-high

  move_entry.pv = false;

  for (size_t i = 1; i < move_count; ++i)
  {
    uo_move move = params.move = position->movelist.head[i];

    // 14.1 Determine search depth reduction
    // see: https://en.wikipedia.org/wiki/Late_move_reductions

    size_t depth_reduction =
      // no reduction for shallow depth
      entry->depth <= 3
      // no reduction for expected pv nodes
      || entry->pv
      // no reduction if position is check
      || uo_position_is_check(position)
      ? 0 // no reduction
      : 1; // default reduction is one ply

    if (depth_reduction)
    {
      // increase reduction for captures with negative SSE
      if (uo_move_is_capture(move) && uo_position_move_sse(position, move) < 0)
      {
        depth_reduction += entry->depth > 4 ? 2 : 1;
      }

      // decrease reduction if move gives a check
      uo_bitboard checks = uo_position_move_checks(position, move);
      if (checks)
      {
        uo_position_update_next_move_checks(position, checks);
        --depth_reduction;
      }
    }

    // 14.2 Perform null window search for reduced depth
    move_entry.depth = entry->depth - 1 - depth_reduction;
    move_entry.alpha = -entry->alpha - 1;
    move_entry.beta = -entry->alpha;

    uo_position_make_move(position, move);
    bool completed = uo_search_principal_variation(thread, &move_entry);

    if (!completed)
    {
      uo_position_unmake_move(position);
      entry->type = uo_alphabeta_type__incomplete;
      return false;
    }

    move_entry.value = uo_score_adjust_for_mate(move_entry.value);

    // 14.3 If move failed high, perform full re-search
    if (-move_entry.value > entry->alpha && -move_entry.value < entry->beta)
    {
      move_entry.depth = entry->depth - 1;
      move_entry.alpha = -entry->beta;
      move_entry.beta = -entry->alpha;

      bool completed = uo_search_principal_variation(thread, &move_entry);

      if (!completed)
      {
        uo_position_unmake_move(position);
        entry->type = uo_alphabeta_type__incomplete;
        return false;
      }

      move_entry.value = uo_score_adjust_for_mate(move_entry.value);
    }

    uo_position_unmake_move(position);

    uo_position_update_butterfly_heuristic(position, move);

    if (-move_entry.value > entry->value)
    {
      entry->value = -move_entry.value;
      if (entry->value > entry->alpha)
      {
        uo_position_update_history_heuristic(position, move, move_entry.depth);

        if (entry->value >= entry->beta)
        {
          entry->type = uo_alphabeta_type__lower_bound;
          uo_engine_store_entry(position, entry);
          return true;
        }

        entry->type = uo_alphabeta_type__exact;
        entry->alpha = entry->value;
        uo_pv_update(entry->line, move, line, move_entry.depth);
      }
    }
  }

  uo_engine_store_entry(position, entry);
  return true;
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
  const int16_t aspiration_window_mate = (uo_score_checkmate - 1) / aspiration_window_factor;

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
      *alpha = -uo_score_checkmate;
      return false;
    }

    if (value < -aspiration_window_mate)
    {
      *alpha = -uo_score_checkmate;
      return false;
    }

    *alpha = value - aspiration_window;
    return false;
  }

  if (value >= *beta)
  {
    if ((*fail_count)++ >= aspiration_window_fail_threshold)
    {
      *beta = uo_score_checkmate;
      return false;
    }

    if (value > aspiration_window_mate)
    {
      *beta = uo_score_checkmate;
      return false;
    }

    *beta = value + aspiration_window;
    return false;
  }

  *fail_count = 0;

  if (value < -aspiration_window_mate)
  {
    *alpha = -uo_score_checkmate;
    *beta = -aspiration_window_mate;
    return true;
  }

  if (value > aspiration_window_mate)
  {
    *alpha = aspiration_window_mate;
    *beta = uo_score_checkmate;
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
  uo_move move = params->move;

  uo_alphabeta entry = {
    .alpha = params->alpha,
    .beta = params->beta,
    .line = thread->info.pv,
    .depth = params->depth,
    .pv = true
  };

  uo_position_copy(&thread->position, &owner->position);
  uo_atomic_unlock(&thread->busy);

  thread->info = (uo_search_info){
    .depth = entry.depth,
    .multipv = engine_options.multipv,
    .nodes = 0
  };

  uo_move *line = entry.line = uo_allocate_line(entry.depth);
  line[0] = 0;

  bool completed = uo_search_principal_variation(thread, &entry);
  entry.value = uo_score_adjust_for_mate(entry.value);

  uo_search_queue_item result = {
    .thread = thread,
    .nodes = thread->info.nodes,
    .depth = entry.depth,
    .value = entry.value,
    .move = move ? move : line[0]
  };

  uo_search_queue_post_result(queue, &result);

  uo_atomic_lock(&thread->busy);
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

  uo_time_now(&thread->info.time_start);
  uo_engine_thread_load_position(thread);
  uo_engine_run_thread(uo_engine_thread_start_timer, thread);

  uo_alphabeta entry = {
    .alpha = params->alpha,
    .beta = params->beta,
    .line = thread->info.pv,
    .depth = 1,
    .pv = true
  };

  size_t aspiration_fail_count = 0;

  uo_move bestmove = 0;

  if (engine.ponder.key)
  {
    entry.value = engine.ponder.value;

    if (position->stack[-1].key == engine.ponder.key)
    {
      int16_t window = (position->stack[-1].move == engine.ponder.move) ? 40 : 200;

      entry.alpha = entry.value > -uo_score_checkmate + window ? entry.value - window : -uo_score_checkmate;
      entry.beta = entry.value < uo_score_checkmate - window ? entry.value - window : -uo_score_checkmate;
    }
  }

  uo_search_principal_variation(thread, &entry);
  uo_search_adjust_alpha_beta(entry.value, &entry.alpha, &entry.beta, &aspiration_fail_count);
  bestmove = thread->info.bestmove;

  if (bestmove)
  {
    uo_position_make_move(position, bestmove);
    engine.ponder.key = position->key;
    engine.ponder.value = thread->info.value = entry.value;
    engine.ponder.move = thread->info.pv[1];
    uo_position_unmake_move(position);
  }
  else
  {
    engine.ponder.key = 0;
  }

  size_t lazy_smp_count = 0;
  size_t lazy_smp_max_count = uo_min(UO_PARALLEL_MAX_COUNT, engine.thread_count - 1 - UO_LAZY_SMP_FREE_THREAD_COUNT);
  uo_parallel_search_params lazy_smp_params = {
    .thread = thread,
    .queue = {.init = 0 },
    .alpha = entry.alpha,
    .beta = entry.beta
  };

  for (size_t depth = 2; depth <= params->depth; ++depth)
  {
    thread->info.depth = lazy_smp_params.depth = entry.depth = depth;

    if (thread->info.nodes)
    {
      uo_search_print_info(thread);
    }

    thread->info.nodes = 0;

    while (!uo_engine_is_stopped())
    {
      bool can_delegate = (depth >= UO_LAZY_SMP_MIN_DEPTH) && (lazy_smp_count < lazy_smp_max_count);

      while (can_delegate && uo_search_try_delegate_parallel_search(&lazy_smp_params))
      {
        can_delegate = ++lazy_smp_count < lazy_smp_max_count;
      }

      bool completed = uo_search_principal_variation(thread, &entry);

      if (lazy_smp_count > 0)
      {
        lazy_smp_count = 0;
        uo_search_cutoff_parallel_search(thread, &lazy_smp_params.queue);
        uo_search_queue_item result;
        while (uo_search_queue_get_result(&lazy_smp_params.queue, &result))
        {
          thread->info.nodes += result.nodes;
          //if (result.move && result.value > value)
          //{
          //  value = result.value;
          //  entry.line[0] = result.move;
          //  entry.line[1] = 0;
          //}
        }
      }

      if (!completed) break;

      if (uo_search_adjust_alpha_beta(entry.value, &entry.alpha, &entry.beta, &aspiration_fail_count))
      {
        bestmove = thread->info.bestmove;

        if (bestmove)
        {
          uo_position_make_move(position, bestmove);
          engine.ponder.key = position->key;
          engine.ponder.value = thread->info.value = entry.value;
          engine.ponder.move = thread->info.pv[1];
          uo_position_unmake_move(position);
        }
        else
        {
          engine.ponder.key = 0;
        }

        break;
      }
    }

    if (uo_engine_is_stopped()) break;
  }

  if (lazy_smp_count > 0)
  {
    lazy_smp_count = 0;
    uo_search_cutoff_parallel_search(thread, &lazy_smp_params.queue);
    uo_search_queue_item result;
    while (uo_search_queue_get_result(&lazy_smp_params.queue, &result))
    {
      thread->info.nodes += result.nodes;
      //if (result.move && result.value > value)
      //{
      //  value = result.value;
      //  entry.line[0] = result.move;
      //  entry.line[1] = 0;
      //}
    }
  }

  thread->info.completed = true;
  uo_search_print_info(thread);

  uo_engine_lock_position();
  for (size_t i = 0; i < 768; ++i)
  {
    engine.position.hhtable[i] = (position->hhtable[i] + 1) >> 1;
    engine.position.bftable[i] = (position->bftable[i] + 1) >> 1;
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

  uo_alphabeta entry = {
    .alpha = params->alpha,
    .beta = params->beta
  };

  bool completed = uo_search_quiesce(thread, &entry);

  double time_msec = uo_time_elapsed_msec(&thread->info.time_start);
  uint64_t nps = thread->info.nodes / time_msec * 1000.0;

  size_t tentry_count = uo_atomic_load(&engine.ttable.count);
  uint64_t hashfull = (tentry_count * 1000) / (engine.ttable.hash_mask + 1);

  uo_engine_lock_stdout();

  printf("info qsearch ");
  if (thread->info.seldepth) printf("seldepth %d ", thread->info.seldepth);

  if (entry.value > uo_score_mate_in_threshold)
  {
    printf("score mate %d ", (uo_score_checkmate - entry.value + 1) >> 1);
  }
  else if (entry.value < -uo_score_mate_in_threshold)
  {
    printf("score mate %d ", -((uo_score_checkmate + entry.value + 1) >> 1));
  }
  else
  {
    printf("score cp %d ", entry.value);
  }

  if (entry.value >= entry.beta)
  {
    printf("lowerbound ");
  }
  else if (entry.value <= entry.alpha)
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
