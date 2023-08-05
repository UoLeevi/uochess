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

    // Default time to spend is average time per move plus half the difference to opponent's clock,
    // but at minimum half the time per move
    movetime = uo_max(avg_time_per_move / 2, avg_time_per_move + time_diff / 2);

    // If best move is not chaning when doing iterative deepening, let's reduce thinking time
    int64_t bestmove_age_reduction = movetime / uo_max(16, info->depth);
    int64_t bestmove_age = info->depth - info->bestmove_change_depth;
    movetime = uo_max(1, movetime - bestmove_age_reduction * bestmove_age);

    // If opponent did play the expected move, reduce move time a bit
    if (engine.ponder.is_ponderhit)
    {
      movetime = movetime * 4 / 5;
    }

    // If current evaluation is significantly lower than previous evaluation, grant some additional search time
    if (engine.ponder.is_continuation
      && engine.ponder.value < uo_score_Q
      && engine.ponder.value > -uo_score_Q
      && engine.ponder.value > info->value
      && engine.ponder.value - info->value > uo_score_P / 2)
    {
      movetime += avg_time_per_move * 4;
    }

    // The minimum time that should be left on the clock is one second
    movetime = uo_max(1, uo_min(movetime, (int64_t)params->time_own - 1000));
  }

  if (movetime)
  {
    const double margin_msec = engine_options.move_overhead / 2;
    double time_msec = uo_time_elapsed_msec(&thread->info.time_start);

    info->movetime_remaining_msec = (double)movetime - time_msec - margin_msec;

    if (info->movetime_remaining_msec <= 0.0)
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

static void uo_search_print_info_string(uo_engine_thread *thread, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof buf, fmt, args);
  va_end(args);
  uo_engine_lock_stdout();
  printf("info string %s\n", buf);
  uo_engine_unlock_stdout();
}

static void uo_search_print_info(uo_engine_thread *thread)
{
  uo_search_info *info = &thread->info;
  uo_position *position = &thread->position;

  double time_msec = uo_time_elapsed_msec(&info->time_start);
  uint64_t nps = (double)thread->info.nodes / time_msec * 1000.0;

  size_t tentry_count = uo_atomic_load(&engine.ttable.count);
  uint64_t hashfull = (tentry_count * 1000) / (engine.ttable.hash_mask + 1);

  char move_str[6];

  uo_engine_lock_stdout();

  for (int i = 0; i < info->multipv; ++i)
  {
    printf("info depth %d ", info->depth);
    if (info->seldepth) printf("seldepth %d ", info->seldepth);
    printf("multipv %d ", info->multipv);

    int16_t score = info->value;

    if (info->is_tb_position)
    {
      if (info->dtz > 0)
      {
        score = uo_max(uo_score_tb_win_threshold, score);
      }
      else if (info->dtz < 0)
      {
        score = uo_min(-uo_score_tb_win_threshold, score);
      }
      else
      {
        score = 0;
      }
    }

    if (score == -uo_score_checkmate)
    {
      printf("score mate 0\nbestmove (none)\n");
      uo_engine_unlock_stdout();
      return;
    }
    else if (score > uo_score_mate_in_threshold)
    {
      printf("score mate %d ", (uo_score_checkmate - score + 1) / 2);
    }
    else if (score < -uo_score_mate_in_threshold)
    {
      printf("score mate %d ", (-uo_score_checkmate - score) / 2);
    }
    else
    {
      printf("score cp %d ", score);
    }

    printf("nodes %" PRIu64 " nps %" PRIu64 " hashfull %" PRIu64 " tbhits %d time %.0f",
      info->nodes, nps, hashfull, (int)info->tbhits, time_msec);

    size_t i = 0;
    uo_move move = info->pv[0];

    uint8_t color = uo_color(position->flags);
    uint16_t side_xor = 0;

    if (move)
    {
      printf(" pv");
      while (move)
      {
        uo_position_print_move(position, move ^ side_xor, move_str);
        side_xor ^= 0xE38;
        printf(" %s", move_str);
        move = info->pv[++i];
      }
    }

    printf("\n");

    if (info->completed && info->multipv == 1)
    {
      uo_position_print_move(position, info->pv[0], move_str);
      printf("bestmove %s", move_str);

      if (info->pv[1])
      {
        uo_position_print_move(position, info->pv[1] ^ 0xE38, move_str);
        printf(" ponder %s", move_str);
      }

      printf("\n");
    }
  }

  uo_engine_unlock_stdout();
}

static void uo_search_print_currmove(uo_engine_thread *thread, uo_move currmove, size_t currmovenumber)
{
  uo_position *position = &thread->position;
  char move_str[6];
  uo_position_print_move(position, currmove, move_str);

  uo_engine_lock_stdout();
  printf("info depth %d currmove %s currmovenumber %zu\n", thread->info.depth, move_str, currmovenumber);
  uo_engine_unlock_stdout();
}

static inline void uo_search_sort_and_filter_moves(uo_engine_thread *thread, uo_abtentry *entry)
{
  uo_position *position = &thread->position;
  uo_move_history *stack = position->stack;
  uo_move bestmove = entry->bestmove;

  if (stack->moves_sorted)
  {
    uo_position_sort_move_as_first(position, bestmove);
    return;
  }

  int16_t alpha = *entry->alpha;
  int16_t beta = *entry->beta;
  uint8_t depth = entry->depth;

  size_t move_count = stack->moves_generated
    ? stack->move_count - stack->skipped_move_count
    : uo_position_generate_moves(position);

  size_t tactical_move_count = stack->tactical_move_count;

  uo_move *movelist = position->movelist.head;

  for (size_t i = 0; i < move_count; ++i)
  {
    uo_move move = movelist[i];

    if (move == bestmove)
    {
      position->movelist.move_scores[i] = uo_score_checkmate;
      continue;
    }

    if (depth < 99)
    {
      position->movelist.move_scores[i] = i < tactical_move_count
        ? uo_position_calculate_tactical_move_score(position, move, thread->move_cache)
        : uo_position_calculate_non_tactical_move_score(position, move, thread->move_cache);

      continue;
    }

    uint64_t key = uo_position_move_key(position, move, NULL);
    uo_tdata data;
    bool found = uo_ttable_get(&engine.ttable, key, position->root_ply, &data);

    if (!found)
    {
      position->movelist.move_scores[i] = i < tactical_move_count
        ? uo_position_calculate_tactical_move_score(position, move, thread->move_cache)
        : uo_position_calculate_non_tactical_move_score(position, move, thread->move_cache);

      continue;
    }

    int16_t score_tt = -data.value;

    if (score_tt >= beta
      && (data.type == uo_score_type__upper_bound || data.type == uo_score_type__exact))
    {
      // TODO
    }
    else if (score_tt <= alpha
      && (data.type == uo_score_type__upper_bound || data.type == uo_score_type__exact))
    {
      // TODO
    }

    position->movelist.move_scores[i] = score_tt / 5 + uo_score_checkmate / 10 + data.depth * 100;
  }

  uo_position_quicksort_moves(position, movelist, 0, move_count - 1);
  uo_position_sort_skipped_moves(position, movelist, 0, move_count - 1);
  position->stack->moves_sorted = true;
}

static inline int uo_search_determine_depth_reduction_or_extension(uo_engine_thread *thread, size_t move_num, size_t depth, int16_t alpha, int16_t beta, int improvement_count)
{
  // Step 1. Initialize variables
  uo_search_info *info = &thread->info;
  uo_position *position = &thread->position;
  uo_move_history *stack = position->stack;
  bool is_check = uo_position_is_check(position);
  uo_move move = stack[-1].move;
  uo_square square_to = uo_move_square_to(move);
  const uo_piece *board = position->board;
  uo_piece piece = board[square_to ^ 56] ^ 1;

  // Step 2. Limit search depth extensions to prevent search explosion.
  int net_extension_plies = (int)position->ply + (int)depth - (int)info->depth;
  int max_extension = uo_max(0, ((int)info->depth - uo_max(0, net_extension_plies)) * 1000);

  // Step 3. Determine depth extension
  int extension = 0;

  // Passed pawn near promotion extension
  if (piece == uo_piece__P
    && depth < 6
    && square_to >= uo_square__a7)
  {
    extension += 1000;
  }

  // Check extension
  if (is_check) extension += 600;

  // If search extension is specified, return depth extension in plies.
  if (extension > 0)
  {
    extension = uo_min(extension, max_extension);
    return lround((double)extension / 1000.0);
  }

  // Step 4. No extension. Determine reductions.

  // Also limit reductions to avoid missing threats
  int max_reduction = uo_min((int)depth * 400 + 1000, (int)info->depth * 600 - uo_max(0, -net_extension_plies * 1000));
  if (max_reduction <= 0) return 0;

  if (
    // no reduction for shallow depth
    depth <= 3
    // no reduction if aspiration window is really wide
    || alpha <= -uo_score_tb_win_threshold
    // no reduction if looking for mate
    || beta >= uo_score_tb_win_threshold
    // no reduction on queen or knight promotions
    || uo_move_is_promotion_Q_or_N(move)
    // no reduction on captures
    || uo_move_is_capture(move)
    // no reduction on killer moves
    || uo_position_is_killer_move(position, 0))
  {
    return 0;
  }

  // Default reduction is one fifth of the depth but capped at two plies
  int reduction = depth * 200;

  // Reduction based on move ordering
  reduction += move_num * 50;

  // Reduction based on how many times alpha has been improved already
  reduction += improvement_count * 500;

  // Return depth reduction in plies.
  reduction = uo_min(reduction, max_reduction);
  return -lround((double)reduction / 1000.0);
}

// The quiescence search is a used to evaluate the board position when the game is not in non-quiet state,
// i.e., there are pieces that can be captured.
static int16_t uo_search_quiesce(uo_engine_thread *thread, int16_t alpha, int16_t beta, uint8_t depth, bool *incomplete)
{
  // Step 1. Initialize variables
  uo_search_info *info = &thread->info;
  uo_position *position = &thread->position;
  uo_move_history *stack = position->stack;
  const int16_t score_checkmate = uo_score_checkmate - position->ply;
  const int16_t score_tb_win = uo_score_tb_win - position->ply;
  const size_t rule50 = uo_position_flags_rule50(position->flags);

  // Step 2. Update searched node count and selective search depth information
  ++info->nodes;
  info->seldepth = uo_max(info->seldepth, position->ply);

  // Step 3. Check for draw by 50 move rule
  if (rule50 >= 100)
  {
    if (uo_position_is_check(position))
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

  // Step 5. Mate distance pruning
  alpha = uo_max(-score_checkmate, alpha);
  beta = uo_min(score_checkmate - 1, beta);
  if (alpha >= beta) return alpha;

  // Step 6. Lookup position from transposition table and return if exact score for equal or higher depth is found
  uo_abtentry entry = { &alpha, &beta, 0 };
  if (uo_engine_lookup_entry(position, &entry))
  {
    return entry.value;
  }

  // Step 7. If search is stopped, return unknown value
  if (uo_engine_thread_is_stopped(thread))
  {
    *incomplete = true;
    return uo_score_unknown;
  }

  // Step 8. If maximum search depth is reached return static evaluation
  if (uo_position_is_max_depth_reached(position)) return uo_position_evaluate_and_cache(position, thread->move_cache);

  // Step 9. If position is check, perform quiesence search for all moves
  if (uo_position_is_check(position))
  {
    // Step 9.1. Generate legal moves
    size_t move_count = stack->moves_generated
      ? stack->move_count
      : uo_position_generate_moves(position);

    // Step 9.2. If there are no legal moves, return checkmate
    if (move_count == 0) return -score_checkmate;

    // Step 9.3. Initialize score to be checkmate
    entry.value = -score_checkmate;

    // Step 9.4. Sort moves
    uo_position_sort_moves(&thread->position, entry.bestmove, thread->move_cache);

    // Step 9.5. Search each move
    for (size_t i = 0; i < move_count; ++i)
    {
      uo_move move = position->movelist.head[i];
      uo_position_flags flags;
      uint64_t key = uo_position_move_key(position, move, &flags);
      uo_engine_prefetch_entry(key);
      uo_position_make_move(position, move, key, flags);
      assert(!key || key == position->key);
      int16_t node_value = -uo_search_quiesce(thread, -beta, -alpha, depth + 1, incomplete);
      uo_position_unmake_move(position);

      if (*incomplete) return uo_score_unknown;

      if (node_value > entry.value)
      {
        entry.value = node_value;
        entry.bestmove = move;

        if (entry.value > alpha)
        {
          if (entry.value >= beta)
          {
            return uo_engine_store_entry(position, &entry);
          }

          alpha = entry.value;
        }
      }
    }

    return entry.value;
  }

  // Position is not check. Examine only tactical moves and the possible transposition table move.

  // Step 10. Initialize score to static evaluation. "Stand pat"
  int16_t static_eval = uo_position_evaluate_and_cache(position, thread->move_cache);
  assert(stack->static_eval != uo_score_unknown);

  // Step 11. Adjust value if position was found from transposition table
  entry.value =
    entry.data.type == uo_score_type__lower_bound ? uo_max(entry.data.value, static_eval) :
    entry.data.type == uo_score_type__upper_bound ? uo_min(entry.data.value, static_eval) : // <- this might be a bit dubious
    static_eval;

  // Step 12. Cutoff if static evaluation is higher or equal to beta.
  if (entry.value >= beta) return beta;

  // Step 13. Delta pruning
  bool is_promotion_possible = (position->P & position->own) >= uo_square_bitboard(uo_square__a7);
  int16_t delta
    // Large material gain from capture
    = ((position->Q & position->enemy) ? uo_score_Q : uo_score_R)
    // Promotion
    + is_promotion_possible * uo_score_Q
    // Reduction from quiscence search depth
    - uo_score_mul_ln(uo_score_P, depth * depth);

  if (entry.value + delta < alpha) return alpha;

  // Step 14. Update alpha
  alpha = uo_max(alpha, entry.value);

  // Step 15. Search transposition table move if it is not a tactical move
  if (entry.bestmove
    && !uo_move_is_tactical(entry.bestmove))
  {
    uo_position_flags flags;
    uint64_t key = uo_position_move_key(position, entry.bestmove, &flags);
    uo_engine_prefetch_entry(key);
    uo_position_make_move(position, entry.bestmove, key, flags);
    assert(!key || key == position->key);
    int16_t node_value = -uo_search_quiesce(thread, -beta, -alpha, depth + 1, incomplete);
    uo_position_unmake_move(position);

    if (*incomplete) return uo_score_unknown;

    if (node_value > beta) return node_value;

    entry.value = uo_max(node_value, entry.value);
    alpha = uo_max(node_value, alpha);
  }

  // Step 16. Determine futility threshold for pruning unpotential moves
  const int16_t futility_base = static_eval + uo_score_P * 3 / 2;
  int16_t futility_threshold = alpha < futility_base ? 0 : alpha - futility_base;

  // Step 17. Generate tactical moves with potential to raise alpha
  size_t tactical_move_count = uo_position_generate_tactical_moves(position, futility_threshold);

  // Step 18. Sort tactical moves
  uo_position_sort_tactical_moves(position, thread->move_cache);

  // Step 19. Search tactical moves
  for (size_t i = 0; i < tactical_move_count; ++i)
  {
    uo_move move = position->movelist.head[i];

    // Step 19.1. Futility pruning for captures
    if (uo_move_is_capture(move)
      && !uo_move_is_promotion(move)
      && !uo_move_is_enpassant(move)
      && !uo_position_move_see_gt(position, move, futility_threshold, thread->move_cache))
    {
      continue;
    }

    uo_position_flags flags;
    uint64_t key = uo_position_move_key(position, move, &flags);
    uo_engine_prefetch_entry(key);
    uo_position_make_move(position, move, key, flags);
    assert(!key || key == position->key);
    int16_t node_value = -uo_search_quiesce(thread, -beta, -alpha, depth + 1, incomplete);
    uo_position_unmake_move(position);

    if (*incomplete) return uo_score_unknown;

    if (node_value >= beta) return node_value;

    if (node_value > entry.value)
    {
      entry.value = node_value;
      entry.bestmove = move;

      if (node_value > alpha)
      {
        alpha = node_value;
        futility_threshold = alpha < futility_base ? 0 : alpha - futility_base;
      }
    }
  }

  return entry.value;
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
#define uo_allocate_line(depth) uo_alloca(uo_max((depth + 2) * 2, UO_PV_ALLOC_MIN_LENGTH) * sizeof(uo_move))

static inline void uo_position_extend_pv(uo_position *position, uo_move bestmove, uo_move *line, size_t depth)
{
  line[0] = bestmove;
  line[1] = 0;

  if (!depth) return;

  uo_position_make_move(position, bestmove, 0, 0);

  int16_t alpha = -uo_score_checkmate;
  int16_t beta = uo_score_checkmate;
  uo_abtentry entry = { &alpha, &beta, depth };

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
  uo_move_history *stack = position->stack;
  bool iid = false;
  int improvement_count = 0;
  const int16_t score_checkmate = uo_score_checkmate - position->ply;
  const int16_t score_tb_win = uo_score_tb_win - position->ply;
  const bool is_check = uo_position_is_check(position);
  const bool is_root_node = !position->ply;
  const bool is_main_thread = thread->owner == NULL;
  const bool is_zw_search = beta - alpha == 1;
  const size_t depth_initial = depth;
  const size_t rule50 = uo_position_flags_rule50(position->flags);

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
    beta = uo_min(score_checkmate - 1, beta);
    if (alpha >= beta) return alpha;
  }

  // Step 6. Lookup position from transposition table and return if exact score for equal or higher depth is found
  uo_abtentry entry = { &alpha, &beta, depth };
  if (uo_engine_lookup_entry(position, &entry))
  {
    // For root node, in case the position is in tablebase, let's verify that the tt move is preserving win or draw.
    if (is_root_node
      && info->is_tb_position
      && entry.bestmove != pline[0]
      && uo_position_is_skipped_move(position, entry.bestmove))
    {
      goto continue_search;
    }

    // Let's update principal variation line if transposition table score is better than current best score
    if (pline
      && entry.bestmove
      && entry.value > alpha)
    {
      uo_position_extend_pv(position, entry.bestmove, pline, depth);
    }

    // On root node, best move is required
    if (!is_root_node || entry.bestmove) return entry.value;
  }

continue_search:

  // Step 7. If search is stopped, return unknown value
  if (!is_root_node && uo_engine_thread_is_stopped(thread))
  {
    *incomplete = true;
    return uo_score_unknown;
  }

  // Step 8. Increment searched node count
  ++thread->info.nodes;

  // Step 9. If maximum search depth is reached return static evaluation
  if (uo_position_is_max_depth_reached(position)) return uo_position_evaluate_and_cache(position, thread->move_cache);

  // Step 10. Tablebase probe
  // Do not probe on root node or if search depth is too shallow
  // Do not probe if 50 move rule, castling or en passant can interfere with the table base result
  if (engine.tb.enabled
    && !is_root_node
    && !info->is_tb_position
    && depth >= engine.tb.probe_depth
    && rule50 == 0
    && uo_position_flags_castling(position->flags) == 0
    && uo_position_flags_enpassant_file(position->flags) == 0)
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

      return
        wdl > engine.tb.score_wdl_draw ? score_tb_win - 101 :
        wdl < -engine.tb.score_wdl_draw ? -score_tb_win + 101 :
        wdl;
    }
  }

  // Step 11. Allocate principal variation line on the stack
  uo_move *line = uo_allocate_line(depth);
  line[0] = 0;

  // Step 12. Search bestmove even before move generation
  entry.bestmove = pline && pline[0] ? pline[0] : entry.bestmove;

  if (entry.bestmove)
  {
    // On main thread, root node, let's report current move on higher depths
    if (is_main_thread
      && is_root_node
      && depth > 8
      && uo_time_elapsed_msec(&info->time_start) > 3000)
    {
      uo_search_print_currmove(thread, entry.bestmove, 1);
    }

    uo_position_flags flags;
    uint64_t key = uo_position_move_key(position, entry.bestmove, &flags);
    uo_engine_prefetch_entry(key);
    uo_position_make_move(position, entry.bestmove, key, flags);
    assert(key == position->key);
    int depth_extension = uo_max(0, uo_search_determine_depth_reduction_or_extension(thread, 0, depth, alpha, beta, improvement_count));
    entry.value = -uo_search_principal_variation(thread, depth + depth_extension - 1, -beta, -alpha, line, false, incomplete);
    uo_position_unmake_move(position);

    if (*incomplete) return uo_score_unknown;

    if (entry.value > alpha)
    {
      if (pline) uo_position_update_pv(position, pline, entry.bestmove, line);

      if (entry.value >= beta)
      {
        uo_position_update_killer_move(position, entry.bestmove, depth);
        return uo_engine_store_entry(position, &entry);
      }

      ++improvement_count;
      alpha = entry.value;
    }
  }

  // Step 13. Generate legal moves
  size_t move_count = stack->moves_generated
    ? stack->move_count - stack->skipped_move_count
    : uo_position_generate_moves(position);

  // Step 14. If there are no legal moves, return draw or checkmate
  if (move_count == 0) return is_check ? -score_checkmate : 0;

  // Step 15. Static evaluation and calculation of improvement margin
  int16_t static_eval = uo_position_evaluate_and_cache(position, thread->move_cache);
  assert(is_check || static_eval != uo_score_unknown);

  bool is_improving
    = stack[-2].checks == uo_move_history__checks_none ? static_eval > stack[-2].static_eval
    : stack[-4].checks == uo_move_history__checks_none ? static_eval > stack[-4].static_eval
    : true;

  // Step 16. Null move pruning
  if (!is_check
    && !is_root_node
    && static_eval >= beta
    && !entry.bestmove
    && depth > 3
    && position->ply >= thread->nmp_min_ply
    && uo_position_is_null_move_allowed(position))
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
      // 15.1. Verification
      thread->nmp_min_ply = position->ply + depth_nmp * 3 / 4;
      int16_t value = uo_search_principal_variation(thread, depth_nmp, beta - 1, beta, pline, false, incomplete);
      thread->nmp_min_ply = 0;

      if (value >= beta) return null_value;
    }
  }

  // Step 17. Sort moves and place pv move or transposition table move as first
  uo_position_sort_moves(position, entry.bestmove, thread->move_cache);
  //uo_search_sort_and_filter_moves(thread, &entry);
  assert(!entry.bestmove || entry.bestmove == position->movelist.head[0]);

  if (!entry.bestmove)
  {
    uo_move move = entry.bestmove = position->movelist.head[0];

    // On main thread, root node, let's report current move on higher depths
    if (is_main_thread
      && is_root_node
      && depth > 8
      && uo_time_elapsed_msec(&info->time_start) > 3000)
    {
      uo_search_print_currmove(thread, move, 1);
    }

    // Step 17. Internal iterative deepening
    iid = depth > 5 && !is_zw_search;
    depth = depth / (1 + iid);

    if (false)
    {
    increase_depth_iid:
      iid = false;
      line[0] = 0;
      improvement_count = 0;
      entry.depth = depth = depth_initial;
      alpha = entry.alpha_initial;
      uo_position_sort_move_as_first(position, entry.bestmove);
    }

    // Step 18. Perform full alpha-beta search for the first move

    // Search extensions
    uo_position_flags flags;
    uint64_t key = uo_position_move_key(position, move, &flags);
    uo_engine_prefetch_entry(key);
    uo_position_make_move(position, move, key, flags);
    assert(key == position->key);
    int depth_extension = uo_max(0, uo_search_determine_depth_reduction_or_extension(thread, 0, depth, alpha, beta, improvement_count));
    entry.value = -uo_search_principal_variation(thread, depth + depth_extension - 1, -beta, -alpha, line, false, incomplete);
    uo_position_unmake_move(position);

    if (*incomplete) return uo_score_unknown;

    if (entry.value > alpha)
    {
      if (pline && !iid) uo_position_update_pv(position, pline, entry.bestmove, line);

      if (entry.value >= beta)
      {
        if (iid) goto increase_depth_iid;
        uo_position_update_cutoff_history(position, 0, depth);
        return uo_engine_store_entry(position, &entry);
      }

      ++improvement_count;
      alpha = entry.value;
    }
  }

  // Step 19. Initialize parallel search parameters and result queue if applicable
  size_t parallel_search_count = 0;
  uo_parallel_search_params params = { 0 };
  bool is_parallel_search_root = !iid && !is_zw_search && depth >= UO_PARALLEL_MIN_DEPTH;

  if (is_parallel_search_root)
  {
    params.thread = thread;
    params.alpha = -beta;
    params.beta = -alpha;
    params.depth = depth - 1;
    params.line = uo_allocate_line(depth);
    params.line[0] = 0;
  };

  // Step 20. Search rest of the moves with zero window and reduced depth unless they fail-high
  for (size_t i = 1; i < move_count; ++i)
  {
    uo_move move = params.move = position->movelist.head[i];

    // Step 20.1 Capture pruning using SEE
    if (!is_check
      && uo_move_get_type(move) == uo_move_type__x /* non-promotion, non-enpassant capture */
      && entry.value >= -uo_score_tb_win_threshold
      && !uo_position_move_checks(position, move, thread->move_cache)
      && !uo_position_move_see_gt(position, move, depth * -uo_score_P * 2, thread->move_cache)
      && !uo_position_move_discoveries(position, move, position->enemy & (position->R | position->Q | position->K)))
    {
      // Prune bad capture
      continue;
    }

    // On main thread, root node, let's report current move on higher depths
    if (is_main_thread
      && is_root_node
      && depth > 8
      && uo_time_elapsed_msec(&info->time_start) > 3000)
    {
      uo_search_print_currmove(thread, move, i + 1);
    }

    // Step 20.1 Reset pv line
    line[0] = 0;

    // Step 20.2 Determine search depth extension or reduction
    uo_position_flags flags;
    uint64_t key = uo_position_move_key(position, move, &flags);
    uo_engine_prefetch_entry(key);
    uo_position_make_move(position, move, key, flags);
    assert(!key || key == position->key);
    int depth_extension_or_reduction = is_check ? 0 : uo_search_determine_depth_reduction_or_extension(thread, i, depth, alpha, beta, improvement_count);
    int16_t node_value = alpha + 1;
    size_t depth_lmr = depth + depth_extension_or_reduction;

    // Step 20.3 Perform zero window search for reduced depth
    if (is_zw_search || depth_extension_or_reduction < 0)
    {
      node_value = -uo_search_principal_variation(thread, depth_lmr - 1, -alpha - 1, -alpha, is_zw_search ? NULL : line, !cut, incomplete);

      if (*incomplete)
      {
        uo_position_unmake_move(position);
        if (parallel_search_count > 0) uo_search_cutoff_parallel_search(thread, &params.queue, NULL);
        return uo_score_unknown;
      }
    }

    // Step 20.4 If reduced depth search failed high, perform full re-search
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

      int depth_extension = uo_max(0, depth_extension_or_reduction);
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

    if (node_value > entry.value)
    {
      entry.value = node_value;
      entry.bestmove = move;
      entry.depth = depth_lmr;

      if (entry.value > alpha)
      {
        // Update principal variation
        if (pline && !iid) uo_position_update_pv(position, pline, entry.bestmove, line);

        // Beta cutoff
        if (entry.value >= beta)
        {
          if (iid) goto increase_depth_iid;
          uo_position_update_cutoff_history(position, i, depth);
          if (parallel_search_count > 0) uo_search_cutoff_parallel_search(thread, &params.queue, NULL);
          return uo_engine_store_entry(position, &entry);
        }

        // Update alpha
        ++improvement_count;
        alpha = entry.value;
        params.beta = -alpha;

        if (is_root_node && is_main_thread)
        {
          thread->info.bestmove_change_depth = depth_lmr;
          if (uo_time_elapsed_msec(&info->time_start) > 3000) uo_search_print_info(thread);
        }
      }
    }

    // Step 21. Get results from already finished parallel searches
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
            if (pline) uo_position_update_pv(position, pline, entry.bestmove, result.line);

            if (entry.value >= beta)
            {
              size_t i = 0;
              for (; i < move_count; ++i)
                if (move == position->movelist.head[i]) break;

              assert(i < move_count);

              uo_position_update_cutoff_history(position, i, depth);
              uo_search_cutoff_parallel_search(thread, &params.queue, NULL);
              return uo_engine_store_entry(position, &entry);
            }

            ++improvement_count;
            alpha = entry.value;
            params.beta = -alpha;

            if (is_root_node && is_main_thread)
            {
              thread->info.bestmove_change_depth = depth_lmr;
              if (uo_time_elapsed_msec(&info->time_start) > 3000) uo_search_print_info(thread);
            }
          }
        }
      }
    }
  }

  // Step 22. Get results from remaining parallel searches and wait if not yet finished
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
          if (pline) uo_position_update_pv(position, pline, entry.bestmove, result.line);

          if (entry.value >= beta)
          {
            size_t i = 0;
            for (; i < move_count; ++i)
              if (move == position->movelist.head[i]) break;

            assert(i < move_count);

            uo_position_update_cutoff_history(position, i, depth);
            uo_search_cutoff_parallel_search(thread, &params.queue, NULL);
            return uo_engine_store_entry(position, &entry);
          }

          alpha = entry.value;
          params.beta = -alpha;

          if (is_root_node && is_main_thread)
          {
            thread->info.bestmove_change_depth = depth_lmr;
            if (uo_time_elapsed_msec(&info->time_start) > 3000) uo_search_print_info(thread);
          }
        }
      }
    }
  }

  if (iid) goto increase_depth_iid;
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
// Returns
//  1 on fail high
//  0 when value is within window
// -1 on fail low
static inline int uo_search_adjust_alpha_beta(int16_t value, int16_t *alpha, int16_t *beta)
{
  const int fail_high = 1;
  const int exact = 0;
  const int fail_low = -1;

  const int16_t aspiration_window_minimum = uo_score_P / 3;
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
    *alpha = -uo_score_checkmate;
    *beta = uo_score_checkmate;
    return fail_low;
  }

  if (value >= *beta)
  {
    *alpha = -uo_score_checkmate;
    *beta = uo_score_checkmate;
    return fail_high;
  }

  if (value < -aspiration_window_mate)
  {
    *alpha = -uo_score_checkmate;
    *beta = -aspiration_window_mate;
    return exact;
  }

  if (value > aspiration_window_mate)
  {
    *alpha = aspiration_window_mate;
    *beta = uo_score_checkmate;
    return exact;
  }

  *alpha = value - aspiration_window;
  *beta = value + aspiration_window;
  return exact;
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
    .secondary_pvs = thread->secondary_pvs,
    .movetime_remaining_msec = INFINITY
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
    .secondary_pvs = thread->secondary_pvs,
    .movetime_remaining_msec = params->time_own ? params->time_own : INFINITY
  };

  uo_search_info *info = &thread->info;

  uo_move *line = thread->pv;
  line[0] = 0;

  // Start timer
  uo_time_now(&info->time_start);
  uo_time time_start_last_iteration = info->time_start;

  // Start thread for managing time
  uo_engine_run_thread(uo_engine_thread_start_timer, thread);

  // Load position
  uo_engine_thread_load_position(thread);

  // Initialize aspiration window
  int16_t alpha = params->alpha;
  int16_t beta = params->beta;

  int16_t value;
  uo_move bestmove = 0;
  uo_move tb_move = 0;
  size_t nodes_prev = 0;
  bool incomplete = false;

  // Check if new search is continuation to previous search
  bool is_continuation = engine.ponder.is_continuation = engine.ponder.key
    && position->stack[-1].key == engine.ponder.key;

  // Check if opponent played the expected move
  bool is_ponderhit = engine.ponder.is_ponderhit = is_continuation
    && position->stack[-1].move == engine.ponder.move;

  // Check if mate sequence is detected
  bool is_mate_found = is_continuation
    && engine.ponder.value > uo_score_mate_in_threshold;

  // Check for ponder hit
  if (is_continuation)
  {
    value = engine.ponder.value;
    int16_t window = uo_score_P;

    // If opponent played the expected move, initialize pv and use more narrow aspiration window
    if (is_ponderhit)
    {
      // Ponder hit
      assert(engine.pv[1] == engine.ponder.move);

      // Copy principal variation from previous search
      size_t i = 0;
      while (engine.pv[i + 2])
      {
        line[i] = engine.pv[i] = engine.pv[i + 2];
        ++i;
      }
      line[i] = engine.pv[i] = 0;

      // Use more narrow alpha-beta window
      window = uo_score_P / 3;
    }

    // Set aspiration window
    alpha = value > -uo_score_tb_win_threshold + window ? value - window : -uo_score_checkmate;
    beta = value < uo_score_tb_win_threshold - window ? value + window : uo_score_checkmate;

    // Probe transposition table
    uo_tdata tentry;
    bool found = uo_ttable_get(&engine.ttable, position->key, position->root_ply, &tentry);

    if (found)
    {
      // If entry type is exact or lower bound, let's initialize principal variation
      if (!line[0]
        && tentry.type != uo_score_type__upper_bound)
      {
        line[0] = tentry.bestmove;
        line[1] = 0;
      }

      // If entry type is exact and best move is known, let's update initial search depth
      if (tentry.type == uo_score_type__exact
        && tentry.bestmove)
      {
        info->depth = tentry.depth;
      }
    }
  }

  // Tablebase probe
  if (engine.tb.enabled
    // Do not probe tablebase if mate is detected
    && !is_mate_found)
  {
    size_t piece_count = uo_popcnt(position->own | position->enemy);
    size_t probe_limit = uo_min(TBlargest, engine.tb.probe_limit);

    // Do not probe if too many pieces are on the board
    if (piece_count <= probe_limit)
    {
      int success;
      int dtz = info->dtz = uo_tb_root_probe_dtz(position, &success);

      ++thread->info.tbhits;
      assert(success);

      // Flag position as tablebase position. This turns of tablebase probing during search.
      info->is_tb_position = true;

      // Clear hash table
      uo_engine_clear_hash();

      tb_move = position->movelist.head[0];
      if (tb_move != line[0])
      {
        line[0] = tb_move;
        line[1] = 0;
      }

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
        alpha = uo_max(1, alpha);
        beta = uo_min(-1, beta);
      }
    }
  }

  // Perform search for first depth
  value = uo_search_principal_variation(thread, info->depth, alpha, beta, line, false, &incomplete);

  // If search failed low perform re-search
  if (uo_search_adjust_alpha_beta(value, &alpha, &beta) < 0)
  {
    value = uo_search_principal_variation(thread, info->depth, alpha, beta, line, false, &incomplete);

    // If search failed low again, let's clear transposition table and start over
    if (uo_search_adjust_alpha_beta(value, &alpha, &beta) < 0)
    {
      info->depth = 1;
      uo_engine_clear_hash();
      value = uo_search_principal_variation(thread, info->depth, alpha, beta, line, false, &incomplete);
    }
  }

  // Save pv
  assert(incomplete || line[0]);
  if (!line[0])
  {
    line[0] = position->movelist.head[0];
    line[1] = 0;
  }

  bestmove = line[0];
  uo_pv_copy(engine.pv, line);

  // Save ponder move
  engine.ponder.key = uo_position_move_key(position, bestmove, NULL);
  engine.ponder.move = line[1];
  thread->info.value = value;

  // Stop search if mate in one
  if (value <= -uo_score_checkmate + 1
    || value == uo_score_checkmate)
  {
    goto search_completed;
  }

  // Stop search if not enough time left
  if (!incomplete
    && uo_time_elapsed_msec(&info->time_start) > info->movetime_remaining_msec / 2)
  {
    goto search_completed;
  }

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

  size_t fail_count;

  // Iterative deepening loop
  for (size_t depth = info->depth + 1; depth <= params->depth; ++depth)
  {
    // Stop search if not possible to improve
    if (value <= -uo_score_checkmate + (int16_t)depth - 1 || value >= uo_score_checkmate - (int16_t)depth - 2)
    {
      goto search_completed;
    }

    // Stop if not enough time to complete next search iteration
    double duration_last_iteration_msec = uo_time_elapsed_msec(&time_start_last_iteration);
    if (duration_last_iteration_msec > info->movetime_remaining_msec)
    {
      goto search_completed;
    }

    // Update search depth info
    thread->info.depth = lazy_smp_params.depth = depth;

    // Report search info
    if (thread->info.nodes)
    {
      uo_search_print_info(thread);
      nodes_prev = thread->info.nodes;
      thread->info.nodes = 0;
    }

    // Aspiration search loop
    fail_count = 0;

    do
    {
      // Start timer for current search iteration
      uo_time_now(&time_start_last_iteration);

      // Start parallel search on Lazy SMP threads if depth is sufficient and there are available threads
      bool can_delegate = !info->is_tb_position && (depth >= UO_LAZY_SMP_MIN_DEPTH) && (lazy_smp_count < lazy_smp_max_count);

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
          engine.ponder.key = uo_position_move_key(position, bestmove, NULL);
          engine.ponder.move = line[1];
        }
        else
        {
          uo_pv_copy(line, engine.pv);
        }

        goto search_completed;
      }

      // Check if search returned a value that is within aspiration window
      switch (uo_search_adjust_alpha_beta(value, &alpha, &beta))
      {
        // Fail-low
        case -1:
          // If search fails low too many times on the same depth, let's clear the transposition table and reduce depth.
          if (++fail_count > 3)
          {
            uo_engine_clear_hash();
            uo_search_print_info_string(thread, "hash cleared");
            depth /= 2;
            fail_count = 1;
          }
          break;

          // Exact
        case 0:
          assert(line[0]);
          fail_count = 0;
          break;

          // Fail-high
        case 1:
          // Let's advance to next depth even if the search fails high
          assert(line[0]);
          fail_count = 0;
          break;
      }

    } while (fail_count);

    // We should always have a best move if search was successful
    bestmove = line[0];
    assert(bestmove);

    uo_pv_copy(engine.pv, line);
    engine.ponder.key = uo_position_move_key(position, bestmove, NULL);
    engine.ponder.move = line[1];
    thread->info.value = value;
  }

search_completed:

  // If tablebase position, adjust score and verify that the best move is not skipped move
  if (tb_move)
  {
    if (info->dtz > 0)
    {
      value = uo_max(uo_score_tb_win_threshold, value);
    }
    else if (info->dtz < 0)
    {
      value = uo_min(-uo_score_tb_win_threshold, value);
    }
    else
    {
      value = 0;
    }

    thread->info.value = value;

    if (uo_position_is_skipped_move(position, bestmove))
    {
      bestmove = line[0] = tb_move;
      line[1] = 0;

      uo_pv_copy(engine.pv, line);
      engine.ponder.key = uo_position_move_key(position, bestmove, NULL);
      engine.ponder.move = line[1];
    }
  }

  engine.ponder.value = thread->info.value;
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
  uint64_t nps = (double)thread->info.nodes / time_msec * 1000.0;

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
    thread->info.nodes, nps, hashfull, (int)thread->info.tbhits, time_msec);

  printf("\n");

  uo_engine_unlock_stdout();

  uo_engine_stop_search();

  return NULL;
}
