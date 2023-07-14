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
    const double margin_msec = engine_options.move_overhead / 2;
    double time_msec = uo_time_elapsed_msec(&thread->info.time_start);

    if (time_msec + margin_msec > (double)movetime)
    {
      if (!engine.pv[0]) return;
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

// see: https://en.wikipedia.org/wiki/Late_move_reductions
static inline size_t uo_search_choose_next_move_depth_reduction(uo_engine_thread *thread, uo_move move, size_t move_num, size_t depth)
{
  return 0;

  // Step 1. Initialize variables
  uo_search_info *info = &thread->info;
  uo_position *position = &thread->position;
  uo_move_history *stack = position->stack;

  size_t depth_reduction =
    // no reduction for shallow depth
    depth <= 4
    // no reduction if there are very few legal moves
    || stack->move_count < 8
    // no reduction if position is check
    || uo_position_is_check(position)
    // no reduction on promotions or captures
    || uo_move_is_tactical(move)
    // no reduction on killer moves
    || uo_position_is_killer_move(position, move)
    // no reduction on king moves
    || uo_position_move_piece(position, move) == uo_piece__K
    ? 0 // no reduction
    : uo_min(2, depth / 4); // default reduction is one fourth of depth but maximum of two

  return depth_reduction;
}

static inline size_t uo_search_choose_next_move_depth_extension(uo_engine_thread *thread, uo_move move, size_t move_num, size_t depth)
{
  uo_position *position = &thread->position;
  bool is_check = uo_position_is_check(position);
  uo_move_history *stack = position->stack;

  if (position->ply >= thread->info.depth * 2) return 0;

  size_t depth_extension = 0;

  // Check extension
  uo_bitboard checks = uo_position_move_checks(position, move, thread->move_cache);
  uo_position_update_next_move_checks(position, checks);
  if (checks) depth_extension += 1;

  return depth_extension;
}

// The quiescence search is a used to evaluate the board position when the game is not in non-quiet state,
// i.e., there are pieces that can be captured.
static int16_t uo_search_quiesce(uo_engine_thread *thread, int16_t alpha, int16_t beta, uint8_t depth, bool *incomplete)
{
  // Step 1. Initialize variables
  uo_search_info *info = &thread->info;
  uo_position *position = &thread->position;
  int16_t score_checkmate = uo_score_checkmate - position->ply;
  int16_t score_tb_win = uo_score_tb_win - position->ply;
  bool is_check = uo_position_is_check(position);
  size_t rule50 = uo_position_flags_rule50(position->flags);
  uo_move_history *stack = position->stack;

  // Step 2. Update searched node count and selective search depth information
  ++info->nodes;
  info->seldepth = uo_max(info->seldepth, position->ply);

  // Step 3. Check for draw by 50 move rule
  if (rule50 >= 100)
  {
    if (is_check)
    {
      size_t move_count = stack->moves_generated
        ? stack->move_count
        : uo_position_generate_moves(position);

      if (move_count == 0) return -score_checkmate;
    }

    return uo_score_draw;
  }

  // Step 4. Return draw score for the first repetition already.
  if (stack->repetitions) return uo_score_draw;

  // Step 5. Lookup position from transposition table and return if exact score for equal or higher depth is found
  uo_abtentry entry = { alpha, beta, 0 };
  if (uo_engine_lookup_entry(position, &entry))
  {
    // Unless draw by 50 move rule is a possibility, let's return transposition table score
    if (rule50 < 90) return entry.value;
  }

  // Step 6. If search is stopped, return unknown value
  if (uo_engine_thread_is_stopped(thread))
  {
    *incomplete = true;
    return uo_score_unknown;
  }

  // Step 7. If maximum search depth is reached return static evaluation
  if (uo_position_is_max_depth_reached(position)) return uo_position_evaluate(position);

  // Step 8. Generate legal moves
  size_t move_count = stack->moves_generated
    ? stack->move_count
    : uo_position_generate_moves(position);

  // Step 9. If there are no legal moves, return draw or checkmate
  if (move_count == 0) return is_check ? -score_checkmate : 0;

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
          if (value >= beta) return value;
          alpha = value;
        }
      }
    }

    return value;
  }

  // Step 11. Position is not check. Initialize score to static evaluation. "Stand pat"
  int16_t value = stack->static_eval
    = stack[-1].move == 0 ? -stack[-1].static_eval
    : stack->static_eval != uo_score_unknown ? stack->static_eval
    : uo_position_evaluate(position);

  assert(stack->static_eval != uo_score_unknown);

  // Step 12. Cutoff if static evaluation is higher or equal to beta.
  if (value >= beta) return beta;

  // Step 13. Update alpha
  alpha = uo_max(alpha, value);

  // Step 14. Sort tactical moves
  uo_position_sort_tactical_moves(&thread->position, thread->move_cache);

  // Step 15. Search tactical moves which have potential to raise alpha and checks depending on depth
  for (size_t i = 0; i < move_count; ++i)
  {
    uo_move move = position->movelist.head[i];

    // Step 15.1. Search moves that give a check on first depths of quiescence search
    uo_bitboard checks = uo_position_move_checks(position, move, thread->move_cache);
    if (checks && depth < UO_QS_CHECKS_DEPTH) goto search_move;

    // Step 15.2. Search queen and knight promotions
    uo_move_type move_promo_type = uo_move_get_type(move) & uo_move_type__promo_Q;
    if (move_promo_type == uo_move_type__promo_Q
      || move_promo_type == uo_move_type__promo_N)
      goto search_move;

    // Step 15.3. Futility pruning for captures
    if (uo_move_is_capture(move))
    {
      // Step 15.4. If move has low chance of raising alpha based on static exchange evaluation, do not search the move
      int16_t futility_margin = (checks ? 2 : 1) * uo_score_P;
      futility_margin = futility_margin - uo_score_mul_ln(futility_margin, depth);

      int16_t futility_threshold = alpha - stack->static_eval - futility_margin;
      if (uo_position_move_see_gt(position, move, futility_threshold, thread->move_cache)) goto search_move;
    }

    // Do not search the move
    continue;

  search_move:
    uo_position_update_next_move_checks(position, checks);
    uo_position_make_move(position, move);
    int16_t node_value = -uo_search_quiesce(thread, -beta, -alpha, depth + 1, incomplete);
    uo_position_unmake_move(position);

    if (*incomplete) return uo_score_unknown;

    if (node_value > value)
    {
      value = node_value;

      if (value > alpha)
      {
        if (value >= beta) return value;
        alpha = value;
      }
    }
  }

  return value;
}

static inline void uo_search_cutoff_parallel_search(uo_engine_thread *thread, uo_search_queue *queue, uo_search_queue_item *results)
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

  if (results)
  {
    while (uo_search_queue_get_result(queue, results))
    {
      ++results;
    }
  }
  else
  {
    uo_search_queue_item result;
    while (uo_search_queue_get_result(queue, &result))
    {
      thread->info.nodes += result.nodes;
    }
  }
}

static inline bool uo_search_try_delegate_parallel_search(uo_parallel_search_params *params)
{
  return uo_search_queue_try_enqueue(&params->queue, uo_engine_thread_run_parallel_principal_variation_search, params);
}

// Maximum principal variation length is double the root search depth
#define uo_allocate_line(depth) uo_alloca((depth + 2) * 2 * sizeof(uo_move))

static inline void uo_position_extend_pv(uo_position *position, uo_move bestmove, uo_move *line, size_t depth)
{
  line[0] = bestmove;
  line[1] = 0;

  if (!depth) return;

  uo_position_make_move(position, bestmove);
  uo_abtentry entry = { -uo_score_checkmate, uo_score_checkmate, depth };

  if (uo_engine_lookup_entry(position, &entry)
    && entry.bestmove
    && entry.data.type == uo_score_type__exact)
  {
    uo_position_extend_pv(position, entry.bestmove, line + 1, depth - 1);
  }

  uo_position_unmake_move(position);
}

static inline void uo_position_update_pv(uo_position *position, uo_move *pline, uo_move bestmove, uo_move *line)
{
  if (!pline) return;
  pline[0] = bestmove;

  size_t i = 0;

  while (line[i])
  {
    pline[i + 1] = line[i];
    ++i;
  }

  pline[i + 1] = 0;
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
  // Step 1. If specified search depth is reached, perform quiescence search and return evaluation if search was completed
  if (depth == 0) return uo_search_quiesce(thread, alpha, beta, 0, incomplete);

  // Step 2. Initialize variables
  uo_search_info *info = &thread->info;
  uo_position *position = &thread->position;
  int16_t score_checkmate = uo_score_checkmate - position->ply;
  int16_t score_tb_win = uo_score_tb_win - position->ply;
  bool is_check = uo_position_is_check(position);
  bool is_root_node = !position->ply;
  bool is_main_thread = thread->owner == NULL;
  bool is_zw_search = beta - alpha == 1;
  size_t rule50 = uo_position_flags_rule50(position->flags);
  uo_move_history *stack = position->stack;

  // Step 3. Check for draw by 50 move rule
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

  // Step 4. Check for draw by threefold repetition.
  // To minimize search tree, let's return draw score for the first repetition already. Exception is the search root position.
  if (stack->repetitions == 1 + is_root_node)
  {
    ++info->nodes;
    return uo_score_draw;
  }

  // Step 5. Mate distance pruning
  if (!is_root_node)
  {
    alpha = uo_max(-score_checkmate, alpha);
    beta = uo_min(score_checkmate + 1, beta);
    if (alpha >= beta) return alpha;
  }

  // Step 6. Lookup position from transposition table and return if exact score for equal or higher depth is found
  uo_abtentry entry = { alpha, beta, depth };
  if (uo_engine_lookup_entry(position, &entry))
  {
    // Let's update principal variation line if transposition table score is better than current best score but not better than beta
    if (pline
      && entry.value > alpha
      && entry.value < beta
      && entry.bestmove)
    {
      if (depth)
      {
        uo_move *line = uo_allocate_line(depth);
        line[0] = 0;
        uo_position_extend_pv(position, entry.bestmove, line, depth - 1);
        uo_position_update_pv(position, pline, entry.bestmove, line);
      }
      else
      {
        uo_position_update_pv(position, pline, entry.bestmove, NULL);
      }
    }

    // Unless draw by 50 move rule is a possibility, let's return transposition table score
    if (rule50 < 90) return entry.value;
  }

  // Step 7. If search is stopped, return unknown value
  if (!is_root_node && uo_engine_thread_is_stopped(thread))
  {
    *incomplete = true;
    return uo_score_unknown;
  }

  // Step 8. Increment searched node count
  ++thread->info.nodes;

  // Step 9. If maximum search depth is reached return static evaluation
  if (uo_position_is_max_depth_reached(position)) uo_position_evaluate(position);

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

      entry.value = wdl > engine.tb.score_wdl_draw ? score_tb_win - 101
        : wdl < engine.tb.score_wdl_draw ? -score_tb_win + 101
        : uo_score_draw;

      if (entry.value >= beta) return entry.value;
      alpha = uo_max(alpha, entry.value);
    }
  }

  // Step 11. Static evaluation and calculation of improvement margin
  int16_t static_eval = stack->static_eval
    = is_check ? uo_score_unknown
    : is_root_node ? uo_position_evaluate(position)
    : stack[-1].move == 0 ? -stack[-1].static_eval
    : uo_position_evaluate(position);

  assert(is_check || static_eval != uo_score_unknown);

  int16_t improvement_margin
    = stack[-2].static_eval != uo_score_unknown ? static_eval - stack[-2].static_eval
    : stack[-4].static_eval != uo_score_unknown ? static_eval - stack[-4].static_eval
    : uo_score_P;

  bool is_improving = improvement_margin > 0;

  // Step 13. Generate legal moves
  size_t move_count = stack->moves_generated
    ? stack->move_count - stack->skipped_move_count
    : uo_position_generate_moves(position);

  // Step 14. If there are no legal moves, return draw or checkmate
  if (move_count == 0) return is_check ? -score_checkmate : 0;

  // Step 15. Allocate principal variation line on the stack
  uo_move *line = uo_allocate_line(depth);
  line[0] = 0;

  // Step 16. Null move pruning
  if (!is_check
    && depth > 3
    && position->ply > 1 + thread->nmp_min_ply
    && uo_position_is_null_move_allowed(position)
    && static_eval >= beta)
  {
    // depth * 3/4 - 1
    size_t depth_nmp = depth * 3 / 4 - 1;

    uo_position_make_null_move(position);
    int16_t null_value = -uo_search_principal_variation(thread, depth_nmp, -beta, -beta + 1, NULL, false, incomplete);
    uo_position_unmake_null_move(position);

    if (*incomplete) return uo_score_unknown;

    null_value = uo_min(null_value, uo_score_tb_win_threshold - 1);
    if (null_value >= beta)
    {
      // 16.1. Verification
      thread->nmp_min_ply = position->ply + depth_nmp * 3 * 4;
      int16_t value = uo_search_principal_variation(thread, depth_nmp, beta - 1, beta, pline, false, incomplete);
      thread->nmp_min_ply = 0;

      if (value >= beta) return null_value;
    }
  }

  // Step 17. Sort moves and place pv move or transposition table move as first
  uo_move move = pline && pline[0] ? pline[0] : entry.data.bestmove;
  uo_position_sort_moves(position, move, thread->move_cache);
  assert(!move || move == position->movelist.head[0]);
  move = position->movelist.head[0];

  //// Step 18. Multi-Cut pruning
  //if (cut
  //  && is_zw_search
  //  && !is_check
  //  && depth > 2 * UO_MULTICUT_DEPTH_REDUCTION
  //  && entry.data.type == uo_score_type__lower_bound)
  //{
  //  size_t cutoff_counter = 0;
  //  size_t move_count_mc = uo_min(move_count, UO_MULTICUT_MOVE_COUNT);
  //  size_t depth_mc = depth - UO_MULTICUT_DEPTH_REDUCTION;
  //  uo_square *cut_move_squares = uo_alloca(UO_MULTICUT_CUTOFF_COUNT);

  //  for (size_t i = 0; i < move_count_mc; ++i)
  //  {
  //    uo_move move = position->movelist.head[i];
  //    uo_square square_from = uo_move_square_from(move);

  //    // Require that beta cut-offs are not caused by the same piece
  //    for (size_t j = 0; j < cutoff_counter; ++j)
  //    {
  //      if (cut_move_squares[j] == square_from)
  //      {
  //        move_count_mc = uo_min(move_count, move_count_mc + 1);
  //        goto next_move_mc;
  //      }
  //    };

  //    uo_position_make_move(position, move);
  //    int16_t node_value = -uo_search_principal_variation(thread, depth_mc, -beta, -beta + 1, NULL, false, incomplete);
  //    uo_position_unmake_move(position);

  //    if (node_value >= beta)
  //    {
  //      if (++cutoff_counter == UO_MULTICUT_CUTOFF_COUNT) return beta; // mc-prune
  //      cut_move_squares[cutoff_counter - 1] = square_from;
  //    }

  //  next_move_mc:;
  //  }
  //}

  // Step 19. Perform full alpha-beta search for the first move
  entry.depth = depth;

  // On main thread, root node, let's report current move on higher depths
  if (is_main_thread && is_root_node && depth >= 9)
  {
    uo_search_print_currmove(thread, move, 1);
  }

  uo_position_update_butterfly_heuristic(position, move);

  // Search extensions
  size_t depth_extension = uo_search_choose_next_move_depth_extension(thread, move, 0, depth);

  uo_position_make_move(position, move);
  int16_t node_value = -uo_search_principal_variation(thread, depth + depth_extension - 1, -beta, -alpha, line, false, incomplete);
  uo_position_unmake_move(position);

  if (*incomplete) return uo_score_unknown;

  if (node_value >= entry.value)
  {
    entry.value = node_value;
    entry.bestmove = move;

    if (entry.value > alpha)
    {
      uo_position_update_history_heuristic(position, entry.bestmove, depth);

      if (entry.value >= beta)
      {
        uo_position_update_killers(position, entry.bestmove);
        return uo_engine_store_entry(position, &entry);
      }

      alpha = entry.value;
      uo_position_update_pv(position, pline, entry.bestmove, line);
    }
  }

  // Step 20. Initialize parallel search parameters and result queue if applicable
  size_t parallel_search_count = 0;
  uo_parallel_search_params params = { 0 };
  bool is_parallel_search_root = is_root_node && depth >= UO_PARALLEL_MIN_DEPTH;

  if (is_parallel_search_root)
  {
    params.thread = thread;
    params.alpha = -beta;
    params.beta = -alpha;
    params.depth = depth - 1;
    params.line = uo_allocate_line(depth);
    params.line[0] = 0;
  };

  // Step 21. Search rest of the moves with zero window and reduced depth unless they fail-high
  for (size_t i = 1; i < move_count; ++i)
  {
    uo_move move = params.move = position->movelist.head[i];

    //// Step 21.1. Futility pruning for non promotion captures
    //if (!is_check
    //  && is_zw_search
    //  && depth < 9
    //  && entry.value < uo_score_tb_win_threshold
    //  && uo_move_is_capture(move)
    //  && !uo_move_is_promotion(move))
    //{
    //  // Step 21.1.1. Determine if move gives a check
    //  uo_bitboard checks = uo_position_move_checks(position, move, thread->move_cache);
    //  if (!checks)
    //  {
    //    // Step 21.4. If move has low chance of raising alpha based on static exchange evaluation, do not search the move
    //    int16_t futility_margin = uo_score_P * depth * 8 / 3;
    //    int16_t futility_threshold = alpha - entry.value - futility_margin;
    //    if (!uo_position_move_see_gt(position, move, futility_threshold, thread->move_cache)) continue;
    //  }

    //  uo_position_update_next_move_checks(position, checks);
    //}

    // On main thread, root node, let's report current move on higher depths
    if (is_main_thread && is_root_node && depth >= 9)
    {
      uo_search_print_currmove(thread, move, i + 1);
    }

    // Reset pv line
    line[0] = 0;

    // Step 21.1 Determine search depth extension
    size_t depth_extension = uo_search_choose_next_move_depth_extension(thread, move, i, depth);

    // Step 21.2 Determine search depth reduction
    size_t depth_reduction = uo_search_choose_next_move_depth_reduction(thread, move, i, depth);

    // Step 21.3 Perform zero window search for reduced depth
    size_t depth_lmr = depth - depth_reduction + depth_extension;
    uo_position_make_move(position, move);
    int16_t node_value = -uo_search_principal_variation(thread, depth_lmr - 1, -alpha - 1, -alpha, line, !cut, incomplete);

    if (*incomplete)
    {
      uo_position_unmake_move(position);
      if (parallel_search_count > 0) uo_search_cutoff_parallel_search(thread, &params.queue, NULL);
      return uo_score_unknown;
    }

    // Step 21.4 If move failed high, perform full re-search
    if (!is_zw_search
      && node_value > alpha
      && node_value <= beta)
    {
      if (is_parallel_search_root
        && move_count - i > UO_PARALLEL_MIN_MOVE_COUNT
        && parallel_search_count < UO_PARALLEL_MAX_COUNT
        && uo_search_try_delegate_parallel_search(&params))
      {
        params.line = uo_allocate_line(depth);
        params.line[0] = 0;
        uo_position_unmake_move(position);
        ++parallel_search_count;
        continue;
      }

      depth_lmr = depth + depth_extension;
      node_value = -uo_search_principal_variation(thread, depth_lmr - 1, -beta, -alpha, line, false, incomplete);

      if (*incomplete)
      {
        uo_position_unmake_move(position);
        if (parallel_search_count > 0) uo_search_cutoff_parallel_search(thread, &params.queue, NULL);
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
          if (parallel_search_count > 0) uo_search_cutoff_parallel_search(thread, &params.queue, NULL);
          return uo_engine_store_entry(position, &entry);
        }

        alpha = entry.value;
        params.beta = -alpha;
        uo_position_update_pv(position, pline, entry.bestmove, line);

        if (is_root_node && is_main_thread)
        {
          thread->info.bestmove_change_depth = depth_lmr;
          if (depth_lmr > 10) uo_search_print_info(thread);
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
          if (parallel_search_count > 0) uo_search_cutoff_parallel_search(thread, &params.queue, NULL);
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
              uo_search_cutoff_parallel_search(thread, &params.queue, NULL);
              return uo_engine_store_entry(position, &entry);
            }

            alpha = entry.value;
            params.beta = -alpha;
            uo_position_update_pv(position, pline, entry.bestmove, result.line);

            if (is_root_node && is_main_thread)
            {
              thread->info.bestmove_change_depth = depth_lmr;
              if (depth_lmr > 10) uo_search_print_info(thread);
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
        if (parallel_search_count > 0) uo_search_cutoff_parallel_search(thread, &params.queue, NULL);
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
            uo_search_cutoff_parallel_search(thread, &params.queue, NULL);
            return uo_engine_store_entry(position, &entry);
          }

          alpha = entry.value;
          params.beta = -alpha;
          uo_position_update_pv(position, pline, entry.bestmove, result.line);

          if (is_root_node && is_main_thread)
          {
            thread->info.bestmove_change_depth = depth_lmr;
            if (depth_lmr > 10) uo_search_print_info(thread);
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
  line[0] = 0;

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
  size_t nodes_prev = 0;
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
  int dtz;

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
        is_tb_position = true;

        int success;
        dtz = uo_tb_root_probe_dtz(position, &success);

        ++thread->info.tbhits;
        assert(success);

        is_tb_position = true;

        if (dtz > 0)
        {
          alpha = uo_max(0, alpha);
          beta = uo_score_checkmate;
        }
        else if (dtz < 0)
        {
          beta = uo_min(0, beta);
          alpha = -uo_score_checkmate;
        }
        else
        {
          alpha = uo_max(0, alpha);
          beta = uo_min(0, beta);
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
  uo_search_queue_item *lazy_smp_results = uo_alloca((lazy_smp_max_count + 1) * sizeof * lazy_smp_results);
  uo_move **lazy_smp_lines = uo_alloca((lazy_smp_max_count + 1) * sizeof * lazy_smp_lines);

  for (size_t i = 0; i < lazy_smp_max_count; ++i)
  {
    lazy_smp_lines[i] = uo_allocate_line(UO_MAX_PLY);
    lazy_smp_lines[i][0] = 0;
  }
  lazy_smp_lines[lazy_smp_max_count] = uo_allocate_line(1);


  uo_parallel_search_params lazy_smp_params = {
    .thread = thread,
    .queue = {.init = 0 },
    .alpha = alpha,
    .beta = beta,
    .line = lazy_smp_lines[0]
  };

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

    // Report search info
    if (thread->info.nodes)
    {
      uo_search_print_info(thread);
      nodes_prev = thread->info.nodes;
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
        lazy_smp_params.line = lazy_smp_lines[lazy_smp_count];
        lazy_smp_params.line[0] = 0;
      }

      // Start search on main search thread
      value = uo_search_principal_variation(thread, depth, alpha, beta, line, false, &incomplete);

      // Wait for parallel searches to finish
      if (lazy_smp_count > 0)
      {
        uo_search_cutoff_parallel_search(thread, &lazy_smp_params.queue, lazy_smp_results);

        for (size_t i = 0; i < lazy_smp_count; ++i)
        {
          uo_search_queue_item *result = lazy_smp_results + i;
          thread->info.nodes += result->nodes;

          if (result->incomplete
            || value >= result->value
            || !result->line[0])
          {
            continue;
          }

          value = result->value;
          uo_pv_copy(line, result->line);
        }

        lazy_smp_count = 0;
      }

      // Search was incomplete. Decide whether to use results from partial search.
      if (incomplete)
      {
        if (line[0]
          && line[1]
          && thread->info.nodes > nodes_prev)
        {
          bestmove = line[0];
          uo_pv_copy(engine.pv, line);
          uo_position_make_move(position, bestmove);
          engine.ponder.key = position->key;
          engine.ponder.move = line[1];
          uo_position_unmake_move(position);
        }
        else
        {
          uo_pv_copy(line, engine.pv);
        }

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
