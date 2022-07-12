#include "uo_search.h"
#include "uo_position.h"
#include "uo_evaluation.h"
#include "uo_thread.h"
#include "uo_global.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <assert.h>

int search_id;

const size_t quicksort_depth = 4; // arbitrary

void uo_search_init(uo_search *search, uo_search_init_options *options)
{
  // threads
  search->thread_count = options->threads;
  search->threads = calloc(search->thread_count, sizeof * search->threads);
  search->main_thread = search->threads;

  for (size_t i = 0; i < search->thread_count; ++i)
  {
    uo_search_thread *thread = search->threads + i;
    thread->search = search;
    thread->head = thread->moves;
  }

  // hash table
  size_t capacity = options->hash_size / sizeof * search->ttable.entries;
  uo_ttable_init(&search->ttable, uo_msb(capacity) + 1);

  // multipv
  search->pv = malloc(options->multipv * sizeof * search->pv);
}

static int16_t uo_search_quiesce(uo_search_thread *thread, int16_t a, int16_t b, int64_t move_count)
{
  int16_t stand_pat = uo_position_evaluate(&thread->position);

  if (stand_pat >= b)
  {
    return b;
  }

  if (a < stand_pat)
  {
    a = stand_pat;
  }

  thread->head += move_count;

  for (int64_t i = 0; i < move_count; ++i)
  {
    uo_move move = thread->head[i - move_count];

    if (uo_move_is_capture(move) || uo_move_is_promotion(move))
    {
      uo_position_make_move(&thread->position, move);
      int64_t next_move_count = uo_position_get_moves(&thread->position, thread->head);

      if (next_move_count == 0)
      {
        thread->head -= move_count;
        uo_position_unmake_move(&thread->position);
        return uo_position_is_check(&thread->position) ? -UO_SCORE_CHECKMATE : 0;
      }

      int16_t node_value = -uo_search_quiesce(thread, -b, -a, next_move_count);
      uo_position_unmake_move(&thread->position);

      if (node_value > UO_SCORE_MATE_IN_THRESHOLD)
      {
        --node_value;
      }
      else if (node_value < -UO_SCORE_MATE_IN_THRESHOLD)
      {
        ++node_value;
      }

      if (node_value >= b)
      {
        thread->head -= move_count;
        return b;
      }

      if (node_value > a)
      {
        a = node_value;
      }
    }
  }

  thread->head -= move_count;

  return a;
}

static int uo_search_moves_cmp(uo_search_thread *thread, uo_move move_lhs, uo_move move_rhs)
{
  uo_square square_from_lhs = uo_move_square_from(move_lhs);
  uo_square square_to_lhs = uo_move_square_to(move_lhs);
  uo_square move_type_lhs = uo_move_get_type(move_lhs);
  int score_lhs = 0;

  if (move_type_lhs & uo_move_type__x)
  {
    uo_piece piece_lhs = thread->position.board[square_from_lhs];
    uo_piece piece_captured_lhs = thread->position.board[square_to_lhs];
    score_lhs = uo_piece_type(piece_captured_lhs) - uo_piece_type(piece_lhs) + uo_piece__B;
  }

  if (uo_bitboard_attacks_enemy_P(thread->position.P & thread->position.enemy) & uo_square_bitboard(square_to_lhs))
  {
    score_lhs -= 1;
  }
  else if (uo_position_is_check_move(&thread->position, move_lhs))
  {
    score_lhs += 2;
  }

  uo_square square_from_rhs = uo_move_square_from(move_rhs);
  uo_square square_to_rhs = uo_move_square_to(move_rhs);
  uo_square move_type_rhs = uo_move_get_type(move_rhs);
  int score_rhs = 0;

  if (move_type_rhs & uo_move_type__x)
  {
    uo_piece piece_rhs = thread->position.board[square_from_rhs];
    uo_piece piece_captured_rhs = thread->position.board[square_to_rhs];
    score_rhs = uo_piece_type(piece_captured_rhs) - uo_piece_type(piece_rhs) + uo_piece__B;
  }

  if (uo_bitboard_attacks_enemy_P(thread->position.P & thread->position.enemy) & uo_square_bitboard(square_to_rhs))
  {
    score_rhs -= 1;
  }
  else if (uo_position_is_check_move(&thread->position, move_rhs))
  {
    score_rhs += 2;
  }

  return score_lhs == score_rhs
    ? 0
    : score_lhs > score_rhs
    ? -1
    : 1;
}

static int uo_search_partition_moves(uo_search_thread *thread, uo_move *movelist, int lo, int hi)
{
  int mid = (lo + hi) >> 1;
  uo_move temp;

  if (uo_search_moves_cmp(thread, movelist[mid], movelist[lo]) < 0)
  {
    temp = movelist[mid];
    movelist[mid] = movelist[lo];
    movelist[lo] = temp;
  }

  if (uo_search_moves_cmp(thread, movelist[hi], movelist[lo]) < 0)
  {
    temp = movelist[hi];
    movelist[hi] = movelist[lo];
    movelist[lo] = temp;
  }

  if (uo_search_moves_cmp(thread, movelist[mid], movelist[hi]) < 0)
  {
    temp = movelist[mid];
    movelist[mid] = movelist[hi];
    movelist[hi] = temp;
  }

  uo_move pivot = movelist[hi];

  for (int j = lo; j < hi - 1; ++j)
  {
    if (uo_search_moves_cmp(thread, movelist[j], pivot) < 0)
    {

      temp = movelist[lo];
      movelist[lo] = movelist[j];
      movelist[j] = temp;
      ++lo;
    }
  }

  temp = movelist[lo];
  movelist[lo] = movelist[hi];
  movelist[hi] = temp;
  return lo;
}

// Limited quicksort which prioritizes ordering beginning of movelist
static void uo_search_quicksort_moves(uo_search_thread *thread, uo_move *movelist, int lo, int hi, int depth)
{
  if (lo >= hi || depth <= 0)
  {
    return;
  }

  int p = uo_search_partition_moves(thread, movelist, lo, hi);
  uo_search_quicksort_moves(thread, movelist, lo, p - 1, depth - 1);
  uo_search_quicksort_moves(thread, movelist, p + 1, hi, depth - 2);
}

static void uo_search_sort_moves(uo_search_thread *thread, int64_t move_count)
{
  uo_tentry *entry = uo_ttable_get(&thread->search->ttable, thread->position.key);
  uo_move *movelist = thread->head;

  int lo = 0;

  // If found, set pv move as first
  if (entry)
  {
    uo_move move_pv = entry->bestmove;

    for (int i = 1; i < move_count; ++i)
    {
      if (movelist[i] == move_pv)
      {
        movelist[i] = movelist[0];
        movelist[0] = move_pv;
        lo = 1;
        break;
      }
    }
  }

  uo_search_quicksort_moves(thread, movelist, lo, move_count - 1, quicksort_depth);
}

// see: https://en.wikipedia.org/wiki/Negamax#Negamax_with_alpha_beta_pruning_and_transposition_tables
static int16_t uo_search_negamax(uo_search_thread *thread, size_t depth, int16_t a, int16_t b, int16_t color)
{
  int16_t a_orig = a;

  uo_tentry *entry = uo_ttable_get(&thread->search->ttable, thread->position.key);
  if (entry && entry->depth >= depth)
  {
    int16_t score = color * entry->score;

    switch (entry->type)
    {
      case uo_tentry_type__exact:
        return score;

      case uo_tentry_type__lower_bound:
        a = score > a ? score : a;
        break;

      case uo_tentry_type__upper_bound:
        b = score < b ? score : b;
        break;

      default:
        assert(false);
    }

    if (a >= b)
    {
      return score;
    }
  }

  int64_t move_count = uo_position_get_moves(&thread->position, thread->head);

  if (move_count == 0)
  {
    return uo_position_is_check(&thread->position) ? -UO_SCORE_CHECKMATE : 0;
  }

  if (depth == 0)
  {
    return uo_search_quiesce(thread, a, b, move_count);
  }

  uo_search_sort_moves(thread, move_count);
  thread->head += move_count;

  uo_move bestmove;
  int16_t value = -UO_SCORE_CHECKMATE;

  for (int64_t i = 0; i < move_count; ++i)
  {
    uo_move move = thread->head[i - move_count];
    uo_position_make_move(&thread->position, move);
    int16_t node_value = -uo_search_negamax(thread, depth - 1, -b, -a, -color);
    uo_position_unmake_move(&thread->position);

    if (node_value > UO_SCORE_MATE_IN_THRESHOLD)
    {
      --node_value;
    }
    else if (node_value < -UO_SCORE_MATE_IN_THRESHOLD)
    {
      ++node_value;
    }

    if (node_value > value)
    {
      value = node_value;
      bestmove = move;

      if (value > a)
      {
        a = value;
      }
    }

    if (a >= b)
    {
      break;
    }
  }

  thread->head -= move_count;

  if (!entry)
  {
    entry = uo_ttable_set(&thread->search->ttable, thread->position.key);
  }

  entry->score = color * value;
  entry->depth = depth;
  entry->bestmove = bestmove;

  if (value > a_orig)
  {
    entry->type = uo_tentry_type__upper_bound;
  }
  else if (value >= b)
  {
    entry->type = uo_tentry_type__lower_bound;
  }
  else
  {
    entry->type = uo_tentry_type__exact;
  }

  return value;
}

size_t uo_search_perft(uo_search_thread *thread, size_t depth)
{
  size_t move_count = uo_position_get_moves(&thread->position, thread->head);
  //uint64_t key = thread->position.key;

  if (depth == 1)
  {
    return move_count;
  }

  thread->head += move_count;

  size_t node_count = 0;

  //char fen_before_make[90];
  //char fen_after_unmake[90];

  //uo_position_print_fen(&thread->position, fen_before_make);

  for (int64_t i = 0; i < move_count; ++i)
  {
    uo_move move = thread->head[i - move_count];
    uo_position_make_move(&thread->position, move);
    //uo_position_print_move(&thread->position, move, buf);
    //printf("move: %s\n", buf);
    //uo_position_print_fen(&thread->position, buf);
    //printf("%s\n", buf);
    //uo_position_print_diagram(&thread->position, buf);
    //printf("%s\n", buf);
    node_count += uo_search_perft(thread, depth - 1);
    uo_position_unmake_move(&thread->position);
    //assert(thread->position.key == key);

  //  uo_position_print_fen(&thread->position, fen_after_unmake);

  //  if (strcmp(fen_before_make, fen_after_unmake) != 0 /* || key != thread->position.key */)
  //  {
  //    uo_position position;
  //    uo_position_from_fen(&position, fen_before_make);

  //    uo_position_print_move(&thread->position, move, buf);
  //    printf("error when unmaking move: %s\n", buf);
  //    printf("\nbefore make move\n");
  //    uo_position_print_diagram(&position, buf);
  //    printf("\n%s", buf);
  //    printf("\n");
  //    printf("Fen: %s\n", fen_before_make);
  //    printf("Key: %" PRIu64 "\n", key);
  //    printf("\nafter unmake move\n");
  //    uo_position_print_diagram(&thread->position, buf);
  //    printf("\n%s", buf);
  //    printf("\n");
  //    printf("Fen: %s\n", fen_after_unmake);
  //    printf("Key: %" PRIu64 "\n", thread->position.key);
  //    printf("\n");
  //  }
  }

  thread->head -= move_count;

  return node_count;
}

int uo_search_start(uo_search *search, const uo_search_params params)
{
  int16_t color = uo_position_flags_color_to_move(search->main_thread->position.flags) ? -1 : 1;

  uo_search_info info = {
    .depth = 1,
    .search = search,
    .multipv = params.multipv
  };

  uo_search_negamax(search->main_thread, 1, -UO_SCORE_CHECKMATE, UO_SCORE_CHECKMATE, color);

  search->pv = uo_ttable_get(&search->ttable, search->main_thread->position.key);
  search->pv->keep = true;

  for (size_t depth = 2; depth <= params.depth; ++depth)
  {
    params.report(info);
    uo_search_negamax(search->main_thread, depth, -UO_SCORE_CHECKMATE, UO_SCORE_CHECKMATE, color);
    info.depth = depth;
  }

  info.completed = true;
  params.report(info);
  search->pv->keep = false;

  return ++search_id;
}

void uo_search_end(int search_id)
{
  // TODO
}
