#include "uo_bitboard.h"
#include "uo_position.h"
#include "uo_util.h"
#include "uo_move.h"
#include "uo_square.h"
#include "uo_search.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

/*
uci
isready
position fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
*/

char buf[1028];
char *ptr;

struct
{
  uint16_t multipv;
} options;

uo_search search;

enum state
{
  INIT,
  OPTIONS,
  READY,
  POSITION
} state = INIT;

static void engine_init(void)
{
  uo_bitboard_init();
  search.head = &search.moves;
  options.multipv = 1;
}

static void process_cmd__init(void)
{
  if (ptr && strcmp(ptr, "uci") == 0)
  {
    printf("id name uochess\n");
    printf("id author Leevi Uotinen\n\n");
    printf("option name MultiPV type spin default 1 min 1 max 500\n");
    printf("uciok\n");
    state = OPTIONS;
  }
}

static void process_cmd__options(void)
{
  char *ptr = buf;

  if (ptr && strcmp(ptr, "setoption name") == 0)
  {
    ptr = strtok(NULL, " ");
    ptr = strtok(NULL, " ");

    int spin;

    if (ptr && sscanf(ptr, "MultiPV value %d", &spin) == 1 && spin >= 1 && spin <= 500)
    {
      options.multipv = spin;
    }
  }
  else if (ptr && strcmp(ptr, "isready") == 0)
  {
    printf("readyok\n");

    search.pv = malloc(options.multipv * sizeof * search.pv);

    state = READY;
  }
}

static void process_cmd__ready(void)
{
  char *ptr = buf;

  if (ptr && strcmp(ptr, "position") == 0)
  {
    ptr = strtok(NULL, "\n ");

    if (ptr && strcmp(ptr, "startpos") == 0)
    {
      uo_position *ret = uo_position_from_fen(&search.position, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
      state = POSITION;
      //printf("position: %p\n", ret);
      //uo_position_to_diagram(&search.position, buf);
      //printf("diagram:\n%s", buf);

      ptr = strtok(NULL, "\n ");
    }
    else if (ptr && strcmp(ptr, "fen") == 0)
    {
      ptr = strtok(NULL, "\n");

      char *fen = strtok(ptr, "m"); /* moves */
      char *ptr = strtok(NULL, "\n ");

      if (ptr)
      {
        --ptr;
        *ptr = 'm';
      }

      uo_position *ret = uo_position_from_fen(&search.position, fen);
      state = POSITION;
      //printf("position: %p\n", ret);
      //uo_position_to_diagram(&search.position, buf);
      //printf("diagram:\n%s", buf);
    }
    else
    {
      // unexpected command
      return;
    }

    if (ptr && strcmp(ptr, "moves") == 0)
    {
      while (ptr = strtok(NULL, " \n"))
      {
        if (strlen(ptr) != 4) return;

        int file_from = ptr[0] - 'a';
        if (file_from < 0 || file_from > 7) return;
        int rank_from = ptr[1] - '1';
        if (rank_from < 0 || rank_from > 7) return;
        uo_square square_from = (rank_from << 3) + file_from;

        int file_to = ptr[2] - 'a';
        if (file_to < 0 || file_to > 7) return;
        int rank_to = ptr[3] - '1';
        if (rank_to < 0 || rank_to > 7) return;
        uo_square square_to = (rank_to << 3) + file_to;

        int nodes_count = uo_position_get_moves(&search.position, search.head);
        search.head += nodes_count;

        for (int i = 0; i < nodes_count; ++i)
        {
          uo_move move = search.head[i];
          if (square_from == uo_move_square_from(move)
            && square_to == uo_move_square_to(move))
          {
            *search.head++ = uo_position_make_move(&search.position, move);
            goto next_move;
          }
        }

        search.head -= nodes_count;

        // Not a legal move
        return;

      next_move:;
      }
    }
  }
}

static void report_search_info(uo_search_info info)
{
  for (int i = 0; i < info.multipv; ++i)
  {
    printf("info depth %d seldepth %d multipv %d score cp %d nodes %" PRIu64 " nps %" PRIu64 " tbhits %d time %" PRIu64 " pv",
      info.depth, info.seldepth, info.multipv, info.pv[i].score.cp, info.nodes, info.nps, info.tbhits, info.time);

    uo_move *moves = info.pv[i].moves;

    while (*moves)
    {
      uo_move move = *moves++;
      uo_square square_from = uo_move_square_from(move);
      uo_square square_to = uo_move_square_to(move);
      printf(" %c%d%c%d",
        'a' + uo_square_file(square_from), 1 + uo_square_rank(square_to),
        'a' + uo_square_file(square_from), 1 + uo_square_rank(square_to));
    }

    printf("\n");

    if (info.completed && info.multipv == 1)
    {
      uo_move bestmove = *info.pv[i].moves;
      uo_square square_from = uo_move_square_from(bestmove);
      uo_square square_to = uo_move_square_to(bestmove);
      printf("bestmove %c%d%c%d\n",
        'a' + uo_square_file(square_from), 1 + uo_square_rank(square_to),
        'a' + uo_square_file(square_from), 1 + uo_square_rank(square_to));
    }
  }
}

static void process_cmd__position(void)
{
  char *ptr = buf;

  if (ptr && strcmp(ptr, "ucinewgame") == 0)
  {
    state = READY;
    return;
  }

  if (ptr && strcmp(ptr, "d") == 0)
  {
    uo_position_print_diagram(&search.position, buf);
    printf("\n%s", buf);
    uo_position_print_fen(&search.position, buf);
    printf("\nFen: %s\n", buf);
    return;
  }

  else if (ptr && strcmp(ptr, "go") == 0)
  {
    ptr = strtok(NULL, "\n ");

    if (ptr && strcmp(ptr, "perft") == 0)
    {
      ptr = strtok(NULL, "\n ");

      int depth;
      if (sscanf(ptr, "%d", &depth) == 1)
      {
        int nodes_count = uo_position_get_moves(&search.position, search.head);
        search.head += nodes_count;

        for (int i = 0; i < nodes_count; ++i)
        {
          uo_move move = search.head[i];
          uo_square square_from = uo_move_square_from(move);
          uo_square square_to = uo_move_square_to(move);

          printf("%c%d%c%d: 1\n",
            'a' + uo_square_file(square_from), 1 + uo_square_rank(square_from),
            'a' + uo_square_file(square_to), 1 + uo_square_rank(square_to));
        }

        search.head -= nodes_count;

        printf("\nNodes searched: %d\n\n", nodes_count);
      }
    }

    else if (ptr && strcmp(ptr, "depth") == 0)
    {
      ptr = strtok(NULL, "\n ");

      int depth;
      if (sscanf(ptr, "%d", &depth) == 1 && depth > 0)
      {
        uo_search_params search_params = {
          .depth = depth,
          .multipv = options.multipv,
          .report = report_search_info
        };

        uo_search_start(&search, search_params);
      }
    }
  }
}

static void (*process_cmd[])() = {
  process_cmd__init,
  process_cmd__options,
  process_cmd__ready,
  process_cmd__position,
};

int main(
  int argc,
  char **argv)
{
  printf("Uochess 0.1 by Leevi Uotinen\n");
  engine_init();

  while (fgets(buf, sizeof buf, stdin))
  {
    ptr = strtok(buf, "\n ");

    if (strcmp(ptr, "quit") == 0)
    {
      return 0;
    }

    process_cmd[state]();
  }

  return 0;
}
