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

inline static uint16_t uo_search_calculate_move_score(uo_engine_thread *thread, uo_move move, uo_search_info *info)
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
    return 2000 + uo_position_capture_gain(&thread->position, move);
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

  //// If it is our turn, then let's be more heavy handed with pruning moves
  //int8_t pruning_threshold = (uo_color(position->ply) == uo_color_own ? -100 : -200) + position->ply;

  //if (pruning_threshold < 0) pruning_threshold = 0;

  size_t j = move_count;
  size_t i = lo;

  while (i < move_count)
  {
    uo_move move = movelist[i];
    uint16_t score = uo_search_calculate_move_score(thread, move, info);
    position->movelist.move_scores[i] = score;
    ++i;
  }

  //do
  //{
  //  uo_move move = movelist[i];
  //  int8_t score = uo_search_calculate_move_score(thread, move, info);

  //  if (score >= pruning_threshold)
  //  {
  //    position->movelist.move_scores[i] = score;
  //    ++i;
  //  }
  //  else
  //  {
  //    --j;
  //    position->movelist.move_scores[j] = score;
  //    movelist[i] = movelist[j];
  //    movelist[j] = move;
  //  }
  //} while (i < j);

  //while (j < 10)
  //{
  //  i = j;
  //  j = move_count;
  //  pruning_threshold += 2;

  //  do
  //  {
  //    uo_move move = movelist[i];
  //    int8_t score = position->movelist.move_scores[i];

  //    if (score >= pruning_threshold)
  //    {
  //      ++i;
  //    }
  //    else
  //    {
  //      --j;
  //      position->movelist.move_scores[j] = score;
  //      movelist[i] = movelist[j];
  //      movelist[j] = move;
  //    }
  //  } while (i < j);
  //}

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

  ++info->nodes;

  if (info->seldepth < position->ply)
  {
    info->seldepth = position->ply;
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

  int16_t value = -UO_SCORE_CHECKMATE;

  if (uo_position_is_check(&thread->position))
  {
    for (int64_t i = 0; i < move_count; ++i)
    {
      uo_move move = position->movelist.head[i];

      uo_position_make_move(position, move);
      int16_t node_value = -uo_search_quiesce(thread, -beta, -alpha, info);
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
    }

    return value;
  }

  bool is_queiscent = true;

  for (size_t i = 0; i < move_count; ++i)
  {
    uo_move move = position->movelist.head[i];

    bool is_tactical_move = uo_move_is_promotion(move)
      || (uo_move_is_capture(move) && uo_position_capture_gain(position, move) >= 0);

    if (!is_tactical_move)
    {
      continue;
    }

    is_queiscent = false;

    uo_position_make_move(position, move);
    int16_t node_value = -uo_search_quiesce(thread, -beta, -alpha, info);
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
  }

  if (is_queiscent)
  {
    return uo_position_evaluate(position);
  }

  return value;
}

// see: https://en.wikipedia.org/wiki/Negamax#Negamax_with_alpha_beta_pruning_and_transposition_tables
// see: https://en.wikipedia.org/wiki/Principal_variation_search
static int16_t uo_search_principal_variation(uo_engine_thread *thread, size_t depth, int16_t alpha, int16_t beta, uo_search_info *info)
{
  uo_position *position = &thread->position;
  int16_t alpha_original = alpha;

  uo_engine_lock_ttable();
  uo_tentry *entry = uo_ttable_get(&engine.ttable, position);

  if (entry && entry->depth >= depth)
  {
    int16_t value = entry->value;
    uo_engine_unlock_ttable();

    switch (entry->type)
    {
      case uo_tentry_type__exact:
        return value;

      case uo_tentry_type__alpha:
        if (value > alpha)
        {
          alpha = value;
        }

        break;

      case uo_tentry_type__beta:
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
    ++info->nodes;
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
    // depth * 3/4
    size_t depth_nmp = depth * 3 >> 2;
    int16_t pass_value = -uo_search_principal_variation(thread, depth_nmp, -beta, -beta + 1, info);
    uo_position_unmake_null_move(position);

    if (pass_value >= beta)
    {
      return pass_value;
    }
  }

  uo_search_sort_and_prune_moves(thread, info);

  if (depth == 0)
  {
    // Search depth reached. Let's perform quiescence search.

    int16_t static_evaluation = uo_position_evaluate(position);
    const int16_t stand_pat_margin = -200;
    int16_t stand_pat = static_evaluation + stand_pat_margin;

    if (stand_pat >= beta)
    {
      return beta;
    }

    if (stand_pat > alpha)
    {
      alpha = stand_pat;
    }

    int16_t value = -UO_SCORE_CHECKMATE;

    for (size_t i = 0; i < move_count; ++i)
    {
      uo_move move = position->movelist.head[i];
      uo_position_make_move(position, move);
      int16_t node_value = -uo_search_quiesce(thread, -beta, -alpha, info);
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
    }

    return value;
  }

  uo_search_stop_if_movetime_over(thread, info);

  // search first move with full alpha/beta window
  uo_move bestmove = position->movelist.head[0];
  ++position->bftable[uo_color(position->flags)][bestmove & 0xFFF];
  uo_position_make_move(position, bestmove);
  int16_t value = -uo_search_principal_variation(thread, depth - 1, -beta, -alpha, info);
  value = uo_score_adjust_for_mate(value);
  uo_position_unmake_move(position);

  if (value > alpha)
  {
    position->hhtable[uo_color(position->flags)][bestmove & 0xFFF] += 2 << depth;
    alpha = value;
  }

  if (alpha >= beta)
  {
    if (!uo_move_is_capture(bestmove))
    {
      position->stack->search.killers[1] = position->stack->search.killers[0];
      position->stack->search.killers[0] = bestmove;
    }
  }
  else
  {
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
      int16_t node_value = -uo_search_principal_variation(thread, depth_lmr - 1, -alpha - 1, -alpha, info);
      node_value = uo_score_adjust_for_mate(node_value);

      if (alpha < node_value && node_value < beta)
      {
        // failed high, do a full re-search
        node_value = -uo_search_principal_variation(thread, depth_lmr - 1, -beta, -alpha, info);
        node_value = uo_score_adjust_for_mate(node_value);
      }

      uo_position_unmake_move(position);

      if (node_value > value)
      {
        value = node_value;
        bestmove = move;

        if (value > alpha)
        {
          position->hhtable[uo_color(position->flags)][move & 0xFFF] += 2 << depth_lmr;
          alpha = value;

          if (alpha >= beta)
          {
            if (!uo_move_is_capture(move))
            {
              position->stack->search.killers[1] = position->stack->search.killers[0];
              position->stack->search.killers[0] = move;
            }

            break;
          }
        }
      }

      if (uo_engine_is_stopped())
      {
        return value;
      }
    }
  }

  uo_engine_lock_ttable();

  if (!entry)
  {
    entry = uo_ttable_set(&engine.ttable, position);
  }

  entry->depth = depth;
  entry->bestmove = bestmove;
  entry->value = value;

  if (alpha >= beta)
  {
    entry->type = uo_tentry_type__beta;
  }
  else if (alpha != alpha_original)
  {
    entry->type = uo_tentry_type__alpha;
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

    uo_position_print_move(position, info->bestmove, buf);
    printf("nodes %" PRIu64 " nps %" PRIu64 " hashfull %" PRIu64 " tbhits %d time %.0f pv %s",
      info->nodes, nps, hashfull, info->tbhits, time_msec, buf);

    size_t pv_len = 1;

    size_t i = 1;

    uo_position_make_move(position, info->bestmove);

    uo_engine_lock_ttable();
    uo_tentry *next_entry = uo_ttable_get(&engine.ttable, position);
    uo_tentry pv_entry;
    if (next_entry) pv_entry = *next_entry;
    uo_engine_unlock_ttable();

    for (; i < info->depth && next_entry; ++i)
    {
      uo_position_print_move(position, pv_entry.bestmove, buf);
      printf(" %s", buf);

      if (!uo_position_is_legal_move(position, pv_entry.bestmove))
      {
        uo_position_print_move(position, pv_entry.bestmove, buf);
        printf("\n\nillegal bestmove %s\n", buf);
        uo_position_print_diagram(position, buf);
        printf("\n%s", buf);
        uo_position_print_fen(position, buf);
        printf("\n");
        printf("Fen: %s\n", buf);
        printf("Key: %" PRIu64 "\n", position->key);
        fflush(stdout);
        assert(false);
      }

      uo_position_make_move(position, pv_entry.bestmove);

      uo_engine_lock_ttable();
      next_entry = uo_ttable_get(&engine.ttable, position);
      if (next_entry) pv_entry = *next_entry;
      uo_engine_unlock_ttable();
    }

    while (i--)
    {
      uo_position_unmake_move(position);
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

void *uo_engine_thread_run_principal_variation_search(void *arg)
{
  uo_engine_thread_work *work = arg;
  uo_engine_thread *thread = work->thread;
  uo_position *position = &thread->position;
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
  uo_search_principal_variation(thread, 1, -UO_SCORE_CHECKMATE, UO_SCORE_CHECKMATE, &info);

  uo_engine_lock_ttable();
  uo_tentry *pv_entry = uo_ttable_get(&engine.ttable, position);
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
    uo_search_principal_variation(thread, depth, -UO_SCORE_CHECKMATE, UO_SCORE_CHECKMATE, &info);

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

  uo_engine_lock_position();
  for (size_t i = 0; i < 4096; ++i)
  {
    engine.position.hhtable[0][i] = position->hhtable[0][i] >> 1;
    engine.position.bftable[0][i] = position->bftable[0][i] >> 1;
  }
  uo_engine_unlock_position();

  uo_engine_stop_search();

  info.params = NULL;
  free(params);
  return NULL;
}
