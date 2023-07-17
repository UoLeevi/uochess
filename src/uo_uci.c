#include "uo_uci.h"
#include "uo_misc.h"
#include "uo_engine.h"
#include "uo_evaluation.h"
#include "uo_def.h"
#include "uo_global.h"
#include "uo_strmap.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>


#define uo_uci_state_init 0
#define uo_uci_state_config 1
#define uo_uci_state_idle 2
#define uo_uci_state_running 3
static int state;

static FILE *log_file;
static char *ptr;

typedef void (*uo_uci_command)(void);

static char *uo_uci_read_stdin(void)
{
  if (ptr == NULL)
  {
    if (fgets(buf, sizeof buf, stdin) == NULL)
    {
      exit(1);
    }

    if (log_file)
    {
      fwrite(buf, sizeof * buf, strlen(buf), log_file);
    }

    ptr = strtok(buf, "\n ");
  }
  else
  {
    ptr = strtok(NULL, "\n ");
  }

  return ptr;
}

static void uo_uci_command__quit(void)
{
  exit(0);
}

static void uo_uci_command__d(void)
{
  uo_engine_lock_position();
  uo_engine_lock_stdout();
  uo_position_print_diagram(&engine.position, buf);
  printf("\n%s", buf);
  uo_position_print_fen(&engine.position, buf);
  printf("\n");
  printf("Fen: %s\n", buf);
  printf("Key: %" PRIu64 "\n", engine.position.key);
  printf("Checkers:");

  uo_bitboard checks = uo_position_checks(&engine.position);
  while (checks)
  {
    uo_square square = uo_bitboard_next_square(&checks);
    if (uo_color(engine.position.flags) == uo_black)
    {
      square ^= 56;
    }

    printf(" %c%c", 'a' + uo_square_file(square), '1' + uo_square_rank(square));
  }

  printf("\n");

  uo_engine_unlock_stdout();
  uo_engine_unlock_position();
}

static void uo_uci_command__position_startpos(void)
{
  uo_position_from_fen(&engine.position, uo_fen_startpos);
  uo_uci_read_stdin();
}

static void uo_uci_command__position_fen(void)
{
  ptr = strtok(NULL, "\n");

  char *fen = strtok(ptr, "m"); /* moves */
  uo_uci_read_stdin();

  if (ptr && strcmp(ptr, "oves") == 0)
  {
    --ptr;
    *ptr = 'm';
  }

  uo_position_from_fen(&engine.position, fen);
}

static void uo_uci_command__position_randomize(void)
{
  ptr = strtok(NULL, " \n");
  uo_position_randomize(&engine.position, ptr);
}

static void uo_uci_command__position(void)
{
  uo_uci_read_stdin();

  static uo_strmap *uci_command_map__position = NULL;

  if (!uci_command_map__position)
  {
    uci_command_map__position = uo_strmap_create();
    uo_strmap_add(uci_command_map__position, "startpos", uo_uci_command__position_startpos);
    uo_strmap_add(uci_command_map__position, "fen", uo_uci_command__position_fen);
    uo_strmap_add(uci_command_map__position, "randomize", uo_uci_command__position_randomize);

  }

  uo_uci_command command = uo_strmap_get(uci_command_map__position, ptr);
  if (!command) return;

  uo_engine_lock_position();
  command();

  if (ptr && strcmp(ptr, "moves") == 0)
  {
    while ((ptr = strtok(NULL, " \n")))
    {
      if (strlen(ptr) < 4)
      {
        uo_engine_unlock_position();
        return;
      }

      uo_move move = uo_position_parse_move(&engine.position, ptr);
      if (!move)
      {
        uo_engine_unlock_position();
        return;
      }

      if (uo_position_is_max_depth_reached(&engine.position))
      {
        uo_position_reset_root(&engine.position);
      }

      uo_square square_from = uo_move_square_from(move);
      uo_square square_to = uo_move_square_to(move);
      uo_move_type move_type_promo = uo_move_get_type(move) & uo_move_type__promo_Q;

      size_t move_count = uo_position_generate_moves(&engine.position);

      for (size_t i = 0; i < move_count; ++i)
      {
        uo_move move = engine.position.movelist.head[i];
        if (square_from == uo_move_square_from(move)
          && square_to == uo_move_square_to(move))
        {
          uo_move_type move_type = uo_move_get_type(move);

          if ((move_type & uo_move_type__promo) && move_type_promo != (move_type & uo_move_type__promo_Q))
          {
            continue;
          }

          uo_position_make_move(&engine.position, move);
          goto next_move;
        }
      }

      uo_engine_unlock_position();

      // Not a legal move
      return;

    next_move:;
    }

    uo_position_reset_root(&engine.position);
  }

  uo_engine_unlock_position();
}

static void uo_uci_command__isready(void)
{
  uo_engine_lock_stdout();
  printf("readyok\n");
  uo_engine_unlock_stdout();
}

static void uo_uci_command__ucinewgame(void)
{
  uo_ttable_clear(&engine.ttable);
}

static void uo_uci_command__setoption(void)
{
  uo_uci_read_stdin();

  if (ptr && strcmp(ptr, "name") == 0)
  {
    ptr = strtok(NULL, "\n");

    int64_t spin;
    char check[6];
    char filepath[0x100];

    // Clear Hash
    if (ptr && strcmp(ptr, "Clear Hash") == 0)
    {
      uo_engine_clear_hash();
    }

    // Hash
    if (ptr && sscanf(ptr, "Hash value %" PRIi64, &spin) == 1 && spin >= 1 && spin <= 33554432)
    {
      engine_options.hash_size = spin;
      if (state == uo_uci_state_idle) uo_engine_reconfigure();
    }

    // Threads
    if (ptr && sscanf(ptr, "Threads value %" PRIi64, &spin) == 1 && spin >= 1 && spin <= 512)
    {
      engine_options.threads = spin;
      if (state == uo_uci_state_idle) uo_engine_reconfigure();
    }

    // MultiPV
    if (ptr && sscanf(ptr, "MultiPV value %" PRIi64, &spin) == 1 && spin >= 1 && spin <= 500)
    {
      engine_options.multipv = spin;
    }

    // EvalFile
    if (ptr && sscanf(ptr, "EvalFile value %255s", engine_options.eval_filename) == 1)
    {
      // 
    }

    // Move Overhead
    if (ptr && sscanf(ptr, "Move Overhead value %" PRIi64, &spin) == 1 && spin >= 1 && spin <= 5000)
    {
      engine_options.move_overhead = spin;
    }

    // OwnBook
    if (ptr && sscanf(ptr, "OwnBook value %5s", check) == 1)
    {
      if (strcmp(check, "true") == 0)
      {
        engine_options.use_own_book = true;
        if (state == uo_uci_state_idle) uo_engine_reconfigure();
      }
      else if (strcmp(check, "false") == 0)
      {
        engine_options.use_own_book = false;
        if (state == uo_uci_state_idle) uo_engine_reconfigure();
      }
    }

    // BookFile
    if (ptr && sscanf(ptr, "BookFile value %255s", filepath) == 1)
    {
      strcpy(engine_options.book_filename, filepath);
      if (state == uo_uci_state_idle) uo_engine_reconfigure();
    }

    // SyzygyPath
    if (ptr && sscanf(ptr, "SyzygyPath value %255s", filepath) == 1)
    {
      strcpy(engine_options.tb.syzygy.dir, filepath);
    }

    // SyzygyProbeDepth
    if (ptr && sscanf(ptr, "SyzygyProbeDepth value %" PRIi64, &spin) == 1 && spin >= 1 && spin <= 100)
    {
      engine_options.tb.syzygy.probe_depth = spin;
    }

    // Syzygy50MoveRule
    if (ptr && sscanf(ptr, "Syzygy50MoveRule value %5s", check) == 1)
    {
      if (strcmp(check, "true") == 0)
      {
        engine_options.tb.syzygy.rule50 = true;
      }
      else if (strcmp(check, "false") == 0)
      {
        engine_options.tb.syzygy.rule50 = false;
      }
    }

    // SyzygyProbeLimit
    if (ptr && sscanf(ptr, "SyzygyProbeLimit value %" PRIi64, &spin) == 1 && spin >= 0 && spin <= 7)
    {
      engine_options.tb.syzygy.probe_limit = spin;
    }
  }
}

static void uo_uci_command__go(void)
{
  uo_uci_read_stdin();

  if (ptr && strcmp(ptr, "perft") == 0)
  {
    uo_uci_read_stdin();

    int depth;
    if (sscanf(ptr, "%d", &depth) == 1 && depth > 0)
    {
      uo_engine_lock_stdout();
      uo_engine_lock_position();

      uo_time time_start;
      uo_time_now(&time_start);

      size_t total_node_count = 0;
      size_t move_count = uo_position_generate_moves(&engine.position);

      for (size_t i = 0; i < move_count; ++i)
      {
        uo_move move = engine.position.movelist.head[i];
        uo_position_print_move(&engine.position, move, buf);
        uo_position_make_move(&engine.position, move);
        size_t node_count = depth == 1 ? 1 : uo_position_perft(&engine.position, depth - 1);
        uo_position_unmake_move(&engine.position);
        total_node_count += node_count;
        printf("%s: %zu\n", buf, node_count);
      }

      double time_msec = uo_time_elapsed_msec(&time_start);
      uint64_t knps = total_node_count / time_msec;

      printf("\nNodes searched: %zu, time: %.03f s (%zu kN/s)\n\n", total_node_count, time_msec / 1000.0, knps);
      fflush(stdout);
      uo_engine_unlock_position();
      uo_engine_unlock_stdout();
    }
  }
  else
  {
    uo_engine_reset_search_params(uo_seach_type__principal_variation);

    while (ptr)
    {
      if (ptr && strcmp(ptr, "searchmoves") == 0)
      {
        // TODO
        uo_uci_read_stdin();
        continue;
      }

      if (ptr && strcmp(ptr, "depth") == 0)
      {
        uo_uci_read_stdin();

        if (ptr)
        {
          engine.search_params.depth = strtoul(ptr, NULL, 10);
          uo_uci_read_stdin();
          continue;
        }
      }

      if (ptr && strcmp(ptr, "wtime") == 0)
      {
        uo_uci_read_stdin();

        if (ptr)
        {
          uint32_t *time = uo_color(engine.position.flags) == uo_white
            ? &engine.search_params.time_own
            : &engine.search_params.time_enemy;

          *time = strtoul(ptr, NULL, 10);
          uo_uci_read_stdin();
          continue;
        }
      }

      if (ptr && strcmp(ptr, "btime") == 0)
      {
        uo_uci_read_stdin();

        if (ptr)
        {
          uint32_t *time = uo_color(engine.position.flags) == uo_black
            ? &engine.search_params.time_own
            : &engine.search_params.time_enemy;

          *time = strtoul(ptr, NULL, 10);
          uo_uci_read_stdin();
          continue;
        }
      }

      if (ptr && strcmp(ptr, "winc") == 0)
      {
        uo_uci_read_stdin();

        if (ptr)
        {
          uint32_t *time = uo_color(engine.position.flags) == uo_white
            ? &engine.search_params.time_inc_own
            : &engine.search_params.time_inc_enemy;

          *time = strtoul(ptr, NULL, 10);
          uo_uci_read_stdin();
          continue;
        }
      }

      if (ptr && strcmp(ptr, "binc") == 0)
      {
        uo_uci_read_stdin();

        if (ptr)
        {
          uint32_t *time = uo_color(engine.position.flags) == uo_black
            ? &engine.search_params.time_inc_own
            : &engine.search_params.time_inc_enemy;

          *time = strtoul(ptr, NULL, 10);
          uo_uci_read_stdin();
          continue;
        }
      }

      if (ptr && strcmp(ptr, "movestogo") == 0)
      {
        uo_uci_read_stdin();

        if (ptr)
        {
          engine.search_params.movestogo = strtoul(ptr, NULL, 10);
          uo_uci_read_stdin();
          continue;
        }
      }

      if (ptr && strcmp(ptr, "ponder") == 0)
      {
        engine.search_params.ponder = true;
        uo_uci_read_stdin();
        continue;
      }

      if (ptr && strcmp(ptr, "nodes") == 0)
      {
        uo_uci_read_stdin();

        if (ptr)
        {
          engine.search_params.nodes = strtoull(ptr, NULL, 10);
          uo_uci_read_stdin();
          continue;
        }
      }

      if (ptr && strcmp(ptr, "mate") == 0)
      {
        uo_uci_read_stdin();

        if (ptr)
        {
          engine.search_params.mate = strtoul(ptr, NULL, 10);
          uo_uci_read_stdin();
          continue;
        }
      }

      if (ptr && strcmp(ptr, "movetime") == 0)
      {
        uo_uci_read_stdin();

        if (ptr)
        {
          engine.search_params.movetime = strtoull(ptr, NULL, 10);
          uo_uci_read_stdin();
          continue;
        }
      }

      if (ptr && strcmp(ptr, "alpha") == 0)
      {
        uo_uci_read_stdin();

        if (ptr)
        {
          engine.search_params.alpha = strtol(ptr, NULL, 10);
          uo_uci_read_stdin();
          continue;
        }
      }

      if (ptr && strcmp(ptr, "beta") == 0)
      {
        uo_uci_read_stdin();

        if (ptr)
        {
          engine.search_params.beta = strtol(ptr, NULL, 10);
          uo_uci_read_stdin();
          continue;
        }
      }

      uo_uci_read_stdin();
    }

    uo_engine_start_search();
    state = uo_uci_state_running;
    return;
  }
}

static void uo_uci_command__test__see(void)
{
  uo_engine_lock_stdout();
  uo_engine_lock_position();

  size_t move_count = uo_position_generate_moves(&engine.position);

  for (size_t i = 0; i < move_count; ++i)
  {
    uo_move move = engine.position.movelist.head[i];
    int16_t gain = uo_position_move_see(&engine.position, move, NULL);

    if (gain != 0 || uo_move_is_capture(move))
    {
      uo_position_print_move(&engine.position, move, buf);

      printf("%s: %d\n", buf, gain);
    }
  }

  printf("\n");
  fflush(stdout);
  uo_engine_unlock_position();
  uo_engine_unlock_stdout();
}

static void uo_uci_command__test__checks(void)
{
  uo_engine_lock_stdout();
  uo_engine_lock_position();

  size_t move_count = uo_position_generate_moves(&engine.position);

  for (size_t i = 0; i < move_count; ++i)
  {
    uo_move move = engine.position.movelist.head[i];

    if (uo_position_move_checks(&engine.position, move, NULL))
    {
      uo_position_print_move(&engine.position, move, buf);

      printf("%s\n", buf);
    }
  }

  printf("\n");
  fflush(stdout);
  uo_engine_unlock_position();
  uo_engine_unlock_stdout();
}

static void uo_uci_command__test__qsearch(void)
{
  uo_uci_read_stdin();
  uo_engine_reset_search_params(uo_seach_type__quiescence);

  while (ptr)
  {
    if (ptr && strcmp(ptr, "alpha") == 0)
    {
      uo_uci_read_stdin();

      if (ptr)
      {
        engine.search_params.alpha = strtol(ptr, NULL, 10);
        uo_uci_read_stdin();
        continue;
      }
    }

    if (ptr && strcmp(ptr, "beta") == 0)
    {
      uo_uci_read_stdin();

      if (ptr)
      {
        engine.search_params.beta = strtol(ptr, NULL, 10);
        uo_uci_read_stdin();
        continue;
      }
    }

    uo_uci_read_stdin();
  }

  uo_engine_start_search();
  state = uo_uci_state_running;
}

static void uo_uci_command__test(void)
{
  uo_uci_read_stdin();

  static uo_strmap *uci_command_map__test = NULL;

  if (!uci_command_map__test)
  {
    uci_command_map__test = uo_strmap_create();
    uo_strmap_add(uci_command_map__test, "see", uo_uci_command__test__see);
    uo_strmap_add(uci_command_map__test, "checks", uo_uci_command__test__checks);
    uo_strmap_add(uci_command_map__test, "qsearch", uo_uci_command__test__qsearch);

  }

  uo_uci_command command = uo_strmap_get(uci_command_map__test, ptr);
  if (command) command();
}

static void uo_uci_command_init__uci(void)
{
  printf("id name uochess\n");
  printf("id author Leevi Uotinen\n\n");
  printf("option name Debug Log File type string default\n");
  printf("option name Threads type spin default %zu min 1 max 254\n", engine_options.threads);
  printf("option name Hash type spin default %zu min 1 max 33554432\n", engine_options.hash_size);
  printf("option name Move Overhead type spin default %zu min 1 max 5000\n", engine_options.move_overhead);
  printf("option name Clear Hash type button\n");
  printf("option name Ponder type check default false\n");
  printf("option name OwnBook type check default true\n");
  printf("option name BookFile type string default %s\n", engine_options.book_filename);
  printf("option name MultiPV type spin default 1 min 1 max 500\n");
  printf("option name SyzygyPath type string default %s\n", engine_options.tb.syzygy.dir);
  printf("option name SyzygyProbeDepth type spin default 1 min 1 max 100\n");
  printf("option name Syzygy50MoveRule type check default true\n");
  printf("option name SyzygyProbeLimit type spin default 7 min 0 max 7\n");
  //printf("option name EvalFile type string default %s\n", engine_options.eval_filename);

  state = uo_uci_state_config;
  printf("uciok\n");
  fflush(stdout);
}

static void uo_uci_command_config__isready(void)
{
  uo_engine_init();
  state = uo_uci_state_idle;
  printf("readyok\n");
  fflush(stdout);
}

static void uo_uci_command__eval(void)
{
  uo_uci_command__d();

  uo_engine_lock_stdout();
  uo_engine_lock_position();

  uo_evaluation_info eval_info;
  int16_t value = uo_position_evaluate(&engine.position, &eval_info);

  int16_t mate_threat_own = eval_info.attack_units_enemy;
  int16_t mate_threat_enemy = eval_info.attack_units_own;

  uint16_t color = uo_color(engine.position.flags) == uo_white ? 1 : -1;

  printf("Mate threats\n");
  printf(" towards white: %d\n", color == 1 ? mate_threat_own : mate_threat_enemy);
  printf(" towards black: %d\n", color == 1 ? mate_threat_enemy : mate_threat_own);

  int16_t score = color * value;
  printf("Evaluation: %d", score);

  if (uo_position_is_quiescent(&engine.position))
  {
    printf("\n\n");
  }
  else
  {
    printf(" (Position not quiescent)\n\n");
  }

  fflush(stdout);
  uo_engine_unlock_position();
  uo_engine_unlock_stdout();
}

static void uo_uci_command__stop(void)
{
  uo_engine_stop_search();
  state = uo_uci_state_idle;
  return;
}


int uo_uci_run()
{
  uo_strmap *uci_command_map_by_state[] =
  {
    [uo_uci_state_init] = NULL,
    [uo_uci_state_config] = NULL,
    [uo_uci_state_idle] = NULL,
    [uo_uci_state_running] = NULL
  };

  uo_strmap *uci_command_map_init = uci_command_map_by_state[uo_uci_state_init] = uo_strmap_create();
  uo_strmap_add(uci_command_map_init, "quit", uo_uci_command__quit);
  uo_strmap_add(uci_command_map_init, "uci", uo_uci_command_init__uci);

  uo_strmap *uci_command_map_config = uci_command_map_by_state[uo_uci_state_config] = uo_strmap_create();
  uo_strmap_add(uci_command_map_config, "quit", uo_uci_command__quit);
  uo_strmap_add(uci_command_map_config, "setoption", uo_uci_command__setoption);
  uo_strmap_add(uci_command_map_config, "isready", uo_uci_command_config__isready);

  uo_strmap *uci_command_map_idle = uci_command_map_by_state[uo_uci_state_idle] = uo_strmap_create();
  uo_strmap_add(uci_command_map_idle, "quit", uo_uci_command__quit);
  uo_strmap_add(uci_command_map_idle, "d", uo_uci_command__d);
  uo_strmap_add(uci_command_map_idle, "isready", uo_uci_command__isready);
  uo_strmap_add(uci_command_map_idle, "position", uo_uci_command__position);
  uo_strmap_add(uci_command_map_idle, "ucinewgame", uo_uci_command__ucinewgame);
  uo_strmap_add(uci_command_map_idle, "isready", uo_uci_command__isready);
  uo_strmap_add(uci_command_map_idle, "setoption", uo_uci_command__setoption);
  uo_strmap_add(uci_command_map_idle, "eval", uo_uci_command__eval);
  uo_strmap_add(uci_command_map_idle, "go", uo_uci_command__go);
  uo_strmap_add(uci_command_map_idle, "test", uo_uci_command__test);

  uo_strmap *uci_command_map_running = uci_command_map_by_state[uo_uci_state_running] = uo_strmap_create();
  uo_strmap_add(uci_command_map_running, "quit", uo_uci_command__quit);
  uo_strmap_add(uci_command_map_running, "stop", uo_uci_command__stop);
  uo_strmap_add(uci_command_map_running, "d", uo_uci_command__d);
  uo_strmap_add(uci_command_map_running, "isready", uo_uci_command__isready);


  printf("Uochess 0.1 by Leevi Uotinen\n");
  fflush(stdout);

  while (true)
  {
    while (!uo_uci_read_stdin())
    {
      // Read until we have a value
    }

    if (state == uo_uci_state_running && uo_engine_is_stopped())
    {
      state = uo_uci_state_idle;
    }

    uo_uci_command command = uo_strmap_get(uci_command_map_by_state[state], ptr);
    if (command) command();
  }
}
