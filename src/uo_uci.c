#include "uo_uci.h"
#include "uo_misc.h"
#include "uo_engine.h"
#include "uo_evaluation.h"
#include "uo_def.h"
#include "uo_global.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef int uo_uci_token;

typedef const struct uo_uci_tokens
{
  uo_uci_token
    empty,
    unknown,
    error,
    quit,
    stop,
    debug,
    uci,
    isready,
    d,
    depth,
    setoption,
    go,
    perft,
    infinite,
    fen,
    moves,
    ucinewgame,
    startpos,
    position,
    name,
    wtime,
    btime,
    winc,
    binc,
    movestogo,
    searchmoves,
    ponder,
    nodes,
    mate,
    movetime,
    test,
    sse,
    eval,
    qsearch,
    alpha,
    beta;

  struct
  {
    uo_uci_token
      Threads,
      Hash,
      MultiPV;
  } options;
} uo_uci_tokens;

typedef int uo_uci_state;

typedef const struct uo_uci_states
{
  uo_uci_state
    init,
    options,
    ready,
    position,
    go;
} uo_uci_states;

static uo_uci_token token;

uo_uci_tokens tokens = {
  .empty = 0,
  .unknown = 1,
  .error = -2,
  .quit = -1,
  .stop = 2,
  .debug = 3,
  .uci = 4,
  .isready = 5,
  .d = 6,
  .depth = 7,
  .setoption = 8,
  .go = 9,
  .perft = 10,
  .infinite = 11,
  .fen = 12,
  .moves = 13,
  .ucinewgame = 14,
  .startpos = 15,
  .position = 16,
  .name = 17,
  .wtime = 18,
  .btime = 19,
  .winc = 20,
  .binc = 21,
  .movestogo = 22,
  .searchmoves = 23,
  .ponder = 24,
  .nodes = 25,
  .mate = 26,
  .movetime = 27,
  .test = 28,
  .sse = 29,
  .eval = 30,
  .qsearch = 31,
  .alpha = 32,
  .beta = 33,
  .options = {
    .Threads = 64,
    .Hash = 65,
    .MultiPV = 66
  }
};

static uo_uci_state state;

#define uo_uci_state_init 0
#define uo_uci_state_options 1
#define uo_uci_state_ready 2
#define uo_uci_state_go 3

static uo_uci_states states = {
  .init = uo_uci_state_init,
  .options = uo_uci_state_options,
  .ready = uo_uci_state_ready,
  .go = uo_uci_state_go
};

static FILE *log_file;
static char *ptr;

static inline bool uo_uci_match(uo_uci_token match)
{
  return match == token;
}

static void uo_uci_read_token(void)
{
  if (ptr == NULL)
  {
    if (fgets(buf, sizeof buf, stdin) == NULL)
    {
      token = tokens.error;
      return;
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

  if (!ptr)
  {
    token = tokens.empty;
    return;
  }

#define streq(lhs, rhs) strcmp(lhs, rhs) == 0

  if (streq(ptr, "quit"))
  {
    token = tokens.quit;
    return;
  }

  if (streq(ptr, "stop"))
  {
    token = tokens.stop;
    return;
  }

  if (streq(ptr, "debug"))
  {
    token = tokens.debug;
    return;
  }

  if (streq(ptr, "uci"))
  {
    token = tokens.uci;
    return;
  }

  if (streq(ptr, "isready"))
  {
    token = tokens.isready;
    return;
  }

  if (streq(ptr, "d"))
  {
    token = tokens.d;
    return;
  }

  if (streq(ptr, "depth"))
  {
    token = tokens.depth;
    return;
  }

  if (streq(ptr, "setoption"))
  {
    token = tokens.setoption;
    return;
  }

  if (streq(ptr, "go"))
  {
    token = tokens.go;
    return;
  }

  if (streq(ptr, "perft"))
  {
    token = tokens.perft;
    return;
  }

  if (streq(ptr, "infinite"))
  {
    token = tokens.infinite;
    return;
  }

  if (streq(ptr, "fen"))
  {
    token = tokens.fen;
    return;
  }

  if (streq(ptr, "moves"))
  {
    token = tokens.moves;
    return;
  }

  if (streq(ptr, "ucinewgame"))
  {
    token = tokens.ucinewgame;
    return;
  }

  if (streq(ptr, "startpos"))
  {
    token = tokens.startpos;
    return;
  }

  if (streq(ptr, "position"))
  {
    token = tokens.position;
    return;
  }

  if (streq(ptr, "name"))
  {
    token = tokens.name;
    return;
  }

  if (streq(ptr, "wtime"))
  {
    token = tokens.wtime;
    return;
  }

  if (streq(ptr, "btime"))
  {
    token = tokens.btime;
    return;
  }

  if (streq(ptr, "winc"))
  {
    token = tokens.winc;
    return;
  }

  if (streq(ptr, "binc"))
  {
    token = tokens.binc;
    return;
  }

  if (streq(ptr, "movestogo"))
  {
    token = tokens.movestogo;
    return;
  }

  if (streq(ptr, "searchmoves"))
  {
    token = tokens.searchmoves;
    return;
  }

  if (streq(ptr, "ponder"))
  {
    token = tokens.ponder;
    return;
  }

  if (streq(ptr, "nodes"))
  {
    token = tokens.nodes;
    return;
  }

  if (streq(ptr, "mate"))
  {
    token = tokens.mate;
    return;
  }

  if (streq(ptr, "movetime"))
  {
    token = tokens.movetime;
    return;
  }

  if (streq(ptr, "test"))
  {
    token = tokens.test;
    return;
  }

  if (streq(ptr, "sse"))
  {
    token = tokens.sse;
    return;
  }

  if (streq(ptr, "eval"))
  {
    token = tokens.eval;
    return;
  }

  if (streq(ptr, "qsearch"))
  {
    token = tokens.qsearch;
    return;
  }

  if (streq(ptr, "alpha"))
  {
    token = tokens.alpha;
    return;
  }

  if (streq(ptr, "beta"))
  {
    token = tokens.beta;
    return;
  }

  token = tokens.unknown;
  return;

#undef streq
}

static void uo_uci__d(void)
{
  uo_engine_lock_position();
  uo_engine_lock_stdout();
  uo_position_print_diagram(&engine.position, buf);
  printf("\n%s", buf);
  uo_position_print_fen(&engine.position, buf);
  printf("\n");
  printf("Fen: %s\n", buf);
  printf("Key: %" PRIu64 "\n", engine.position.key);
  uo_engine_unlock_stdout();
  uo_engine_unlock_position();
}

static void uo_uci__position(void)
{
  uo_uci_read_token();

  uo_engine_lock_position();

  if (uo_uci_match(tokens.startpos))
  {
    uo_position_from_fen(&engine.position, uo_fen_startpos);
    uo_uci_read_token();
  }
  else if (uo_uci_match(tokens.fen))
  {
    ptr = strtok(NULL, "\n");

    char *fen = strtok(ptr, "m"); /* moves */
    uo_uci_read_token();

    if (ptr && strcmp(ptr, "oves") == 0)
    {
      --ptr;
      *ptr = 'm';
      token = tokens.moves;
    }

    uo_position_from_fen(&engine.position, fen);
  }
  else
  {
    uo_engine_unlock_position();
    // unexpected command
    return;
  }

  if (uo_uci_match(tokens.moves))
  {
    while (ptr = strtok(NULL, " \n"))
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

static void uo_uci_process_input__init(void)
{
  if (uo_uci_match(tokens.uci))
  {
    printf("id name uochess\n");
    printf("id author Leevi Uotinen\n\n");
    printf("option name Debug Log File type string default\n");
    printf("option name Threads type spin default 1 min 1 max 512\n");
    printf("option name Hash type spin default 16 min 1 max 33554432\n");
    printf("option name Clear Hash type button\n");
    printf("option name Ponder type check default false\n");
    printf("option name MultiPV type spin default 1 min 1 max 500\n");

    state = states.options;
    printf("uciok\n");
    fflush(stdout);
  }
}

static void uo_uci_process_input__options(void)
{
  if (uo_uci_match(tokens.setoption))
  {
    uo_uci_read_token();

    if (uo_uci_match(tokens.name))
    {
      ptr = strtok(NULL, "\n");

      int64_t spin;

      // Threads
      if (ptr && sscanf(ptr, "Threads value %" PRIi64, &spin) == 1 && spin >= 1 && spin <= 512)
      {
        engine_options.threads = spin;
      }

      // Hash
      if (ptr && sscanf(ptr, "Hash value %" PRIi64, &spin) == 1 && spin >= 1 && spin <= 33554432)
      {
        size_t capacity = spin * (size_t)1000000 / sizeof * engine.ttable.entries;
        engine_options.hash_size = ((size_t)1 << uo_msb(capacity)) * sizeof * engine.ttable.entries;
      }

      // MultiPV
      if (ptr && sscanf(ptr, "MultiPV value %" PRIi64, &spin) == 1 && spin >= 1 && spin <= 500)
      {
        engine_options.multipv = spin;
      }
    }
  }
  else if (uo_uci_match(tokens.isready))
  {
    uo_engine_init();
    state = states.ready;
    printf("readyok\n");
    fflush(stdout);
  }
}

static void uo_uci_process_input__ready(void)
{
  if (uo_uci_match(tokens.isready))
  {
    uo_engine_lock_stdout();
    printf("readyok\n");
    uo_engine_unlock_stdout();
  }

  if (uo_uci_match(tokens.position))
  {
    uo_uci__position();
    return;
  }

  if (uo_uci_match(tokens.ucinewgame))
  {
    uo_engine_lock_ttable();
    uo_ttable_clear(&engine.ttable);
    uo_engine_unlock_ttable();
    return;
  }

  if (uo_uci_match(tokens.setoption))
  {
    uo_uci_read_token();
    if (uo_uci_match(tokens.name))
    {
      ptr = strtok(NULL, "\n");

      int64_t spin;

      // Clear Hash
      if (ptr && strcmp(ptr, "Clear Hash") == 0)
      {
        uo_engine_clear_hash();
      }
      return;
    }
  }

  if (uo_uci_match(tokens.d))
  {
    uo_uci__d();
    return;
  }

  if (uo_uci_match(tokens.go))
  {
    uo_uci_read_token();

    if (uo_uci_match(tokens.perft))
    {
      uo_uci_read_token();

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
        if (uo_uci_match(tokens.searchmoves))
        {
          // TODO
          uo_uci_read_token();
          continue;
        }

        if (uo_uci_match(tokens.depth))
        {
          uo_uci_read_token();

          if (ptr)
          {
            engine.search_params.depth = strtoul(ptr, NULL, 10);
            uo_uci_read_token();
            continue;
          }
        }

        if (uo_uci_match(tokens.wtime))
        {
          uo_uci_read_token();

          if (ptr)
          {
            uint32_t *time = uo_color(engine.position.flags) == uo_white
              ? &engine.search_params.time_own
              : &engine.search_params.time_enemy;

            *time = strtoul(ptr, NULL, 10);
            uo_uci_read_token();
            continue;
          }
        }

        if (uo_uci_match(tokens.btime))
        {
          uo_uci_read_token();

          if (ptr)
          {
            uint32_t *time = uo_color(engine.position.flags) == uo_black
              ? &engine.search_params.time_own
              : &engine.search_params.time_enemy;

            *time = strtoul(ptr, NULL, 10);
            uo_uci_read_token();
            continue;
          }
        }

        if (uo_uci_match(tokens.winc))
        {
          uo_uci_read_token();

          if (ptr)
          {
            uint32_t *time = uo_color(engine.position.flags) == uo_white
              ? &engine.search_params.time_inc_own
              : &engine.search_params.time_inc_enemy;

            *time = strtoul(ptr, NULL, 10);
            uo_uci_read_token();
            continue;
          }
        }

        if (uo_uci_match(tokens.binc))
        {
          uo_uci_read_token();

          if (ptr)
          {
            uint32_t *time = uo_color(engine.position.flags) == uo_black
              ? &engine.search_params.time_inc_own
              : &engine.search_params.time_inc_enemy;

            *time = strtoul(ptr, NULL, 10);
            uo_uci_read_token();
            continue;
          }
        }

        if (uo_uci_match(tokens.movestogo))
        {
          uo_uci_read_token();

          if (ptr)
          {
            engine.search_params.movestogo = strtoul(ptr, NULL, 10);
            uo_uci_read_token();
            continue;
          }
        }

        if (uo_uci_match(tokens.ponder))
        {
          engine.search_params.ponder = true;
          uo_uci_read_token();
          continue;
        }

        if (uo_uci_match(tokens.nodes))
        {
          uo_uci_read_token();

          if (ptr)
          {
            engine.search_params.nodes = strtoull(ptr, NULL, 10);
            uo_uci_read_token();
            continue;
          }
        }

        if (uo_uci_match(tokens.mate))
        {
          uo_uci_read_token();

          if (ptr)
          {
            engine.search_params.mate = strtoul(ptr, NULL, 10);
            uo_uci_read_token();
            continue;
          }
        }

        if (uo_uci_match(tokens.movetime))
        {
          uo_uci_read_token();

          if (ptr)
          {
            engine.search_params.movetime = strtoull(ptr, NULL, 10);
            uo_uci_read_token();
            continue;
          }
        }

        if (uo_uci_match(tokens.alpha))
        {
          uo_uci_read_token();

          if (ptr)
          {
            engine.search_params.alpha = strtol(ptr, NULL, 10);
            uo_uci_read_token();
            continue;
          }
        }

        if (uo_uci_match(tokens.beta))
        {
          uo_uci_read_token();

          if (ptr)
          {
            engine.search_params.beta = strtol(ptr, NULL, 10);
            uo_uci_read_token();
            continue;
          }
        }

        uo_uci_read_token();
      }

      uo_engine_start_search();
      state = states.go;
      return;
    }
  }

  if (uo_uci_match(tokens.test))
  {
    uo_uci_read_token();

    if (uo_uci_match(tokens.sse))
    {
      uo_engine_lock_stdout();
      uo_engine_lock_position();

      size_t move_count = uo_position_generate_moves(&engine.position);

      for (size_t i = 0; i < move_count; ++i)
      {
        uo_move move = engine.position.movelist.head[i];
        int16_t gain = uo_position_move_sse(&engine.position, move);

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

    if (uo_uci_match(tokens.qsearch))
    {
      uo_uci_read_token();
      uo_engine_reset_search_params(uo_seach_type__quiescence);

      while (ptr)
      {
        if (uo_uci_match(tokens.alpha))
        {
          uo_uci_read_token();

          if (ptr)
          {
            engine.search_params.alpha = strtol(ptr, NULL, 10);
            uo_uci_read_token();
            continue;
          }
        }

        if (uo_uci_match(tokens.beta))
        {
          uo_uci_read_token();

          if (ptr)
          {
            engine.search_params.beta = strtol(ptr, NULL, 10);
            uo_uci_read_token();
            continue;
          }
        }

        uo_uci_read_token();
      }

      uo_engine_start_search();
      state = states.go;
      return;
    }
  }

  if (uo_uci_match(tokens.eval))
  {
    uo_uci__d();

    uo_engine_lock_stdout();
    uo_engine_lock_position();

    uint16_t color = uo_color(engine.position.flags) == uo_white ? 1 : -1;
    int16_t score = color * uo_position_evaluate(&engine.position);
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
}

static void uo_uci_process_input__go(void)
{
  if (uo_uci_match(tokens.stop))
  {
    uo_engine_stop_search();
    state = states.ready;
    return;
  }

  if (uo_engine_is_stopped())
  {
    state = states.ready;
    uo_uci_process_input__ready();
    return;
  }

  if (uo_uci_match(tokens.d))
  {
    uo_uci__d();
    return;
  }
}

static void (*process_input[])() = {
  [uo_uci_state_init] = uo_uci_process_input__init,
  [uo_uci_state_options] = uo_uci_process_input__options,
  [uo_uci_state_ready] = uo_uci_process_input__ready,
  [uo_uci_state_go] = uo_uci_process_input__go
};

int uo_uci_run()
{
  printf("Uochess 0.1 by Leevi Uotinen\n");
  fflush(stdout);
  uo_uci_read_token();

  while (token >= 0)
  {
    process_input[state]();
    uo_uci_read_token();
  }

  return token == tokens.quit ? 0 : 1;
}
