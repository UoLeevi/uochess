#include "uo_test.h"
#include "uo_global.h"
#include "uo_util.h"
#include "uo_misc.h"
#include "uo_position.h"
#include "uo_search.h"
#include "uo_engine.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

bool uo_test_move_generation(uo_position *position, char *test_data_dir)
{
  if (!test_data_dir) return false;

  bool passed = true;

  size_t test_count = 0;

  char *filepath = buf;

  strcpy(filepath, test_data_dir);
  strcpy(filepath + strlen(test_data_dir), "/move_generation.txt");

  uo_file_mmap *file_mmap = uo_file_mmap_open_read(filepath);
  if (!file_mmap)
  {
    printf("Cannot open file '%s'", filepath);
    return false;
  }

  size_t move_count;


  char *ptr = uo_file_mmap_readline(file_mmap);
  char *token_context;

  if (!ptr)
  {
    printf("Error while reading test data\n");
    uo_file_mmap_close(file_mmap);
    return false;
  }

  while (ptr)
  {
    if (strlen(ptr) == 0)
    {
      ptr = uo_file_mmap_readline(file_mmap);
      continue;
    }

    ptr = uo_strtok(ptr, " ", &token_context);

    if (strcmp(ptr, "ucinewgame") == 0)
    {
      if (!(ptr = uo_file_mmap_readline(file_mmap)))
      {
        printf("Unexpected EOF while reading test data\n");
        uo_file_mmap_close(file_mmap);
        return false;
      }

      ptr = uo_strtok(ptr, " ", &token_context);
    }

    if (!ptr || strcmp(ptr, "position"))
    {
      printf("Expected to read 'position', instead read '%s'\n", ptr);
      uo_file_mmap_close(file_mmap);
      return false;
    }

    ptr = uo_strtok(NULL, " ", &token_context);

    if (!ptr || strcmp(ptr, "fen"))
    {
      printf("Expected to read 'fen', instead read '%s'\n", ptr);
      uo_file_mmap_close(file_mmap);
      return false;
    }

    char *fen = uo_strtok(NULL, "\r\n", &token_context);

    if (!uo_position_from_fen(position, fen))
    {
      printf("Error while parsing fen '%s'\n", fen);
      uo_file_mmap_close(file_mmap);
      return false;
    }

    if (!(ptr = uo_file_mmap_readline(file_mmap)))
    {
      printf("Unexpected EOF while reading test data\n");
      uo_file_mmap_close(file_mmap);
      return false;
    }

    size_t depth;

    if (!ptr || sscanf(ptr, "go perft %zu", &depth) != 1 || depth < 1)
    {
      printf("Expected to read 'go perft', instead read '%s'\n", ptr);
      uo_file_mmap_close(file_mmap);
      return false;
    }

    uint64_t key = position->key;

    move_count = uo_position_generate_moves(position);

    while ((ptr = uo_file_mmap_readline(file_mmap)))
    {
      if (strlen(ptr) == 0) break;

      char *move_str = uo_strtok(ptr, ":\r\n", &token_context);

      uo_move move = uo_position_parse_move(position, move_str);
      if (!move)
      {
        printf("Error while parsing move '%s'", move_str);
        uo_file_mmap_close(file_mmap);
        return false;
      }

      uo_square square_from = uo_move_square_from(move);
      uo_square square_to = uo_move_square_to(move);
      uo_move_type move_type_promo = uo_move_get_type(move) & uo_move_type__promo_Q;

      size_t node_count_expected;

      ptr = uo_strtok(NULL, "\r\n", &token_context);

      if (sscanf(ptr, " %zu", &node_count_expected) != 1)
      {
        printf("Error while node count for move\r\n");
        uo_file_mmap_close(file_mmap);
        return false;
      }

      for (int64_t i = 0; i < move_count; ++i)
      {
        uo_move move = position->movelist.head[i];
        if (uo_move_square_from(move) == square_from && uo_move_square_to(move) == square_to)
        {
          uo_move_type move_type = uo_move_get_type(move);

          if ((move_type & uo_move_type__promo) && move_type_promo != (move_type & uo_move_type__promo_Q))
          {
            continue;
          }

          // move found

          uo_position_make_move(position, move);
          size_t node_count = depth == 1 ? 1 : uo_position_perft(position, depth - 1);

          if (node_count != node_count_expected)
          {
            printf("TEST 'move_generation' FAILED: generated node count %zu for move '%s' did not match expected count %zu for fen '%s'\n", node_count, move_str, node_count_expected, fen);
            uo_position_print_diagram(position, buf);
            printf("\r\n%s", buf);
            uo_position_print_fen(position, buf);
            printf("\nFen: %s\n", buf);

            uo_file_mmap_close(file_mmap);
            return false;
          }

          uo_position_unmake_move(position);

          if (position->key != key)
          {
            printf("TEST 'move_generation' FAILED: position key '%" PRIu64 "' after unmake move '%s' did not match key '%" PRIu64 "' for fen '%s'\r\n", position->key, move_str, key, fen);
            uo_position_print_diagram(position, buf);
            printf("\n%s", buf);
            uo_position_print_fen(position, buf);
            printf("\nFen: %s\n", buf);

            uo_file_mmap_close(file_mmap);
            return false;
          }

          position->movelist.head[i] = (uo_move){ 0 };
          goto next_move;
        }
      }

      printf("TEST 'move_generation' FAILED: move '%s' was not generated for fen '%s'", move_str, fen);
      uo_file_mmap_close(file_mmap);
      return false;

    next_move:;
    }

    for (int64_t i = 0; i < move_count; ++i)
    {
      uo_move move = position->movelist.head[i];
      if (move)
      {
        uo_square square_from = uo_move_square_from(move);
        uo_square square_to = uo_move_square_to(move);
        printf("TEST 'move_generation' FAILED: move '%c%d%c%d' is not a legal move for fen '%s'",
          'a' + uo_square_file(square_from), 1 + uo_square_rank(square_from),
          'a' + uo_square_file(square_to), 1 + uo_square_rank(square_to),
          fen);

        uo_file_mmap_close(file_mmap);
        return false;
      }
    }

    ++test_count;
    ptr = uo_file_mmap_readline(file_mmap);
  }

  uo_file_mmap_close(file_mmap);

  if (passed)
  {
    printf("TEST 'move_generation' PASSED: total of %zd positions tested.", test_count);
  }

  return passed;
}

bool uo_test_tb_probe(uo_position *position, char *test_data_dir)
{
  if (!test_data_dir) return false;

  bool passed = true;

  size_t test_count = 0;

  char *filepath = buf;

  strcpy(filepath, test_data_dir);
  strcpy(filepath + strlen(test_data_dir), "/tb_probe.txt");

  uo_file_mmap *file_mmap = uo_file_mmap_open_read(filepath);
  if (!file_mmap)
  {
    printf("Cannot open file '%s'", filepath);
    return false;
  }

  size_t move_count;


  char *ptr = uo_file_mmap_readline(file_mmap);
  char *token_context;

  if (!ptr)
  {
    printf("Error while reading test data\n");
    uo_file_mmap_close(file_mmap);
    return false;
  }

  char *fen = NULL;

  while (ptr)
  {
    if (strlen(ptr) == 0)
    {
      ptr = uo_file_mmap_readline(file_mmap);
      continue;
    }

    if (sscanf(ptr, "position fen %s", buf) == 1)
    {
      fen = ptr + sizeof("position fen ") - 1;
      if (!uo_position_from_fen(position, fen))
      {
        printf("Error while parsing fen '%s'\n", fen);
        uo_file_mmap_close(file_mmap);
        return false;
      }

      uo_position_print_fen(position, buf);
      assert(strcmp(buf, fen) == 0);
      fen = buf;

      ptr = uo_file_mmap_readline(file_mmap);
      continue;
    }

    int expected;
    int success;

    if (sscanf(ptr, "wdl %d", &expected) == 1)
    {
      int value = uo_tb_probe_wdl(position, &success);
      if (value != expected)
      {
        printf("TEST 'tb_probe' FAILED: WDL probe returned incorrect value %d when %d was expected for fen '%s'\r\n", value, expected, fen);
        uo_file_mmap_close(file_mmap);
        return false;
      }

      ++test_count;
      ptr = uo_file_mmap_readline(file_mmap);
      continue;
    }

    if (sscanf(ptr, "dtz %d", &expected) == 1)
    {
      int value = uo_tb_probe_dtz(position, &success);
      if (value != expected)
      {
        printf("TEST 'tb_probe' FAILED: DTZ probe returned incorrect value %d when %d was expected for fen '%s'\r\n", value, expected, fen);
        uo_file_mmap_close(file_mmap);
        return false;
      }

      ++test_count;
      ptr = uo_file_mmap_readline(file_mmap);
      continue;
    }

    printf("Unknown command '%s'\n", ptr);
    uo_file_mmap_close(file_mmap);
    return false;
  }

  uo_file_mmap_close(file_mmap);

  if (passed)
  {
    printf("TEST 'tb_probe' PASSED: total of %zd positions tested.", test_count);
  }

  return passed;
}
