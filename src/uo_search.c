#include "uo_search.h"
#include "uo_position.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>

int search_id;

char buf[0x1000];

static double uo_search_negamax(uo_search *search, size_t depth, double color, uo_move *moves)
{
  if (depth == 0 /* or node is a terminal node */)
  {
    return color * uo_position_evaluate(&search->position);
  }

  double value = -INFINITY;

  int nodes_count = uo_position_get_moves(&search->position, search->head);
  search->head += nodes_count;

  for (int i = 0; i < nodes_count; ++i)
  {
    uo_move move = search->head[i - nodes_count];
    uo_move_ex move_ex = uo_position_make_move(&search->position, move);

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

    uo_position_unmake_move(&search->position, move_ex);
  }

  search->head -= nodes_count;
  return value;
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
