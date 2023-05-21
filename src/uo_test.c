#include "uo_test.h"
#include "uo_global.h"
#include "uo_util.h"
#include "uo_misc.h"
#include "uo_position.h"
#include "uo_search.h"
#include "uo_engine.h"
#include "uo_strmap.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct uo_test_info
{
  char *test_name;
  bool passed;
  size_t test_count;
  char *ptr;
  uo_file_mmap *file_mmap;
  uo_position position;
  char fen[0x100];
  char message[0x200];
} uo_test_info;

uo_strmap *test_command_map__go = NULL;
uo_strmap *test_command_map = NULL;

typedef bool (*uo_test_command)(uo_test_info *info);

bool uo_test__wdl(uo_test_info *info)
{
  int expected;
  if (sscanf(info->ptr, "wdl %d", &expected) != 1)
  {
    sprintf(info->message, "Error parsing command '%s'", info->ptr);
    return false;
  }

  int success;
  int wdl = uo_tb_probe_wdl(&info->position, &success);

  if (!success)
  {
    sprintf(info->message, "WDL probe failed for fen '%s'", info->fen);
    return false;
  }

  if (wdl != expected)
  {
    sprintf(info->message, "WDL probe returned incorrect value %d when %d was expected for fen '%s'", wdl, expected, info->fen);
    return false;
  }

  return true;
}

bool uo_test__dtz(uo_test_info *info)
{
  int expected;
  if (sscanf(info->ptr, "dtz %d", &expected) != 1)
  {
    sprintf(info->message, "Error parsing command '%s'", info->ptr);
    return false;
  }

  int success;
  int dtz = uo_tb_probe_dtz(&info->position, &success);

  if (!success)
  {
    sprintf(info->message, "DTZ probe failed for fen '%s'", info->fen);
    return false;
  }

  if (dtz != expected)
  {
    if (dtz < 0 && dtz > -100 && dtz - 1 == expected)
    {
      // OK
    }
    else if (dtz > 0 && dtz < 100 && dtz + 1 == expected)
    {
      // OK
    }
    else
    {
      sprintf(info->message, "DTZ probe returned incorrect value %d when %d was expected for fen '%s'", dtz, expected, info->fen);
      return false;
    }
  }

  return true;
}

bool uo_test__moveorder(uo_test_info *info)
{
  uo_position *position = &info->position;
  uint64_t key = position->key;

  size_t move_count = uo_position_generate_moves(position);
  uo_position_sort_moves(position, 0, NULL);

  char *token_context;

  int i = 0;

  while ((info->ptr = uo_file_mmap_readline(info->file_mmap)))
  {
    if (strlen(info->ptr) == 0) break;
    if (info->ptr[0] == '#') continue;

    if (i == move_count)
    {
      sprintf(info->message, "There are only %zu legal moves but more were listed.", move_count);
      return false;
    }

    uo_move move = uo_position_parse_move(position, info->ptr);
    if (!move)
    {
      sprintf(info->message, "Error while parsing move '%s'", info->ptr);
      return false;
    }

    if (move != position->movelist.head[i])
    {
      char move_str[6];
      uo_position_print_move(position, position->movelist.head[i], move_str);
      sprintf(info->message, "move %d. %s was expected, instead '%s' encountered for fen '%s'", i + 1, info->ptr, move_str, info->fen);
      return false;
    }
  }

  if (i < move_count)
  {
    // Not all legal moves where listed on the test case. This is fine
  }

  return true;
}

bool uo_test__go_perft(uo_test_info *info)
{
  size_t depth;

  if (sscanf(info->ptr, "go perft %zu", &depth) != 1 || depth < 1)
  {
    sprintf(info->message, "Expected to read 'go perft', instead read '%s'", info->ptr);
    return false;
  }

  uo_position *position = &info->position;
  uint64_t key = position->key;

  size_t move_count = uo_position_generate_moves(position);
  char *token_context;

  while ((info->ptr = uo_file_mmap_readline(info->file_mmap)))
  {
    if (strlen(info->ptr) == 0) break;

    char *move_str = uo_strtok(info->ptr, ":\r\n", &token_context);

    uo_move move = uo_position_parse_move(position, move_str);
    if (!move)
    {
      sprintf(info->message, "Error while parsing move '%s'", move_str);
      return false;
    }

    uo_square square_from = uo_move_square_from(move);
    uo_square square_to = uo_move_square_to(move);
    uo_move_type move_type_promo = uo_move_get_type(move) & uo_move_type__promo_Q;

    size_t node_count_expected;

    info->ptr = uo_strtok(NULL, "\r\n", &token_context);

    if (sscanf(info->ptr, " %zu", &node_count_expected) != 1)
    {
      sprintf(info->message, "Error while node count for move");
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
          sprintf(info->message, "generated node count %zu for move '%s' did not match expected count %zu for fen '%s'\n", node_count, move_str, node_count_expected, info->fen);
          //uo_position_print_diagram(position, buf);
          //sprintf(info->message, "\r\n%s", buf);
          //uo_position_print_fen(position, buf);
          //sprintf(info->message, "\nFen: %s\n", buf);
          return false;
        }

        uo_position_unmake_move(position);

        if (position->key != key)
        {
          sprintf(info->message, "position key '%" PRIu64 "' after unmake move '%s' did not match key '%" PRIu64 "' for fen '%s'\r\n", position->key, move_str, key, info->fen);
          //uo_position_print_diagram(position, buf);
          //sprintf(info->message, "\n%s", buf);
          //uo_position_print_fen(position, buf);
          //sprintf(info->message, "\nFen: %s\n", buf);
          return false;
        }

        position->movelist.head[i] = (uo_move){ 0 };
        goto next_move;
      }
    }

    sprintf(info->message, "move '%s' was not generated for fen '%s'", move_str, info->fen);
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
      sprintf(info->message, "move '%c%d%c%d' is not a legal move for fen '%s'",
        'a' + uo_square_file(square_from), 1 + uo_square_rank(square_from),
        'a' + uo_square_file(square_to), 1 + uo_square_rank(square_to),
        info->fen);

      return false;
    }
  }

  return true;
}

bool uo_test__go_depth(uo_test_info *info)
{
  size_t depth;

  if (sscanf(info->ptr, "go depth %zu", &depth) != 1 || depth < 1)
  {
    sprintf(info->message, "Expected to read 'go depth', instead read '%s'", info->ptr);
    return false;
  }

  // TODO: Run search to specified depth (perhaps using another process running the engine executable) and get the output

  while ((info->ptr = uo_file_mmap_readline(info->file_mmap)))
  {
    if (strlen(info->ptr) == 0) break;
    if (info->ptr[0] == '#') continue;

    int cp_lower_bound;
    int cp_upper_bound;

    if (sscanf(info->ptr, "cp [%d,%d]]", &cp_lower_bound, &cp_upper_bound) == 2)
    {
      // TODO: Test if evaluation is between boundaries

      
      continue;
    }

    char best_move_str[6];
    char ponder_move_str[6];

    if (sscanf(info->ptr, "bestmove %5s ponder %5s", best_move_str, ponder_move_str) == 2)
    {
      // TODO: Test if best move and ponder move is correct

      
      continue;
    }

    if (sscanf(info->ptr, "bestmove %5s", best_move_str) == 1)
    {
      // TODO: Test if best move correct

      
      continue;
    }


    sprintf(info->message, "Unknown command '%s'", info->ptr);
    return false;
  }

  return info->passed;
}


bool uo_test__go(uo_test_info *info)
{
  if (!test_command_map__go)
  {
    test_command_map__go = uo_strmap_create();

    // Register all 'go xxxx' test commands here
    uo_strmap_add(test_command_map__go, "perft", uo_test__go_perft);
    uo_strmap_add(test_command_map__go, "depth", uo_test__go_depth);

  }

  char *command_str = buf;
  if (sscanf(info->ptr, "go %s", command_str) != 1)
  {
    sprintf(info->message, "Unknown command '%s'", info->ptr);
    return false;
  }

  uo_test_command test_command = uo_strmap_get(test_command_map__go, command_str);

  if (!test_command)
  {
    sprintf(info->message, "Unknown command '%s'", command_str);
    return false;
  }

  return test_command(info);
}

bool uo_test(char *test_data_dir, char *test_name)
{
  if (!test_data_dir) return false;
  if (!test_name) return false;

  if (!test_command_map)
  {
    test_command_map = uo_strmap_create();

    // Register all test commands here
    uo_strmap_add(test_command_map, "wdl", uo_test__wdl);
    uo_strmap_add(test_command_map, "dtz", uo_test__dtz);
    uo_strmap_add(test_command_map, "go", uo_test__go);
    uo_strmap_add(test_command_map, "moveorder", uo_test__moveorder);

  }

  void *allocated_mem[3];
  size_t allocated_mem_count = 0;

  uo_test_info info = {
  .passed = true,
  .test_count = 0
  };

  char *filepath = uo_aprintf("%s/%s.txt", test_data_dir, test_name);
  allocated_mem[allocated_mem_count++] = filepath;

  info.test_name = uo_aprintf("%s", test_name);
  allocated_mem[allocated_mem_count++] = info.test_name;

  info.file_mmap = uo_file_mmap_open_read(filepath);

  if (!info.file_mmap)
  {
    printf("TEST '%s' FAILED: Cannot open file '%s'\r\n", info.test_name, filepath);
    uo_file_mmap_close(info.file_mmap);
    while (allocated_mem_count--) free(allocated_mem[allocated_mem_count]);
    return false;
  }

  info.ptr = uo_file_mmap_readline(info.file_mmap);

  if (!info.ptr)
  {
    printf("TEST '%s' FAILED: Error while reading test data.\r\n", info.test_name);
    uo_file_mmap_close(info.file_mmap);
    while (allocated_mem_count--) free(allocated_mem[allocated_mem_count]);
    return false;
  }

  while (info.ptr)
  {
    if (strlen(info.ptr) == 0)
    {
      // Empty lines are skipped
      info.ptr = uo_file_mmap_readline(info.file_mmap);
      continue;
    }

    if (info.ptr[0] == '#')
    {
      // Comment lines start with '#'
      info.ptr = uo_file_mmap_readline(info.file_mmap);
      continue;
    }

    if (sscanf(info.ptr, "position fen %s", buf) == 1)
    {
      info.ptr += sizeof("position fen ") - 1;
      if (!uo_position_from_fen(&info.position, info.ptr))
      {
        printf("TEST '%s' FAILED: Error while parsing fen '%s'\r\n", info.test_name, info.fen);
        uo_file_mmap_close(info.file_mmap);
        while (allocated_mem_count--) free(allocated_mem[allocated_mem_count]);
        return false;
      }

      uo_position_print_fen(&info.position, info.fen);
      assert(strcmp(info.ptr, info.fen) == 0);

      info.ptr = uo_file_mmap_readline(info.file_mmap);
      continue;
    }

    char *command_str = buf;

    if (sscanf(info.ptr, "%s", command_str) != 1)
    {
      printf("TEST '%s' FAILED: Error while parsing command '%s'\r\n", info.test_name, info.ptr);
      uo_file_mmap_close(info.file_mmap);
      while (allocated_mem_count--) free(allocated_mem[allocated_mem_count]);
      return false;
    }

    uo_test_command test_command = uo_strmap_get(test_command_map, command_str);

    if (!test_command)
    {
      printf("TEST '%s' FAILED: Unknown command '%s'\r\n", info.test_name, command_str);
      uo_file_mmap_close(info.file_mmap);
      while (allocated_mem_count--) free(allocated_mem[allocated_mem_count]);
      return false;
    }

    info.passed = test_command(&info);

    if (!info.passed)
    {
      if (*info.message)
      {
        printf("TEST '%s' FAILED: %s\r\n", info.test_name, info.message);
      }
      else
      {
        printf("TEST '%s' FAILED.\r\n", info.test_name);
      }

      uo_file_mmap_close(info.file_mmap);
      while (allocated_mem_count--) free(allocated_mem[allocated_mem_count]);
      return false;
    }

    ++info.test_count;
    info.ptr = uo_file_mmap_readline(info.file_mmap);
  }

  printf("TEST '%s' PASSED: Total of %zd test cases passed.\r\n", info.test_name, info.test_count);

  uo_file_mmap_close(info.file_mmap);
  while (allocated_mem_count--) free(allocated_mem[allocated_mem_count]);
  return true;
}
