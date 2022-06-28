#include "uo_search.h"
#include "uo_position.h"
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

char buf[0x1000];

const int16_t checkmate = 0x7FFF;
const int16_t mate_in_threshold = 0x7FFF - UO_MAX_PLY;

int16_t uo_search_static_evaluate(uo_search *search);

void uo_search_sort_moves(uo_search *search)
{
  uo_move move;
}

static int uo_move_comp(const uo_move *lhs, const uo_move *rhs)
{
  uo_move move_lhs = *lhs;
  uo_square square_from_lhs = uo_move_square_from(move_lhs);
  uo_square square_to_lhs = uo_move_square_to(move_lhs);
  uo_square move_type_lhs = uo_move_get_type(move_lhs);
  int score_lhs = 0;

  if (move_type_lhs & uo_move_type__x)
  {
    uo_piece piece_lhs = search.position.board[square_from_lhs];
    uo_piece piece_captured_lhs = search.position.board[square_to_lhs];
    score_lhs = uo_piece_type(piece_captured_lhs) - uo_piece_type(piece_lhs) + uo_piece__B;
  }

  uo_move move_rhs = *rhs;
  uo_square square_from_rhs = uo_move_square_from(move_rhs);
  uo_square square_to_rhs = uo_move_square_to(move_rhs);
  uo_square move_type_rhs = uo_move_get_type(move_rhs);
  int score_rhs = 0;

  if (move_type_rhs & uo_move_type__x)
  {
    uo_piece piece_rhs = search.position.board[square_from_rhs];
    uo_piece piece_captured_rhs = search.position.board[square_to_rhs];
    score_rhs = uo_piece_type(piece_captured_rhs) - uo_piece_type(piece_rhs) + uo_piece__B;
  }

  return score_lhs == score_rhs ? 0 : score_lhs > score_rhs ? -1 : 1;
}

// see: https://en.wikipedia.org/wiki/Negamax#Negamax_with_alpha_beta_pruning_and_transposition_tables
static int16_t uo_search_negamax(uo_search *search, size_t depth, int16_t a, int16_t b, int16_t color)
{
  int16_t a_orig = a;

  uo_tentry *entry = uo_ttable_get(&search->ttable, search->position.key);
  if (entry->type && entry->depth >= depth)
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

  int64_t move_count = uo_position_get_moves(&search->position, search->head);

  if (move_count == 0)
  {
    if (!entry->type)
    {
      uo_tentry_release(entry);
    }

    return uo_position_is_check(&search->position) ? -checkmate : 0;
  }

  if (depth == 0)
  {
    if (!entry->type)
    {
      uo_tentry_release(entry);
    }

    return color * uo_search_static_evaluate(search);
  }

  qsort(search->head, move_count, sizeof * search->head, uo_move_comp);
  search->head += move_count;
  entry->keep = true;

  uo_move bestmove;
  int16_t value = -checkmate;

  for (int64_t i = 0; i < move_count; ++i)
  {
    uo_move move = search->head[i - move_count];
    uo_position_make_move(&search->position, move);
    int16_t node_value = -uo_search_negamax(search, depth - 1, -b, -a, -color);
    uo_position_unmake_move(&search->position);

    if (node_value > mate_in_threshold)
    {
      --node_value;
    }
    else if (node_value < -mate_in_threshold)
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

  search->head -= move_count;
  entry->keep = false;
  entry->score = color * value;
  entry->depth = depth;

  if (value > a_orig)
  {
    entry->type = uo_tentry_type__upper_bound;
    entry->bestmove = bestmove;
  }
  else if (value >= b)
  {
    entry->type = uo_tentry_type__lower_bound;
  }
  else
  {
    entry->type = uo_tentry_type__exact;
    entry->bestmove = bestmove;
  }

  return value;
}

size_t uo_search_perft(uo_search *search, size_t depth)
{
  size_t move_count = uo_position_get_moves(&search->position, search->head);
  //uint64_t key = search->position.key;

  if (depth == 1)
  {
    return move_count;
  }

  search->head += move_count;

  size_t node_count = 0;

  //char fen_before_make[90];
  //char fen_after_unmake[90];

  //uo_position_print_fen(&search->position, fen_before_make);

  for (int64_t i = 0; i < move_count; ++i)
  {
    uo_move move = search->head[i - move_count];
    uo_position_make_move(&search->position, move);
    //uo_move_print(move, buf);
    //printf("move: %s\n", buf);
    //uo_position_print_fen(&search->position, buf);
    //printf("%s\n", buf);
    //uo_position_print_diagram(&search->position, buf);
    //printf("%s\n", buf);
    node_count += uo_search_perft(search, depth - 1);
    uo_position_unmake_move(&search->position);
    //assert(search->position.key == key);

    //uo_position_print_fen(&search->position, fen_after_unmake);

    //if (/*strcmp(fen_before_make, fen_after_unmake) != 0 || */key != search->position.key)
    //{
    //  //uo_position position;
    //  //uo_position_from_fen(&position, fen_before_make);

    //  //uo_move_print(move, buf);
    //  //printf("error when unmaking move: %s\n", buf);
    //  //printf("\nbefore make move\n");
    //  //uo_position_print_diagram(&position, buf);
    //  //printf("\n%s", buf);
    //  //printf("\n");
    //  //printf("Fen: %s\n", fen_before_make);
    //  //printf("Key: %" PRIu64 "\n", key);
    //  //printf("\nafter unmake move\n");
    //  //uo_position_print_diagram(&search->position, buf);
    //  //printf("\n%s", buf);
    //  //printf("\n");
    //  //printf("Fen: %s\n", fen_after_unmake);
    //  //printf("Key: %" PRIu64 "\n", search->position.key);
    //  //printf("\n");
    //}
  }

  search->head -= move_count;

  return node_count;
}

int uo_search_start(uo_search *search, const uo_search_params params)
{
  int16_t color = uo_position_flags_color_to_move(search->position.flags) ? -1 : 1;

  uo_search_info info = {
    .search = search,
    .multipv = params.multipv
  };

  uo_tentry *pv = uo_ttable_get(&search->ttable, search->position.key);
  pv->keep = true;

  for (size_t depth = 1; depth < params.depth; ++depth)
  {
    int16_t cp = uo_search_negamax(search, depth, -checkmate, checkmate, color) * color;
    *search->pv = *pv;
    info.depth = depth;
    params.report(info);
  }

  int16_t cp = uo_search_negamax(search, params.depth, -checkmate, checkmate, color) * color;
  *search->pv = *pv;
  info.depth = params.depth;
  info.completed = true;

  params.report(info);
  pv->keep = false;

  return ++search_id;
}

void uo_search_end(int search_id)
{
  // TODO
}


#pragma region eval_features

static double uo_eval_feature_material_P(uo_search *search, uint8_t color)
{
  return uo_popcnt(search->position.P & (color ? search->position.piece_color : ~search->position.piece_color));
}
static double uo_eval_feature_material_N(uo_search *search, uint8_t color)
{
  return uo_popcnt(search->position.N & (color ? search->position.piece_color : ~search->position.piece_color));
}
static double uo_eval_feature_material_B(uo_search *search, uint8_t color)
{
  return uo_popcnt(search->position.B & (color ? search->position.piece_color : ~search->position.piece_color));
}
static double uo_eval_feature_material_R(uo_search *search, uint8_t color)
{
  return uo_popcnt(search->position.R & (color ? search->position.piece_color : ~search->position.piece_color));
}
static double uo_eval_feature_material_Q(uo_search *search, uint8_t color)
{
  return uo_popcnt(search->position.Q & (color ? search->position.piece_color : ~search->position.piece_color));
}

#pragma endregion

static const double (*eval_features[])(uo_search *search, uint8_t color) = {
    uo_eval_feature_material_P,
    uo_eval_feature_material_N,
    uo_eval_feature_material_B,
    uo_eval_feature_material_R,
    uo_eval_feature_material_Q };

#define UO_EVAL_FEATURE_COUNT (sizeof eval_features / sizeof *eval_features)

double weights[UO_EVAL_FEATURE_COUNT << 1] =
{
    1.0,
    -1.0,
    3.0,
    -3.0,
    3.0,
    -3.0,
    5.0,
    -5.0,
    9.0,
    -9.0 };

int16_t uo_search_static_evaluate(uo_search *search)
{
  double inputs[UO_EVAL_FEATURE_COUNT << 1] = { 0 };
  double evaluation = 0;

  for (int i = 0; i < UO_EVAL_FEATURE_COUNT; ++i)
  {
    inputs[i << 1] = eval_features[i](search, uo_piece__white);
    inputs[(i << 1) + 1] = eval_features[i](search, uo_piece__black);
    evaluation += inputs[i << 1] * weights[i << 1];
    evaluation += inputs[(i << 1) + 1] * weights[(i << 1) + 1];
  }

  return evaluation * 100;
}
