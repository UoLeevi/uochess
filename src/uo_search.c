#include "uo_search.h"
#include "uo_position.h"
#include "uo_global.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

int search_id;

char buf[0x1000];

static double uo_search_negamax(uo_search *search, size_t depth, double color, uo_move_ex *moves)
{
  if (depth == 0 /* or node is a terminal node */)
  {
    return color * uo_position_evaluate(&search->position);
  }

  double value = -INFINITY;

  size_t nodes_count = uo_position_get_moves(&search->position, search->head);
  uo_position_flags flags = search->position.flags;
  search->head += nodes_count;

  for (int64_t i = 0; i < nodes_count; ++i)
  {
    uo_move_ex move = search->head[i - nodes_count];
    uo_position_unmake_move *unmake_move = uo_position_make_move(&search->position, move);

    //uo_position_print_diagram(&search->position, buf);
    //printf("\n%s", buf);
    //uo_position_print_fen(&search->position, buf);
    //printf("\nFen: %s\n", buf);

    double node_value = -uo_search_negamax(search, depth - 1, -color, moves + 1);

    if (node_value > value)
    {
      value = node_value;
      *moves = move;
    }

    unmake_move(&search->position, move, flags);
  }

  search->head -= nodes_count;
  return value;
}

size_t uo_search_perft(uo_search *search, size_t depth)
{
  size_t move_count = uo_position_get_moves(&search->position, search->head);

  if (depth == 1)
  {
    return move_count;
  }

  uo_position_flags flags = search->position.flags;
  search->head += move_count;

  size_t node_count = 0;

  //char fen_before_make[90];
  //char fen_after_unmake[90];

  //uo_position_print_fen(&search->position, fen_before_make);

  for (int64_t i = 0; i < move_count; ++i)
  {
    uo_move_ex move = search->head[i - move_count];
    uo_position_unmake_move *unmake_move = uo_position_make_move(&search->position, move);
    //uo_move_print(move.move, buf);
    //printf("move: %s\n", buf);
    //uo_position_print_fen(&search->position, buf);
    //printf("%s\n", buf);
    //uo_position_print_diagram(&search->position, buf);
    //printf("%s\n", buf);
    node_count += uo_search_perft(search, depth - 1);
    unmake_move(&search->position, move, flags);

    //uo_position_print_fen(&search->position, fen_after_unmake);

    //if (strcmp(fen_before_make, fen_after_unmake) != 0)
    //{
    //  uo_position position;
    //  uo_position_from_fen(&position, fen_before_make);

    //  uo_move_print(move.move, buf);
    //  printf("error when unmaking move: %s\n", buf);
    //  printf("\nbefore make move\n");
    //  uo_position_print_diagram(&position, buf);
    //  printf("\n%s", buf);
    //  printf("\nFen: %s\n", fen_before_make);
    //  printf("\nafter unmake move\n");
    //  uo_position_print_diagram(&search->position, buf);
    //  printf("\n%s", buf);
    //  printf("\nFen: %s\n", fen_after_unmake);
    //  printf("\n");
    //}
  }

  search->head -= move_count;

  return node_count;
}

int uo_search_start(uo_search *search, const uo_search_params params)
{
  double color = uo_position_flags_color_to_move(search->position.flags) ? -1.0 : 1.0;

  uo_search_info info = {
    .pv = search->pv,
    .multipv = params.multipv
  };

  for (int i = 0; i < params.multipv; ++i)
  {
    search->pv[i].moves = malloc(params.depth * sizeof * search->pv[i].moves);
  }

  for (size_t depth = 1; depth < params.depth; ++depth)
  {
    double cp = uo_search_negamax(search, depth, color, info.pv->moves) * color;
    info.pv[0].score.cp = cp;
    info.depth = depth;
    params.report(info);
  }

  double cp = uo_search_negamax(search, params.depth, color, info.pv->moves) * color;
  info.pv[0].score.cp = cp;
  info.depth = params.depth;
  info.completed = true;

  params.report(info);

  for (int i = 0; i < params.multipv; ++i)
  {
    free(search->pv[i].moves);
  }

  return ++search_id;
}

void uo_search_end(int search_id)
{
  // TODO
}
