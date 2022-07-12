#include "uo_global.h"
#include "uo_bitboard.h"
#include "uo_position.h"
#include "uo_util.h"
#include "uo_move.h"
#include "uo_square.h"
#include "uo_zobrist.h"
#include "uo_search.h"
#include "uo_test.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>
#include <time.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>

// global buffer
char buf[0x1000];

char *ptr;

uo_search_init_options options;
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
  uo_zobrist_init();
  uo_bitboard_init();
  options.multipv = 1;
  options.threads = 1;

  size_t capacity = (size_t)16000000 / sizeof * search.ttable.entries;
  capacity = (size_t)1 << uo_msb(capacity);
  options.hash_size = capacity * sizeof * search.ttable.entries;
}

static void process_cmd__init(void)
{
  if (ptr && strcmp(ptr, "uci") == 0)
  {
    printf("id name uochess\n");
    printf("id author Leevi Uotinen\n\n");
    printf("option name Threads type spin default 1 min 1 max 512\n");
    printf("option name Hash type spin default 16 min 1 max 33554432\n");
    printf("option name MultiPV type spin default 1 min 1 max 500\n");

    state = OPTIONS;
    printf("uciok\n");
  }
}

static void process_cmd__options(void)
{
  char *ptr = buf;

  if (ptr && strcmp(ptr, "setoption") == 0)
  {
    ptr = strtok(NULL, " ");

    if (ptr && strcmp(ptr, "name") == 0)
    {
      ptr = strtok(NULL, "\n");

      int64_t spin;

      if (ptr && sscanf(ptr, "Threads value %" PRIi64, &spin) == 1 && spin >= 1 && spin <= 512)
      {
        options.threads = spin;
      }

      if (ptr && sscanf(ptr, "Hash value %" PRIi64, &spin) == 1 && spin >= 1 && spin <= 33554432)
      {
        size_t capacity = spin * (size_t)1000000 / sizeof * search.ttable.entries;
        options.hash_size = ((size_t)1 << uo_msb(capacity)) * sizeof * search.ttable.entries;
      }

      if (ptr && sscanf(ptr, "MultiPV value %" PRIi64, &spin) == 1 && spin >= 1 && spin <= 500)
      {
        options.multipv = spin;
      }
    }
  }
  else if (ptr && strcmp(ptr, "isready") == 0)
  {
    uo_search_init(&search, &options);
    state = READY;
    printf("readyok\n");
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
      uo_position *ret = uo_position_from_fen(&search.main_thread->position, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
      state = POSITION;
      //printf("position: %p\n", ret);
      //uo_position_to_diagram(&search.main_thread->position, buf);
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

      uo_position *ret = uo_position_from_fen(&search.main_thread->position, fen);
      state = POSITION;
      //printf("position: %p\n", ret);
      //uo_position_to_diagram(&search.main_thread->position, buf);
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

        uo_move move = uo_position_parse_move(&search.main_thread->position, ptr);
        if (!move) return;

        uo_square square_from = uo_move_square_from(move);
        uo_square square_to = uo_move_square_to(move);

        int move_count = uo_position_get_moves(&search.main_thread->position, search.main_thread->head);
        search.main_thread->head += move_count;

        for (int i = 0; i < move_count; ++i)
        {
          uo_move move = search.main_thread->head[i - move_count];
          if (square_from == uo_move_square_from(move)
            && square_to == uo_move_square_to(move))
          {
            uo_position_make_move(&search.main_thread->position, move);
            *search.main_thread->head++ = move;
            goto next_move;
          }
        }

        search.main_thread->head -= move_count;

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
    uo_position_update_checks_and_pins(&info.search->main_thread->position);

    printf("info depth %d seldepth %d multipv %d ",
      info.depth, info.seldepth, info.multipv);

    int16_t score = info.search->pv[i].score;
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

    printf("nodes %" PRIu64 " nps %" PRIu64 " tbhits %d time %" PRIu64 " pv",
      info.nodes, info.nps, info.tbhits, info.time);

    size_t pv_len = 1;
    uo_tentry *pv_entry = info.search->pv;

    size_t i = 0;

    for (; i < info.depth && pv_entry; ++i)
    {
      uo_position_print_move(&info.search->main_thread->position, pv_entry->bestmove, buf);
      printf(" %s", buf);

      if (!uo_position_is_legal_move(&info.search->main_thread->position, info.search->main_thread->head, pv_entry->bestmove))
      {
        uo_position_print_move(&info.search->main_thread->position, pv_entry->bestmove, buf);
        printf("\n\nillegal bestmove %s\n", buf);
        uo_position_print_diagram(&info.search->main_thread->position, buf);
        printf("\n%s", buf);
        uo_position_print_fen(&info.search->main_thread->position, buf);
        printf("\n");
        printf("Fen: %s\n", buf);
        printf("Key: %" PRIu64 "\n", info.search->main_thread->position.key);
        assert(false);
      }

      uo_position_make_move(&info.search->main_thread->position, pv_entry->bestmove);
      pv_entry = uo_ttable_get(&info.search->ttable, info.search->main_thread->position.key);
    }

    while (i--)
    {
      uo_position_unmake_move(&info.search->main_thread->position);
    }

    printf("\n");

    if (info.completed && info.multipv == 1)
    {
      uo_position_print_move(&info.search->main_thread->position, info.search->pv->bestmove, buf);
      printf("bestmove %s\n", buf);
    }
  }

  uo_position_update_checks_and_pins(&info.search->main_thread->position);
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
    uo_position_print_diagram(&search.main_thread->position, buf);
    printf("\n%s", buf);
    uo_position_print_fen(&search.main_thread->position, buf);
    printf("\n");
    printf("Fen: %s\n", buf);
    printf("Key: %" PRIu64 "\n", search.main_thread->position.key);
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
        size_t move_count = uo_position_get_moves(&search.main_thread->position, search.main_thread->head);
        search.main_thread->head += move_count;

        for (int64_t i = 0; i < move_count; ++i)
        {
          uo_move move = search.main_thread->head[i - move_count];
          uo_position_print_move(&search.main_thread->position, move, buf);
          uo_position_make_move(&search.main_thread->position, move);
          size_t node_count = depth == 1 ? 1 : uo_search_perft(search.main_thread, depth - 1);
          uo_position_unmake_move(&search.main_thread->position);
          total_node_count += node_count;
          printf("%s: %zu\n", buf, node_count);
        }

        search.main_thread->head -= move_count;

        clock_t clock_end = clock();
        int duration_ms = (clock_end - clock_start) * 1000 / CLOCKS_PER_SEC;

        printf("\nNodes searched: %zu, time: %d.%03d s (%zu kN/s)\n\n", total_node_count, duration_ms / 1000, duration_ms % 1000, total_node_count / duration_ms);
      }
    }

    else if (ptr && strcmp(ptr, "depth") == 0)
    {
      ptr = strtok(NULL, "\n ");

      int depth;
      if (ptr && sscanf(ptr, "%d", &depth) == 1 && depth > 0)
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

  passed &= uo_test_move_generation(search.main_thread, test_data_dir);

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
    uo_search_init(&search, &options);
    return run_tests(test_data_dir);
  }

  printf("Uochess 0.1 by Leevi Uotinen\n");
  engine_init();

  while (fgets(buf, sizeof buf, stdin))
  {
    ptr = strtok(buf, "\n ");

    if (ptr && strcmp(ptr, "quit") == 0)
    {
      return 0;
    }

    process_cmd[state]();
  }

  return 0;
}
