#include "uo_global.h"
#include "uo_bitboard.h"
#include "uo_position.h"
#include "uo_util.h"
#include "uo_move.h"
#include "uo_square.h"
#include "uo_zobrist.h"
#include "uo_engine.h"
#include "uo_engine.h"
#include "uo_thread.h"
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

uo_engine_init_options options;
uo_engine engine;

enum state
{
  INIT,
  OPTIONS,
  READY,
  POSITION,
  GO
} state = INIT;

static void uochess_init(void)
{
  uo_zobrist_init();
  uo_bitboard_init();

  // default options

  options.multipv = 1;
  options.threads = 1;

  size_t capacity = (size_t)16000000 / sizeof * engine.ttable.entries;
  capacity = (size_t)1 << uo_msb(capacity);
  options.hash_size = capacity * sizeof * engine.ttable.entries;

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

      // Threads
      if (ptr && sscanf(ptr, "Threads value %" PRIi64, &spin) == 1 && spin >= 1 && spin <= 512)
      {
        options.threads = spin;
      }

      // Hash
      if (ptr && sscanf(ptr, "Hash value %" PRIi64, &spin) == 1 && spin >= 1 && spin <= 33554432)
      {
        size_t capacity = spin * (size_t)1000000 / sizeof * engine.ttable.entries;
        options.hash_size = ((size_t)1 << uo_msb(capacity)) * sizeof * engine.ttable.entries;
      }

      // MultiPV
      if (ptr && sscanf(ptr, "MultiPV value %" PRIi64, &spin) == 1 && spin >= 1 && spin <= 500)
      {
        options.multipv = spin;
      }
    }
  }
  else if (ptr && strcmp(ptr, "isready") == 0)
  {
    uo_engine_init(&engine, &options);
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

    uo_engine_lock_position(&engine);

    if (ptr && strcmp(ptr, "startpos") == 0)
    {
      uo_position *ret = uo_position_from_fen(&engine.position, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
      state = POSITION;
      //printf("position: %p\n", ret);
      //uo_position_to_diagram(&engine.position, buf);
      //printf("diagram:\n%s", buf);

      ptr = strtok(NULL, "\n ");
    }
    else if (ptr && strcmp(ptr, "fen") == 0)
    {
      ptr = strtok(NULL, "\n");

      char *fen = strtok(ptr, "m"); /* moves */
      ptr = strtok(NULL, "\n ");

      if (ptr && strcmp(ptr, "oves") == 0)
      {
        --ptr;
        *ptr = 'm';
      }

      uo_position *ret = uo_position_from_fen(&engine.position, fen);
      state = POSITION;
      //printf("position: %p\n", ret);
      //uo_position_to_diagram(&engine.position, buf);
      //printf("diagram:\n%s", buf);
    }
    else
    {
      uo_engine_unlock_position(&engine);
      // unexpected command
      return;
    }

    if (ptr && strcmp(ptr, "moves") == 0)
    {
      while (ptr = strtok(NULL, " \n"))
      {
        if (strlen(ptr) < 4) return;

        uo_move move = uo_position_parse_move(&engine.position, ptr);
        if (!move) return;

        uo_square square_from = uo_move_square_from(move);
        uo_square square_to = uo_move_square_to(move);

        int move_count = uo_position_generate_moves(&engine.position);
        engine.position.movelist.head += move_count;

        for (int i = 0; i < move_count; ++i)
        {
          uo_move move = engine.position.movelist.head[i - move_count];
          if (square_from == uo_move_square_from(move)
            && square_to == uo_move_square_to(move))
          {
            uo_position_make_move(&engine.position, move);
            *engine.position.movelist.head++ = move;
            goto next_move;
          }
        }

        engine.position.movelist.head -= move_count;

        // Not a legal move
        return;

      next_move:;
      }
    }

    uo_engine_unlock_position(&engine);
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
    uo_engine_lock_position(&engine);
    uo_engine_lock_stdout(&engine);
    uo_position_print_diagram(&engine.position, buf);
    printf("\n%s", buf);
    uo_position_print_fen(&engine.position, buf);
    printf("\n");
    printf("Fen: %s\n", buf);
    printf("Key: %" PRIu64 "\n", engine.position.key);
    uo_engine_unlock_stdout(&engine);
    uo_engine_unlock_position(&engine);
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
        uo_engine_lock_stdout(&engine);
        uo_engine_lock_position(&engine);

        struct timespec ts_start;
        timespec_get(&ts_start, TIME_UTC);

        size_t total_node_count = 0;
        size_t move_count = uo_position_generate_moves(&engine.position);
        engine.position.movelist.head += move_count;

        for (int64_t i = 0; i < move_count; ++i)
        {
          uo_move move = engine.position.movelist.head[i - move_count];
          uo_position_print_move(&engine.position, move, buf);
          uo_position_make_move(&engine.position, move);
          size_t node_count = depth == 1 ? 1 : uo_position_perft(&engine.position, depth - 1);
          uo_position_unmake_move(&engine.position);
          total_node_count += node_count;
          printf("%s: %zu\n", buf, node_count);
        }

        engine.position.movelist.head -= move_count;

        struct timespec ts_end;
        timespec_get(&ts_end, TIME_UTC);

        double time_ms = difftime(ts_end.tv_sec, ts_start.tv_sec) * 1000.0;
        time_ms += (ts_end.tv_nsec - ts_start.tv_nsec) / 1000000.0;

        uint64_t knps = total_node_count / time_ms;

        printf("\nNodes engineed: %zu, time: %.03f s (%zu kN/s)\n\n", total_node_count, time_ms / 1000.0, knps);
        uo_engine_unlock_position(&engine);
        uo_engine_unlock_stdout(&engine);
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
          .multipv = options.multipv
        };

        uo_engine_start_search(&engine, &search_params);
        state = GO;
        return;
      }
    }
  }
}

static void process_cmd__go(void)
{
  char *ptr = buf;

  if (ptr && strcmp(ptr, "stop") == 0)
  {
    uo_engine_stop_search(&engine);
    state = POSITION;
    return;
  }

  if (uo_atomic_compare_exchange(&engine.stop, 1, 1))
  {
    state = POSITION;
    process_cmd__position();
    return;
  }

  if (ptr && strcmp(ptr, "d") == 0)
  {
    uo_engine_lock_position(&engine);
    uo_engine_lock_stdout(&engine);
    uo_position_print_diagram(&engine.position, buf);
    printf("\n%s", buf);
    uo_position_print_fen(&engine.position, buf);
    printf("\n");
    printf("Fen: %s\n", buf);
    printf("Key: %" PRIu64 "\n", engine.position.key);
    uo_engine_unlock_stdout(&engine);
    uo_engine_unlock_position(&engine);
    return;
  }
}

static int run_tests(char *test_data_dir)
{
  bool passed = true;

  uo_engine_lock_position(&engine);
  uo_engine_lock_stdout(&engine);
  passed &= uo_test_move_generation(&engine.position, test_data_dir);
  uo_engine_unlock_position(&engine);
  uo_engine_unlock_stdout(&engine);

  return passed ? 0 : 1;
}

static void (*process_cmd[])() = {
  process_cmd__init,
  process_cmd__options,
  process_cmd__ready,
  process_cmd__position,
  process_cmd__go,
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

    uochess_init();
    uo_engine_init(&engine, &options);
    return run_tests(test_data_dir);
  }

  printf("Uochess 0.1 by Leevi Uotinen\n");
  uochess_init();

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
