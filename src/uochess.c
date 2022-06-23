#include "uo_global.h"
#include "uo_bitboard.h"
#include "uo_position.h"
#include "uo_util.h"
#include "uo_move.h"
#include "uo_square.h"
#include "uo_search.h"
#include "uo_test.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>

// global buffer
char buf[0x1000];

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
  search.head = search.moves;
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
        if (strlen(ptr) < 4) return;

        uo_move_ex move = uo_position_parse_move(&search.position, ptr);
        if (!move.piece) return;

        uo_square square_from = uo_move_square_from(move.move);
        uo_square square_to = uo_move_square_to(move.move);

        int move_count = uo_position_get_moves(&search.position, search.head);
        search.head += move_count;

        for (int i = 0; i < move_count; ++i)
        {
          uo_move_ex move = search.head[i - move_count];
          if (square_from == uo_move_square_from(move.move)
            && square_to == uo_move_square_to(move.move))
          {
            uo_position_make_move(&search.position, move);
            *search.head++ = move;
            goto next_move;
          }
        }

        search.head -= move_count;

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

    uo_move_ex *moves = info.pv[i].moves;

    while (moves->piece)
    {
      uo_move_ex move = *moves++;
      uo_move_print(move.move, buf);
      printf(" %s", buf);
    }

    printf("\n");

    if (info.completed && info.multipv == 1)
    {
      uo_move_ex bestmove = *info.pv[i].moves;
      uo_move_print(bestmove.move, buf);
      printf("bestmove %s\n", buf);
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
      if (sscanf(ptr, "%d", &depth) == 1 && depth > 0)
      {
        clock_t clock_start = clock();
        size_t total_node_count = 0;
        size_t move_count = uo_position_get_moves(&search.position, search.head);
        uo_position_flags flags = search.position.flags;
        search.head += move_count;

        for (int64_t i = 0; i < move_count; ++i)
        {
          uo_move_ex move = search.head[i - move_count];
          uo_move_print(move.move, buf);
          uo_position_unmake_move *unmake_move = uo_position_make_move(&search.position, move);
          size_t node_count = depth == 1 ? 1 : uo_search_perft(&search, depth - 1);
          unmake_move(&search.position, move, flags);
          total_node_count += node_count;
          printf("%s: %zu\n", buf, node_count);
        }

        search.head -= move_count;

        clock_t clock_end = clock();
        int duration_ms = (clock_end - clock_start) * 1000 / CLOCKS_PER_SEC;

        printf("\nNodes searched: %zu, time: %d.%03d s (%zu kN/s)\n\n", total_node_count, duration_ms / 1000, duration_ms % 1000, total_node_count / duration_ms);
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

static int run_tests(char *test_data_dir)
{
  bool passed = true;

  passed &= uo_test_move_generation(&search, test_data_dir);

  return passed ? 0 : 1;
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
  if (argc > 1 && strcmp(argv[1], "--test") == 0)
  {
    char *test_data_dir = NULL;

    for (int i = 2; i < argc - 1; ++i)
    {
      if (strcmp(argv[i], "--datadir") == 0)
      {
        test_data_dir = argv[i + 1];
        struct stat info;

        if (stat(test_data_dir, &info) != 0)
        {
          printf("cannot access '%s'\n", test_data_dir);
          return 1;
        }
        else if (info.st_mode & S_IFDIR)
        {
          // directory exists
          break;
        }
        else
        {
          printf("'%s' is no directory\n", test_data_dir);
          return 1;
        }
      }
    }

    engine_init();
    return run_tests(test_data_dir);
  }

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
