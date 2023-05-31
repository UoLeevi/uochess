#include "uo_search.h"
#include "uo_position.h"
#include "uo_evaluation.h"
#include "uo_thread.h"
#include "uo_engine.h"
#include "uo_misc.h"
#include "uo_global.h"

#include <stddef.h>
#include <math.h>
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
  uo_move *line;
} uo_parallel_search_params;

static inline void uo_search_stop_if_movetime_over(uo_engine_thread *thread)
{
  uo_search_info *info = &thread->info;
  uo_search_params *params = &engine.search_params;
  int64_t movetime = params->movetime;

  // TODO: make better decisions about how much time to use
  if (!movetime && params->time_own)
  {
    // Difference between own time and opponents time
    int64_t time_diff = (int64_t)params->time_own - (int64_t)params->time_enemy;

    // Material percentage
    int64_t material_percentage = uo_position_material_percentage(&thread->position);

    // Estimate of how many moves are left on the game
    int64_t moves_left = uo_max(material_percentage, 10);

    // Time per move on average
    int64_t avg_time_per_move = params->time_own / moves_left + params->time_inc_own;

    // Default time to spend is average time per move plus half the difference to opponent's clock
    movetime = avg_time_per_move + time_diff / 2;

    // If best move is not chaning when doing iterative deepening, let's reduce thinking time
    int64_t bestmove_age_reduction = movetime / uo_max(16, info->depth);
    int64_t bestmove_age = info->depth - info->bestmove_change_depth;
    movetime = uo_max(movetime - bestmove_age_reduction * bestmove_age, 1);

    // The minimum time that should be left on the clock is one second
    movetime = uo_min(movetime, params->time_own - 1000);
  }

  if (movetime)
  {
    const double margin_msec = 1.0;
    double time_msec = uo_time_elapsed_msec(&thread->info.time_start);

    if (time_msec + margin_msec > (double)movetime)
    {
      if (!info->pv[0]) return;
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

    if (score == -uo_score_checkmate)
    {
      printf("score mate 0\nbestmove (none)\n");
      uo_engine_unlock_stdout();
      return;
    }
    else if (score > uo_score_mate_in_threshold)
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
    uo_move move = info->pv[0];

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
      uo_position_print_move(position, info->pv[0], buf);
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

static void uo_search_print_currmove(uo_engine_thread *thread, uo_move currmove, size_t currmovenumber)
{
  uo_position *position = &thread->position;

  uo_engine_lock_stdout();
  uo_position_print_move(position, currmove, buf);
  printf("info depth %d currmove %s currmovenumber %zu\n", thread->info.depth, buf, currmovenumber);
  uo_engine_unlock_stdout();
}

typedef uint8_t uo_search_quiesce_flags;

#define uo_search_quiesce_flags__checks ((uint8_t)1)
#define uo_search_quiesce_flags__positive_see ((uint8_t)2)
#define uo_search_quiesce_flags__non_negative_see ((uint8_t)6)
#define uo_search_quiesce_flags__any_see ((uint8_t)14)
#define uo_search_quiesce_flags__all_moves ((uint8_t)-1)

static inline uo_search_quiesce_flags uo_search_quiesce_determine_flags(uo_engine_thread *thread, uint8_t depth)
{
  if (depth <= 1)
  {
    return uo_search_quiesce_flags__checks | uo_search_quiesce_flags__non_negative_see;
  }
  else
  {
    return uo_search_quiesce_flags__positive_see;
  }
}

static inline bool uo_search_quiesce_should_examine_move(uo_engine_thread *thread, uo_move move, uo_search_quiesce_flags flags)
{
  uo_position *position = &thread->position;

  if (uo_move_is_capture(move))
  {
    if ((flags & uo_search_quiesce_flags__any_see) == uo_search_quiesce_flags__any_see)
    {
      return true;
    }

    int16_t see = uo_position_move_see(position, move, thread->move_cache);

    if ((flags & uo_search_quiesce_flags__any_see) == uo_search_quiesce_flags__positive_see && see > 0)
    {
      return true;
    }

    if ((flags & uo_search_quiesce_flags__any_see) == uo_search_quiesce_flags__non_negative_see && see >= 0)
    {
      return true;
    }
  }

  if ((flags & uo_search_quiesce_flags__checks) == uo_search_quiesce_flags__checks)
  {
    uo_bitboard checks = uo_position_move_checks(position, move, thread->move_cache);
    if (checks)
    {
      uo_position_update_next_move_checks(position, checks);
      return true;
    }
  }

  uo_move_type move_promo_type = uo_move_get_type(move) & uo_move_type__promo_Q;
  if (move_promo_type == uo_move_type__promo_Q
    || move_promo_type == uo_move_type__promo_N)
  {
    return true;
  }

  return false;
}

static int16_t uo_search_quiesce(uo_engine_thread *thread, int16_t alpha, int16_t beta, uint8_t depth, bool *incomplete)
{
  uo_search_info *info = &thread->info;
  uo_position *position = &thread->position;

  // Step 1. Adjust checkmate score
  int16_t score_checkmate = uo_score_checkmate - position->ply;

  // Step 2. Update searched node count and selective search depth information
  ++info->nodes;
  info->seldepth = uo_max(info->seldepth, position->ply);

  // Next two steps can be skipped on the first quiscence search node
  if (depth > 0)
  {
    // Step 3. Check for draw by 50 move rule
    if (uo_position_is_rule50_draw(position))
    {
      ++info->nodes;

      bool is_check = uo_position_is_check(position);
      if (is_check)
      {
        size_t move_count = position->stack->moves_generated
          ? position->stack->move_count
          : uo_position_generate_moves(position);

        if (move_count == 0) return -score_checkmate;
      }

      return uo_score_draw;
    }

    // Step 4. Check for draw by threefold repetition.
    //         To minimize search tree, let's return draw score for the first repetition already. Exception is the search root position.
    if (uo_position_repetition_count(position) == (position->ply ? 1 : 2))
    {
      ++info->nodes;
      return uo_score_draw;
    }

    // Step 5. If search is stopped, return unknown value
    if (uo_engine_thread_is_stopped(thread))
    {
      *incomplete = true;
      return uo_score_unknown;
    }
  }

  // Step 6. If maximum search depth is reached return static evaluation
  if (uo_position_is_max_depth_reached(position))
  {
    return uo_position_evaluate(position);
  }

  // Step 7. Generate legal moves
  size_t move_count = position->stack->moves_generated
    ? position->stack->move_count - position->stack->skipped_move_count
    : uo_position_generate_moves(position);

  // Step 8. Determine if position is check
  bool is_check = uo_position_is_check(position);

  // Step 9. If there are no legal moves, return draw or checkmate
  if (move_count == 0)
  {
    return is_check ? -score_checkmate : 0;
  }

  // Step 10. If position is check, perform quiesence search for all moves
  if (is_check)
  {
    // Step 10.1. Initialize score to be checkmate
    int16_t value = -score_checkmate;

    // Step 10.2. Sort moves
    uo_position_sort_moves(&thread->position, 0, thread->move_cache);

    // Step 10.3. Search each move
    for (size_t i = 0; i < move_count; ++i)
    {
      uo_move move = position->movelist.head[i];
      uo_position_make_move(position, move);
      int16_t node_value = -uo_search_quiesce(thread, -beta, -alpha, depth + 1, incomplete);
      uo_position_unmake_move(position);

      if (*incomplete) return uo_score_unknown;

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
    }

    return value;
  }

  // Step 11. Position is not check. Initialize score to static evaluation. "Stand pat"
  int16_t value = uo_position_evaluate(position);

  // Step 12. Cutoff if static evaluation is higher or equal to beta.
  if (value >= beta)
  {
    return beta;
  }

  // Step 13. Delta pruning
  if (position->Q)
  {
    int16_t delta = uo_piece_value(uo_piece__Q);
    if (alpha > value + delta)
    {
      return alpha;
    }
  }

  // Step 14. Update alpha
  alpha = uo_max(alpha, value);

  // Step 15. Determine which moves should be searched
  uo_search_quiesce_flags flags = uo_search_quiesce_determine_flags(thread, depth);

  // Step 16. Sort tactical moves
  uo_position_sort_tactical_moves(&thread->position, thread->move_cache);

  // Step 17. Search interesting tactical moves and checks
  for (size_t i = 0; i < move_count; ++i)
  {
    uo_move move = position->movelist.head[i];

    if (uo_search_quiesce_should_examine_move(thread, move, flags))
    {
      uo_position_make_move(position, move);
      int16_t node_value = -uo_search_quiesce(thread, -beta, -alpha, depth + 1, incomplete);
      uo_position_unmake_move(position);

      if (*incomplete) return uo_score_unknown;

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
    }
  }

  return value;
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

  uo_search_queue_item result;
  while (uo_search_queue_get_result(queue, &result))
  {
    thread->info.nodes += result.nodes;
  }
}

static inline bool uo_search_try_delegate_parallel_search(uo_parallel_search_params *params)
{
  return uo_search_queue_try_enqueue(&params->queue, uo_engine_thread_run_parallel_principal_variation_search, params);
}

#define uo_allocate_line(depth) uo_alloca(((depth) + 2) * sizeof(uo_move))

static inline void uo_position_extend_pv(uo_position *position, uo_move bestmove, uo_move *line, size_t depth)
{
  uo_position_make_move(position, bestmove);
  uo_abtentry entry = { -uo_score_checkmate, uo_score_checkmate, depth };
  if (uo_engine_lookup_entry(position, &entry) && entry.bestmove && entry.data.type == uo_score_type__exact)
  {
    line[0] = entry.bestmove;
    line[1] = 0;
    if (depth) uo_position_extend_pv(position, entry.bestmove, line + 1, depth - 1);
  }

  uo_position_unmake_move(position);
}

static inline void uo_pv_update(uo_move *pline, uo_move bestmove, uo_move *line, size_t move_count)
{
  if (!pline) return;

  pline[0] = bestmove;
  pline[move_count + 1] = 0;

  if (move_count)
  {
    memcpy(pline + 1, line, move_count * sizeof * line);
  }
}

static inline void uo_pv_copy(uo_move *line_dst, uo_move *line_src)
{
  size_t i = 0;
  while (line_src[i])
  {
    line_dst[i] = line_src[i];
    ++i;
  }
  line_dst[i] = 0;
}

static int16_t uo_search_principal_variation(uo_engine_thread *thread, size_t depth, int16_t alpha, int16_t beta, uo_move *pline, bool cut, bool *incomplete)
{
  // Step 1. Initialize variables
  uo_search_info *info = &thread->info;
  uo_position *position = &thread->position;
  int16_t score_checkmate = uo_score_checkmate - position->ply;
  int16_t score_tb_win = uo_score_tb_win - position->ply;
  bool is_check = uo_position_is_check(position);
  bool is_root_node = !position->ply;
  bool is_main_thread = thread->owner == NULL;
  size_t rule50 = uo_position_flags_rule50(position->flags);
  uo_move_history *stack = position->stack;

  // Step 2. Check for draw by 50 move rule
  if (rule50 >= 100)
  {
    ++info->nodes;

    if (is_check)
    {
      size_t move_count = stack->moves_generated
        ? stack->move_count
        : uo_position_generate_moves(position);

      if (move_count == 0) return -score_checkmate;
    }

    return uo_score_draw;
  }

  // Step 3. Check for draw by threefold repetition.
  //         To minimize search tree, let's return draw score for the first repetition already. Exception is the search root position.
  if (stack->repetitions == 1 + is_root_node)
  {
    ++info->nodes;
    return uo_score_draw;
  }

  // Step 4. Mate distance pruning
  if (!is_root_node)
  {
    alpha = uo_max(-score_checkmate, alpha);
    beta = uo_min(score_checkmate + 1, beta);
    if (alpha >= beta) return alpha;
  }

  // Step 5. Lookup position from transposition table and return if exact score for equal or higher depth is found
  uo_abtentry entry = { alpha, beta, depth };
  if (uo_engine_lookup_entry(position, &entry))
  {
    // Let's update principal variation line if transposition table score is better than current best score but not better than beta
    if (pline && entry.value > alpha && entry.value < beta && entry.bestmove)
    {
      if (depth)
      {
        uo_move *line = uo_allocate_line(depth);
        line[0] = 0;
        uo_position_extend_pv(position, entry.bestmove, line, depth - 1);
        uo_pv_update(pline, entry.bestmove, line, depth);
      }
      else
      {
        uo_pv_update(pline, entry.bestmove, NULL, 0);
      }
    }

    // Unless draw by 50 move rule is a possibility, let's return transposition table score
    if (uo_position_flags_rule50(position->flags) < 90)
    {
      return entry.value;
    }
  }

  // Step 6. If search is stopped, return unknown value
  if (!is_root_node && uo_engine_thread_is_stopped(thread))
  {
    *incomplete = true;
    return uo_score_unknown;
  }

  // Step 7. If specified search depth is reached, perform quiescence search and return evaluation if search was completed
  if (depth == 0)
  {
    int16_t value = uo_search_quiesce(thread, alpha, beta, 0, incomplete);
    if (*incomplete) return uo_score_unknown;
    return value;
  }

  // Step 8. Increment searched node count
  ++thread->info.nodes;

  // Step 9. If maximum search depth is reached return static evaluation
  if (uo_position_is_max_depth_reached(position))
  {
    return uo_position_evaluate(position);
  }

  entry.value = -score_checkmate;

  // Step 10. Tablebase probe
  // Do not probe on root node or if search depth is too shallow
  // Do not probe if 50 move rule, castling or en passant can interfere with the table base result
  if (engine.tb.enabled
    && !is_root_node
    && depth >= engine.tb.probe_depth
    && position->flags <= 1)
  {
    size_t piece_count = uo_popcnt(position->own | position->enemy);
    size_t probe_limit = uo_min(TBlargest, engine.tb.probe_limit);

    // Do not probe if too many pieces are on the board
    if (piece_count <= probe_limit)
    {
      int success;
      int wdl = uo_tb_probe_wdl(position, &success);
      assert(success);

      ++info->tbhits;

      entry.value = wdl > engine.tb.score_wdl_draw ? score_tb_win
        : wdl < engine.tb.score_wdl_draw ? -score_tb_win
        : uo_score_draw;

      if (entry.value >= beta)
      {
        return entry.value;
      }

      alpha = uo_max(alpha, entry.value);
    }
  }

  // Step 11. Static evaluation and calculation of improvement margin
  int16_t static_eval = stack->static_eval = is_check
    ? uo_score_unknown
    : uo_position_evaluate(position);

  int16_t improvement_margin
    = stack[-2].static_eval != uo_score_unknown ? static_eval - stack[-2].static_eval
    : stack[-4].static_eval != uo_score_unknown ? static_eval - stack[-4].static_eval
    : uo_score_P;

  bool is_improving = improvement_margin > 0;

  // Step 12. Futility pruning
  if (!is_check
    && depth < 9
    && static_eval >= beta
    && static_eval - uo_score_P * (depth - is_improving) >= beta
    && static_eval < uo_score_tb_win_threshold)
  {
    return static_eval;
  }

  // Step 13. Generate legal moves
  size_t move_count = stack->moves_generated
    ? stack->move_count - stack->skipped_move_count
    : uo_position_generate_moves(position);

  // Step 14. If there are no legal moves, return draw or checkmate
  if (move_count == 0)
  {
    return is_check ? -score_checkmate : 0;
  }

  // Step 15. Allocate principal variation line on the stack in case it is needed
  uo_move *line = pline ? uo_allocate_line(depth) : NULL;

  // Step 16. Null move pruning
  if (depth > 3
    && position->ply > 1
    && uo_position_is_null_move_allowed(position)
    && uo_position_evaluate(position) > beta)
  {
    // depth * 3/4 - 1
    size_t depth_nmp = (depth * 3 >> 2) - 1;

    uo_position_make_null_move(position);
    int16_t pass_value = -uo_search_principal_variation(thread, depth_nmp, -beta, -beta + 1, line, false, incomplete);
    uo_position_unmake_null_move(position);

    if (*incomplete) return uo_score_unknown;
    if (pass_value >= beta) return pass_value;
  }

  // Step 17. Sort moves and place pv move or transposition table move as first
  uo_move move = pline && pline[0] ? pline[0] : entry.data.bestmove;
  uo_position_sort_moves(&thread->position, move, thread->move_cache);

  // Step 18. Multi-Cut pruning
  if (cut && depth > UO_MULTICUT_DEPTH_REDUCTION)
  {
    size_t cutoff_counter = UO_MULTICUT_CUTOFF_COUNT;
    size_t move_count_mc = uo_min(move_count, UO_MULTICUT_MOVE_COUNT);
    size_t depth_mc = depth - UO_MULTICUT_DEPTH_REDUCTION;

    for (size_t i = 0; i < move_count_mc; ++i)
    {
      uo_move move = position->movelist.head[i];
      uo_position_make_move(position, move);
      int16_t node_value = -uo_search_principal_variation(thread, depth_mc - 1, -beta, -beta + 1, line, false, incomplete);
      uo_position_unmake_move(position);

      if (node_value >= beta && cutoff_counter-- == 0)
      {
        return beta; // mc-prune
      }
    }
  }

  // Step 19. Perform full alpha-beta search for the first move
  entry.bestmove = move = position->movelist.head[0];
  entry.depth = depth;

  // On main thread, root node, let's report current move on higher depths
  if (is_main_thread && is_root_node && depth >= 7)
  {
    uo_search_print_currmove(thread, move, 1);
  }

  uo_position_update_butterfly_heuristic(position, entry.bestmove);

  uo_position_make_move(position, entry.bestmove);
  int16_t node_value = -uo_search_principal_variation(thread, depth - 1, -beta, -alpha, line, !cut, incomplete);
  uo_position_unmake_move(position);

  if (*incomplete) return uo_score_unknown;

  if (node_value >= entry.value)
  {
    entry.value = node_value;

    if (entry.value > alpha)
    {
      uo_position_update_history_heuristic(position, entry.bestmove, depth);

      if (entry.value >= beta)
      {
        uo_position_update_killers(position, entry.bestmove);
        return uo_engine_store_entry(position, &entry);
      }

      alpha = entry.value;
      uo_pv_update(pline, entry.bestmove, line, entry.depth);
    }
  }

  // Step 20. Initialize parallel search parameters and result queue
  size_t parallel_search_count = 0;
  uo_parallel_search_params params = {
    .thread = thread,
    .queue = {.init = 0 },
    .alpha = -beta,
    .beta = -alpha,
    .depth = depth - 1,
    .line = pline ? uo_allocate_line(depth) : NULL
  };

  params.line[0] = 0;

  // Step 21. Search rest of the moves with zero window and reduced depth unless they fail-high
  for (size_t i = 1; i < move_count; ++i)
  {
    uo_move move = params.move = position->movelist.head[i];

    // On main thread, root node, let's report current move on higher depths
    if (is_main_thread && is_root_node && depth >= 7)
    {
      uo_search_print_currmove(thread, move, i + 1);
    }

    // Step 21.1 Determine search depth reduction
    // see: https://en.wikipedia.org/wiki/Late_move_reductions

    size_t depth_reduction =
      // no reduction for shallow depth
      depth <= 3
      // no reduction on the first three moves
      || i < 3
      // no reduction if there are very few legal moves
      || move_count < 8
      // no reduction if position is check
      || uo_position_is_check(position)
      // no reduction on promotions or captures
      || uo_move_is_tactical(move)
      ? 0 // no reduction
      : uo_max(1, depth / 3); // default reduction is one third of depth

    if (depth_reduction)
    {
      // no reduction if move gives a check
      uo_bitboard checks = uo_position_move_checks(position, move, thread->move_cache);
      uo_position_update_next_move_checks(position, checks);

      if (checks)
      {
        depth_reduction = 0;
      }
    }

    // Step 21.2 Perform zero window search for reduced depth
    size_t depth_lmr = depth - depth_reduction;
    uo_position_make_move(position, move);
    int16_t node_value = -uo_search_principal_variation(thread, depth_lmr - 1, -alpha - 1, -alpha, NULL, !cut, incomplete);

    if (*incomplete)
    {
      uo_position_unmake_move(position);
      if (parallel_search_count > 0) uo_search_cutoff_parallel_search(thread, &params.queue);
      return uo_score_unknown;
    }

    // Step 21.3 If move failed high, perform full re-search
    if (pline && node_value > alpha)
    {
      bool can_delegate = is_root_node && (depth >= UO_PARALLEL_MIN_DEPTH) && (move_count - i > UO_PARALLEL_MIN_MOVE_COUNT) && (parallel_search_count < UO_PARALLEL_MAX_COUNT);

      if (can_delegate && uo_search_try_delegate_parallel_search(&params))
      {
        params.line = uo_allocate_line(depth);
        params.line[0] = 0;
        uo_position_unmake_move(position);
        ++parallel_search_count;
        continue;
      }

      depth_lmr = depth;
      node_value = -uo_search_principal_variation(thread, depth - 1, -beta, -alpha, line, !cut, incomplete);

      if (*incomplete)
      {
        uo_position_unmake_move(position);
        if (parallel_search_count > 0) uo_search_cutoff_parallel_search(thread, &params.queue);
        return uo_score_unknown;
      }
    }

    uo_position_unmake_move(position);

    uo_position_update_butterfly_heuristic(position, move);

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
          if (parallel_search_count > 0) uo_search_cutoff_parallel_search(thread, &params.queue);
          return uo_engine_store_entry(position, &entry);
        }

        alpha = entry.value;
        params.beta = -alpha;
        uo_pv_update(pline, entry.bestmove, line, depth_lmr);

        if (is_root_node && is_main_thread)
        {
          thread->info.bestmove_change_depth = depth_lmr;

          if (depth_lmr > 8)
          {
            uo_search_print_info(thread);
          }
        }
      }
    }

    // Step 22. Get results from already finished parallel searches
    if (parallel_search_count > 0)
    {
      uo_search_queue_item result;

      while (uo_search_queue_try_get_result(&params.queue, &result))
      {
        --parallel_search_count;

        if (result.incomplete)
        {
          if (parallel_search_count > 0) uo_search_cutoff_parallel_search(thread, &params.queue);
          return uo_score_unknown;
        }

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
        int16_t node_value = -result.value;
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
              return uo_engine_store_entry(position, &entry);
            }

            alpha = entry.value;
            params.beta = -alpha;
            uo_pv_update(pline, entry.bestmove, result.line, entry.depth);

            if (is_root_node && is_main_thread)
            {
              thread->info.bestmove_change_depth = depth_lmr;

              if (depth_lmr > 8)
              {
                uo_search_print_info(thread);
              }
            }
          }
        }
      }
    }
  }

  // Step 23. Get results from remaining parallel searches and wait if not yet finished
  if (parallel_search_count > 0)
  {
    uo_search_queue_item result;

    while (uo_search_queue_get_result(&params.queue, &result))
    {
      --parallel_search_count;

      if (result.incomplete)
      {
        if (parallel_search_count > 0) uo_search_cutoff_parallel_search(thread, &params.queue);
        return uo_score_unknown;
      }

      size_t depth_lmr = result.depth;
      uo_move move = result.move;
      int16_t node_value = -result.value;
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
            return uo_engine_store_entry(position, &entry);
          }

          alpha = entry.value;
          params.beta = -alpha;
          uo_pv_update(pline, entry.bestmove, result.line, entry.depth);

          if (is_root_node && is_main_thread)
          {
            thread->info.bestmove_change_depth = depth_lmr;

            if (depth_lmr > 8)
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

// Adjust aspiration window by changing alpha and beta.
// Returns boolean indicating whether value was within aspiration window.
static inline bool uo_search_adjust_alpha_beta(int16_t value, int16_t *alpha, int16_t *beta, size_t *fail_count)
{
  const int16_t aspiration_window_fail_threshold = 2;
  const int16_t aspiration_window_minimum = 200;
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
  size_t depth = params->depth;
  int16_t alpha = params->alpha;
  int16_t beta = params->beta;
  uo_move move = params->move;
  uo_move *line = params->line;

  uo_position_copy(&thread->position, &owner->position);
  uo_atomic_unlock(&thread->busy);

  thread->info = (uo_search_info){
    .depth = depth,
    .multipv = engine_options.multipv,
    .nodes = 0,
    .pv = thread->pv,
    .secondary_pvs = thread->secondary_pvs
  };

  bool incomplete = false;

  int16_t value = uo_search_principal_variation(thread, depth, alpha, beta, line, false, &incomplete);

  uo_search_queue_item result = {
    .thread = thread,
    .nodes = thread->info.nodes,
    .depth = depth,
    .value = value,
    .move = move ? move : line[0],
    .line = line,
    .incomplete = incomplete
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
  // TODO: Handle the case where root position is already a checkmate or a stalemate

  uo_engine_thread *thread = arg;
  uo_position *position = &thread->position;
  uo_search_params *params = &engine.search_params;

  uo_atomic_unlock(&thread->busy);

  // Initialize search info
  thread->info = (uo_search_info){
    .depth = 1,
    .multipv = engine_options.multipv,
    .nodes = 0,
    .pv = thread->pv,
    .secondary_pvs = thread->secondary_pvs
  };

  uo_move *line = thread->pv;

  // Start timer
  uo_time_now(&thread->info.time_start);

  // Load position
  uo_engine_thread_load_position(thread);

  // Initialize aspiration window
  int16_t alpha = params->alpha;
  int16_t beta = params->beta;
  size_t aspiration_fail_count = 0;

  int16_t value;
  uo_move bestmove = 0;
  bool incomplete = false;

  // Check for ponder hit
  if (engine.ponder.key)
  {
    value = engine.ponder.value;

    // If opponent played the expected move, initialize pv and use more narrow aspiration window
    if (position->stack[-1].key == engine.ponder.key && engine.pv[2])
    {
      int16_t window = 400;
      bool ponder_hit = position->stack[-1].move == engine.ponder.move;

      if (ponder_hit)
      {
        // Ponder hit
        assert(engine.pv[1] == engine.ponder.move);

        // Use a more narrow aspiration window
        window = 100;

        // Copy principal variation from previous search
        size_t i = 0;
        while (engine.pv[i + 2])
        {
          line[i] = engine.pv[i] = engine.pv[i + 2];
          ++i;
        }
        line[i] = engine.pv[i] = 0;
      }

      alpha = value > -uo_score_checkmate + window ? value - window : -uo_score_checkmate;
      beta = value < uo_score_checkmate - window ? value - window : uo_score_checkmate;
    }
  }

  // Tablebase probe
  bool is_tb_position = false;
  int tb_wdl;

  if (engine.tb.enabled)
  {
    // Do not probe if castling can interfere with the table base result
    if (!uo_position_flags_castling(position->flags))
    {
      size_t piece_count = uo_popcnt(position->own | position->enemy);
      size_t probe_limit = uo_min(TBlargest, engine.tb.probe_limit);

      // Do not probe if too many pieces are on the board
      if (piece_count <= probe_limit)
      {
        int success;
        tb_wdl = uo_tb_root_probe(position, &success);
        ++thread->info.tbhits;
        assert(success);

        if (tb_wdl > 0)
        {
          alpha = uo_max(0, alpha);
          beta = uo_score_checkmate;
        }
        else if (tb_wdl < 0)
        {
          beta = uo_min(0, beta);
          alpha = -uo_score_checkmate;
        }
        else
        {
          thread->info.value = uo_score_draw;
          line[0] = engine.pv[0] = position->movelist.head[0];
          line[1] = engine.pv[1] = 0;
          goto search_completed;
        }
      }
    }
  }

  // Perform search for depth 1
  do
  {
    value = uo_search_principal_variation(thread, 1, alpha, beta, line, false, &incomplete);
  } while (!uo_search_adjust_alpha_beta(value, &alpha, &beta, &aspiration_fail_count));

  // We should always have a best move if search was successful
  bestmove = line[0];
  assert(bestmove);

  // Save pv
  uo_pv_copy(engine.pv, line);
  bestmove = line[0];

  // Save ponder move
  uo_position_make_move(position, bestmove);
  engine.ponder.key = position->key;
  engine.ponder.value = thread->info.value = value;
  engine.ponder.move = line[1];
  uo_position_unmake_move(position);

  // Stop search if mate in one
  if (value <= -uo_score_checkmate + 1 || value == uo_score_checkmate)
  {
    thread->info.value = value;
    goto search_completed;
  }

  // Start thread for managing time
  uo_engine_run_thread(uo_engine_thread_start_timer, thread);

  // Initialize Lazy SMP variables
  size_t lazy_smp_count = 0;
  size_t lazy_smp_max_count = uo_min(UO_PARALLEL_MAX_COUNT, engine.thread_count - 1 - UO_LAZY_SMP_FREE_THREAD_COUNT);
  uo_move *lazy_smp_lines = uo_allocate_line(lazy_smp_max_count * UO_MAX_PLY);
  lazy_smp_lines[0] = 0;

  uo_parallel_search_params lazy_smp_params = {
    .thread = thread,
    .queue = {.init = 0 },
    .alpha = alpha,
    .beta = beta,
    .line = lazy_smp_lines
  };

  lazy_smp_params.line[0] = 0;

  // Iterative deepening loop
  for (size_t depth = 2; depth <= params->depth; ++depth)
  {
    // Stop search if not possible to improve
    if (value <= -uo_score_checkmate + (int16_t)depth - 1 || value >= uo_score_checkmate - (int16_t)depth - 2)
    {
      goto search_completed;
    }

    // Update serch depth info
    thread->info.depth = lazy_smp_params.depth = depth;

    // Report searcg info
    if (thread->info.nodes)
    {
      uo_search_print_info(thread);
      thread->info.nodes = 0;
    }

    // Aspiration search loop
    do
    {
      // Start parallel search on Lazy SMP threads if depth is sufficient and there are available threads
      bool can_delegate = (depth >= UO_LAZY_SMP_MIN_DEPTH) && (lazy_smp_count < lazy_smp_max_count);

      while (can_delegate && uo_search_try_delegate_parallel_search(&lazy_smp_params))
      {
        can_delegate = ++lazy_smp_count < lazy_smp_max_count;
        lazy_smp_params.line = lazy_smp_lines + lazy_smp_count;
        lazy_smp_params.line[0] = 0;
      }

      // Start search on main search thread
      value = uo_search_principal_variation(thread, depth, alpha, beta, line, false, &incomplete);

      // Wait for parallel searches to finish
      if (lazy_smp_count > 0)
      {
        lazy_smp_count = 0;
        uo_search_cutoff_parallel_search(thread, &lazy_smp_params.queue);
      }

      // In case search was not finished, use results from previous search
      if (incomplete)
      {
        uo_pv_copy(line, engine.pv);
        goto search_completed;
      }

      // Check if search returned a value that is within aspiration window
      if (uo_search_adjust_alpha_beta(value, &alpha, &beta, &aspiration_fail_count))
      {
        // We should always have a best move if search was successful
        bestmove = line[0];
        assert(bestmove);

        uo_pv_copy(engine.pv, line);
        uo_position_make_move(position, bestmove);
        engine.ponder.key = position->key;
        engine.ponder.value = thread->info.value = value;
        engine.ponder.move = line[1];
        uo_position_unmake_move(position);
      }

      // Repeat search on same depth while aspiration search is unsuccessful
    } while (aspiration_fail_count);
  }

search_completed:
  thread->info.completed = true;
  uo_search_print_info(thread);

  uo_engine_lock_position();
  for (size_t i = 0; i < 768; ++i)
  {
    engine.position.hhtable[i] = (position->hhtable[i] + 1) >> 1;
    engine.position.bftable[i] = (position->bftable[i] + 1) >> 1;
  }
  uo_engine_unlock_position();
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
    .nodes = 0,
    .pv = thread->pv,
    .secondary_pvs = thread->secondary_pvs
  };

  uo_time_now(&thread->info.time_start);
  uo_engine_thread_load_position(thread);
  uo_engine_run_thread(uo_engine_thread_start_timer, thread);

  bool incomplete = false;

  int16_t alpha = params->alpha;
  int16_t beta = params->beta;
  int16_t value = uo_search_quiesce(thread, alpha, beta, 0, &incomplete);

  double time_msec = uo_time_elapsed_msec(&thread->info.time_start);
  uint64_t nps = thread->info.nodes / time_msec * 1000.0;

  size_t tentry_count = uo_atomic_load(&engine.ttable.count);
  uint64_t hashfull = (tentry_count * 1000) / (engine.ttable.hash_mask + 1);

  uo_engine_lock_stdout();

  printf("info qsearch ");
  if (thread->info.seldepth) printf("seldepth %d ", thread->info.seldepth);

  if (value > uo_score_mate_in_threshold)
  {
    printf("score mate %d ", ((uo_score_checkmate - value) >> 1) + 1);
  }
  else if (value < -uo_score_mate_in_threshold)
  {
    printf("score mate %d ", -(((uo_score_checkmate + value) >> 1) + 1));
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
