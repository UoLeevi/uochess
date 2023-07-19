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

    // The minimum time that should be left on the clock is one second
    movetime = uo_max(1, uo_min(movetime, (int64_t)params->time_own - 1000));
  }

  if (movetime)
  {
    const double margin_msec = engine_options.move_overhead / 2;
    double time_msec = uo_time_elapsed_msec(&thread->info.time_start);

    info->movetime_remaining_msec = (double)movetime - time_msec - margin_msec;

    if (info->movetime_remaining_msec < 0)
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

static inline int uo_search_determine_depth_reduction_or_extension(uo_engine_thread *thread, size_t move_num, size_t depth)
{
  // Step 1. Initialize variables
  uo_search_info *info = &thread->info;
  uo_position *position = &thread->position;
  uo_move_history *stack = position->stack;
  bool is_check = uo_position_is_check(position);
  size_t move_count = stack[-1].move_count;
  uo_move move = stack[-1].move;
  uo_square square_from = uo_move_square_from(move) ^ 56;
  uo_square square_to = uo_move_square_to(move) ^ 56;
  const uo_piece *board = position->board;
  uo_piece piece = board[square_to] ^ 1;

  // Step 2. Limit search depth extensions to prevent search explosion.
  int net_extension_plies = position->ply + depth - info->depth;
  int max_extension = uo_max(0, (info->depth - uo_max(0, net_extension_plies)) * 1000);

  // Step 3. Determine depth extension
  int extension = 0;

  // Passed pawn near promotion extension
  if (piece == uo_piece__P
    && depth < 6
    && (square_to ^ 56) >= uo_square__a7)
  {
    extension += 1000;
  }

  // Single-response extension
  if (move_count == 1) extension += 1000;

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
  int max_reduction = uo_min(depth * 1000 / 3, info->depth * 1000 * 3 / 5 - uo_max(0, -net_extension_plies * 1000));
  if (max_reduction <= 0) return 0;

  if (
    // no reduction for shallow depth
    depth <= 3
    // no reduction for first few moves
    || move_num <= 3
    // no reduction if there are very few legal moves
    || move_count < 6
    // no reduction on queen or knight promotions
    || uo_move_is_promotion_Q_or_N(move)
    // no reduction on killer moves
    || uo_position_is_killer_move(position, 0))
  {
    return 0;
  }

  // Bitboards to detetermine defended squares
  uo_bitboard mask_own = position->enemy;
  uo_bitboard mask_enemy = position->own;
  uo_bitboard occupied = mask_own | mask_enemy;
  uo_bitboard empty = ~occupied;

  uo_bitboard enemy_P = mask_enemy & position->P;
  uo_bitboard enemy_N = mask_enemy & position->N;
  uo_bitboard enemy_B = mask_enemy & position->B;
  uo_bitboard enemy_R = mask_enemy & position->R;
  uo_bitboard enemy_Q = mask_enemy & position->Q;
  uo_bitboard enemy_K = mask_enemy & position->K;

  uo_bitboard attacks_left_enemy_P = uo_bitboard_attacks_left_P(enemy_P);
  uo_bitboard attacks_right_enemy_P = uo_bitboard_attacks_right_P(enemy_P);
  uo_bitboard attacks_enemy_P = attacks_left_enemy_P | attacks_right_enemy_P;

  bool is_square_attacked_by_P = attacks_enemy_P & uo_square_bitboard(square_to);
  bool is_square_attacked_by_N = uo_bitboard_attacks_N(square_to) & enemy_N;
  bool is_square_attacked_by_B = uo_bitboard_attacks_B(square_to, occupied) & enemy_B;
  bool is_square_attacked_by_R = uo_bitboard_attacks_R(square_to, occupied) & enemy_R;
  bool is_square_attacked_by_Q = uo_bitboard_attacks_Q(square_to, occupied) & enemy_Q;
  bool is_square_attacked_by_K = uo_bitboard_attacks_K(square_to) & enemy_K;

  bool is_square_attacked =
    is_square_attacked_by_P
    || is_square_attacked_by_N
    || is_square_attacked_by_B
    || is_square_attacked_by_R
    || is_square_attacked_by_Q
    || is_square_attacked_by_K;

  // no reduction on captures of undefended pieces
  if (uo_move_is_capture(move) && !is_square_attacked) return 0;

  // Default reduction is one fifth of the depth but capped at two plies
  int reduction = uo_min(depth * 1000 / 5, 2000);

  // Reduction for stepping into defended square
  reduction += is_square_attacked * 500;

  // Reduction based on move ordering
  reduction += move_num * 1000 / move_count;

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

  // Step 5. Mate distance pruning
  alpha = uo_max(-score_checkmate, alpha);
  beta = uo_min(score_checkmate + 1, beta);
  if (alpha >= beta) return alpha;

  // Step 6. Lookup position from transposition table and return if exact score for equal or higher depth is found
  uo_abtentry entry = { alpha, beta, 0 };
  if (uo_engine_lookup_entry(position, &entry))
  {
    // Unless draw by 50 move rule is a possibility, let's return transposition table score
    if (rule50 < 90) return entry.value;
  }

  // Step 7. If search is stopped, return unknown value
  if (uo_engine_thread_is_stopped(thread))
  {
    *incomplete = true;
    return uo_score_unknown;
  }

  // Step 8. If maximum search depth is reached return static evaluation
  if (uo_position_is_max_depth_reached(position)) return uo_position_evaluate_and_cache(position, NULL, thread->move_cache);

  // Step 9. Generate legal moves
  size_t move_count = stack->moves_generated
    ? stack->move_count
    : uo_position_generate_moves(position);

  // Step 10. If there are no legal moves, return draw or checkmate
  if (move_count == 0) return is_check ? -score_checkmate : 0;

  // Step 11. If position is check, perform quiesence search for all moves
  if (is_check)
  {
    // Step 11.1. Initialize score to be checkmate
    int16_t value = -score_checkmate;

    // Step 11.2. Sort moves
    uo_position_sort_moves(&thread->position, entry.data.bestmove, thread->move_cache);

    // Step 11.3. Search each move
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

  // Step 12. Position is not check. Initialize score to static evaluation. "Stand pat"
  uo_evaluation_info eval_info;
  int16_t static_eval = uo_position_evaluate_and_cache(position, &eval_info, thread->move_cache);
  assert(stack->static_eval != uo_score_unknown);

  // Step 13. Adjust value if position was found from transposition table
  int16_t value =
    entry.data.type == uo_score_type__lower_bound ? uo_max(entry.data.value, static_eval) :
    entry.data.type == uo_score_type__upper_bound ? uo_min(entry.data.value, static_eval) : // <- this might be a bit dubious
    static_eval;

  // Step 14. Cutoff if static evaluation is higher or equal to beta.
  if (value >= beta) return beta;

  // Step 15. Update alpha
  alpha = uo_max(alpha, value);

  // Step 16. Determine futility threshold.
  // If move has low chance of raising alpha based on static exchange evaluation, do not search the move.
  const int16_t futility_margin = uo_score_P - uo_score_mul_ln(uo_score_P, depth);
  const int16_t futility_threshold = alpha - static_eval - futility_margin;

  // Step 16. Sort tactical moves
  uo_position_sort_tactical_moves(position, thread->move_cache);

  // Step 17. Search tactical moves which have potential to raise alpha and checks depending on depth
  for (size_t i = 0; i < move_count; ++i)
  {
    uo_move move = position->movelist.head[i];

    // Step 17.1. For first couple plies, search all checks that are not immediately recapturable
    // or if they have some possibility to raise alpha
    uo_bitboard checks = uo_position_move_checks(position, move, thread->move_cache);
    if (checks)
    {
      int16_t futility_threshold_for_check = depth < UO_QS_CHECKS_DEPTH
        ? uo_min(0, futility_threshold - uo_score_P)
        : futility_threshold - uo_score_P;

      if (uo_position_move_see_gt(position, move, futility_threshold_for_check, thread->move_cache))
      {
        goto search_move;
      }
    }

    // Step 17.2. Search queen and knight promotions
    else if (uo_move_is_promotion_Q_or_N(move))
    {
      goto search_move;
    }

    // Step 17.3. Futility pruning for captures
    else if (uo_move_is_capture(move)
      && uo_position_move_see_gt(position, move, futility_threshold, thread->move_cache))
    {
      goto search_move;
    }

    // Step 17.4. Examine potential advanced pawn pushes
    else if (uo_position_move_piece(position, move) == uo_piece__P
      && uo_move_square_to(move) >= uo_square__a6
      && uo_position_move_see_gt(position, move, futility_threshold, thread->move_cache))
    {
      goto search_move;
    }

    // Do not search the move
    continue;

    // Search move
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
#define uo_allocate_line(depth) uo_alloca(uo_max((depth + 2) * 2, UO_PV_ALLOC_MIN_LENGTH) * sizeof(uo_move))

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
    do
    {
      // If draw by 50 move rule is a possibility, let's continue search
      if (rule50 > 90) break;

      // For root node, in case the position is in tablebase, let's verify that the tt move is preserving win or draw.
      if (is_root_node
        && info->is_tb_position
        && entry.bestmove != pline[0]
        && uo_position_is_skipped_move(position, entry.bestmove))
      {
        break;
      }

      // Let's update principal variation line if transposition table score is better than current best score but not better than beta
      if (pline
        && entry.value > alpha
        && entry.value < beta
        && entry.bestmove)
      {
        uo_position_extend_pv(position, entry.bestmove, pline, depth);
      }

      // On root node, best move is required
      if (!is_root_node || entry.bestmove) return entry.value;

    } while (false);
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
  if (uo_position_is_max_depth_reached(position)) return uo_position_evaluate_and_cache(position, NULL, thread->move_cache);

  entry.value = -score_checkmate;

  // Step 10. Tablebase probe
  // Do not probe on root node or if search depth is too shallow
  // Do not probe if 50 move rule, castling or en passant can interfere with the table base result
  if (engine.tb.enabled
    && !is_root_node
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

      if (wdl > engine.tb.score_wdl_draw)
      {
        entry.value = score_tb_win - 101;
        if (entry.value >= beta) return entry.value;
      }
      else if (wdl < -engine.tb.score_wdl_draw)
      {
        entry.value = -score_tb_win + 101;
        if (entry.value <= alpha) return entry.value;
      }
      else
      {
        return uo_score_draw;
      }

      alpha = uo_max(alpha, entry.value);
    }
  }

  // Step 11. Static evaluation and calculation of improvement margin
  uo_evaluation_info eval_info;
  int16_t static_eval = uo_position_evaluate_and_cache(position, &eval_info, thread->move_cache);
  assert(is_check || static_eval != uo_score_unknown);

  bool is_expected_cut = !is_root_node && (static_eval >= beta || entry.data.type == uo_score_type__lower_bound);

  bool is_improving
    = stack[-2].checks == uo_move_history__checks_none ? static_eval > stack[-2].static_eval
    : stack[-4].checks == uo_move_history__checks_none ? static_eval > stack[-4].static_eval
    : true;

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
    && is_expected_cut
    && depth > 3
    && position->ply > 1 + thread->nmp_min_ply
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

  // Step 18. Perform full alpha-beta search for the first move
  entry.depth = depth;

  // On main thread, root node, let's report current move on higher depths
  if (is_main_thread
    && is_root_node
    && uo_time_elapsed_msec(&thread->info.time_start) > 3000)
  {
    uo_search_print_currmove(thread, move, 1);
  }

  // Search extensions
  uo_position_make_move(position, move);
  size_t depth_extension = uo_max(0, uo_search_determine_depth_reduction_or_extension(thread, 0, depth));
  int16_t node_value = -uo_search_principal_variation(thread, depth + depth_extension - 1, -beta, -alpha, line, false, incomplete);
  uo_position_unmake_move(position);

  if (*incomplete) return uo_score_unknown;

  if (node_value >= entry.value)
  {
    entry.value = node_value;
    entry.bestmove = move;

    if (entry.value > alpha)
    {
      if (entry.value >= beta)
      {
        uo_position_update_cutoff_history(position, 0, depth);
        return uo_engine_store_entry(position, &entry);
      }

      alpha = entry.value;
      uo_position_update_pv(position, pline, entry.bestmove, line);
    }
  }

  // Step 19. Initialize parallel search parameters and result queue if applicable
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

  // Step 20. Search rest of the moves with zero window and reduced depth unless they fail-high
  for (size_t i = 1; i < move_count; ++i)
  {
    uo_move move = params.move = position->movelist.head[i];

    // On main thread, root node, let's report current move on higher depths
    if (is_main_thread
      && is_root_node
      && uo_time_elapsed_msec(&thread->info.time_start) > 3000)
    {
      uo_search_print_currmove(thread, move, i + 1);
    }

    // Step 20.1 Reset pv line
    line[0] = 0;

    // Step 20.2 Determine search depth extension or reduction
    uo_position_make_move(position, move);
    size_t depth_extension_or_reduction = uo_search_determine_depth_reduction_or_extension(thread, i, depth);
    int16_t node_value = alpha + 1;
    size_t depth_lmr = depth + depth_extension_or_reduction;


    // Step 20.3 Perform zero window search for reduced depth
    if (is_zw_search || depth_extension_or_reduction < 0)
    {
      node_value = -uo_search_principal_variation(thread, depth_lmr - 1, -alpha - 1, -alpha, line, !cut, incomplete);

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

      size_t depth_extension = uo_max(0, depth_extension_or_reduction);
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
        if (entry.value >= beta)
        {
          uo_position_update_cutoff_history(position, i, depth);
          if (parallel_search_count > 0) uo_search_cutoff_parallel_search(thread, &params.queue, NULL);
          return uo_engine_store_entry(position, &entry);
        }

        alpha = entry.value;
        params.beta = -alpha;
        uo_position_update_pv(position, pline, entry.bestmove, line);

        if (is_root_node && is_main_thread)
        {
          thread->info.bestmove_change_depth = depth_lmr;
          if (uo_time_elapsed_msec(&thread->info.time_start) > 3000) uo_search_print_info(thread);
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
            uo_position_update_pv(position, pline, entry.bestmove, result.line);

            if (is_root_node && is_main_thread)
            {
              thread->info.bestmove_change_depth = depth_lmr;
              if (uo_time_elapsed_msec(&thread->info.time_start) > 3000) uo_search_print_info(thread);
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
          uo_position_update_pv(position, pline, entry.bestmove, result.line);

          if (is_root_node && is_main_thread)
          {
            thread->info.bestmove_change_depth = depth_lmr;
            if (uo_time_elapsed_msec(&thread->info.time_start) > 3000) uo_search_print_info(thread);
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
    if (*alpha == -uo_score_checkmate)
    {
      return true;
    }

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
    if (*beta == -uo_score_checkmate)
    {
      return true;
    }

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
    .movetime_remaining_msec = INFINITY
  };

  uo_search_info *info = &thread->info;

  uo_move *line = thread->pv;
  line[0] = 0;

  // Start timer
  uo_time_now(&info->time_start);
  uo_time time_start_last_iteration = info->time_start;

  // Load position
  uo_engine_thread_load_position(thread);

  // Initialize aspiration window
  int16_t alpha = params->alpha;
  int16_t beta = params->beta;
  size_t aspiration_fail_count = 0;

  int16_t value;
  uo_move bestmove = 0;
  uo_move tb_move = 0;
  size_t nodes_prev = 0;
  bool incomplete = false;

  // Check for ponder hit
  if (engine.ponder.key && position->stack[-1].key == engine.ponder.key)
  {
    value = engine.ponder.value;
    int16_t window = uo_score_P;

    // If opponent played the expected move, initialize pv and use more narrow aspiration window
    bool is_ponderhit = position->stack[-1].move == engine.ponder.move;

    if (is_ponderhit)
    {
      // Ponder hit
      assert(engine.pv[1] == engine.ponder.move);

      // Use a more narrow aspiration window
      window = uo_score_P / 3;

      // Copy principal variation from previous search
      size_t i = 0;
      while (engine.pv[i + 2])
      {
        line[i] = engine.pv[i] = engine.pv[i + 2];
        ++i;
      }
      line[i] = engine.pv[i] = 0;
    }

    alpha = value > -uo_score_tb_win_threshold + window ? value - window : -uo_score_checkmate;
    beta = value < uo_score_tb_win_threshold - window ? value + window : uo_score_checkmate;

    // Probe transposition table
    uo_tdata tentry;
    bool found = uo_ttable_get(&engine.ttable, position, &tentry);

    if (found)
    {
      // If entry type is exact, let's update initial search depth and principal variation
      if (tentry.type == uo_score_type__exact)
      {
        info->depth = tentry.depth;
      }

      // Tighten the alpha-beta boundaries
      else if (tentry.type == uo_score_type__lower_bound)
      {
        alpha = uo_max(alpha, tentry.value);
        beta = uo_max(beta, alpha + window);
      }
      else // if (tentry.type == uo_score_type__upper_bound)
      {
        beta = uo_min(beta, tentry.value);
        alpha = uo_min(alpha, beta - window);
      }
    }
  }

  // Tablebase probe
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
        int dtz = info->dtz = uo_tb_root_probe_dtz(position, &success);

        ++thread->info.tbhits;
        assert(success);

        info->is_tb_position = true;

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
  }

  // Perform search for first depth
  do
  {
    value = uo_search_principal_variation(thread, info->depth, alpha, beta, line, false, &incomplete);
  } while (!info->is_tb_position && !uo_search_adjust_alpha_beta(value, &alpha, &beta, &aspiration_fail_count));

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
  if (value <= -uo_score_checkmate + 1
    || value == uo_score_checkmate)
  {
    goto search_completed;
  }

  // Stop search if not enough time left
  if (params->time_own
    && uo_time_elapsed_msec(&info->time_start) > params->time_own / 2)
  {
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
      if (info->is_tb_position || uo_search_adjust_alpha_beta(value, &alpha, &beta, &aspiration_fail_count))
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

    engine.ponder.value = thread->info.value = value;

    if (uo_position_is_skipped_move(position, bestmove))
    {
      bestmove = line[0] = tb_move;
      line[1] = 0;

      uo_pv_copy(engine.pv, line);
      uo_position_make_move(position, bestmove);
      engine.ponder.key = position->key;
      engine.ponder.move = line[1];
      uo_position_unmake_move(position);
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
