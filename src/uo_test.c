#include "uo_test.h"
#include "uo_global.h"
#include "uo_position.h"
#include "uo_search.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

bool uo_test_move_generation(uo_search *search, char *test_data_dir)
{
  if (!test_data_dir) return false;

  bool passed = true;

  size_t test_count = 0;

  char *filepath = buf;

  strcpy(filepath, test_data_dir);
  strcpy(filepath + strlen(test_data_dir), "/move_generation.txt");

  FILE *fp = fopen(filepath, "r");
  if (!fp)
  {
    printf("Cannot open file '%s'", filepath);
    return false;
  }

  size_t move_count;

  while (fgets(buf, sizeof buf, fp))
  {
    char *ptr = buf;
    ptr = strtok(ptr, "\n ");

    if (!ptr)
    {
      printf("Error while reading test data\n");
      fclose(fp);
      return false;
    }
    else if (strcmp(ptr, "ucinewgame") == 0)
    {
      if (!fgets(buf, sizeof buf, fp))
      {
        printf("Unexpected EOF while reading test data\n");
        fclose(fp);
        return false;
      }

      ptr = strtok(ptr, " ");
    }

    if (!ptr || strcmp(ptr, "position"))
    {
      printf("Expected to read 'position', instead read '%s'\n", ptr);
      fclose(fp);
      return false;
    }

    ptr = strtok(NULL, " ");

    if (!ptr || strcmp(ptr, "fen"))
    {
      printf("Expected to read 'fen', instead read '%s'\n", ptr);
      fclose(fp);
      return false;
    }

    char *fen = strtok(NULL, "\n");
    ptr = fen + strlen(fen) + 1;

    if (!uo_position_from_fen(&search->position, fen))
    {
      printf("Error while parsing fen '%s'\n", fen);
      fclose(fp);
      return false;
    }

    if (!fgets(ptr, ptr - buf, fp))
    {
      printf("Unexpected EOF while reading test data\n");
      fclose(fp);
      return false;
    }

    ptr = strtok(ptr, "\n");
    size_t depth;

    if (!ptr || sscanf(ptr, "go perft %zu", &depth) != 1 || depth < 1)
    {
      printf("Expected to read 'go perft', instead read '%s'\n", ptr);
      fclose(fp);
      return false;
    }

    move_count = uo_position_get_moves(&search->position, search->head);
    search->head += move_count;

    while (fgets(ptr, ptr - buf, fp))
    {
      char *move_str = strtok(ptr, ":\n");

      if (!move_str || strlen(move_str) == 0) break;

      uo_move move = uo_position_parse_move(&search->position, move_str);
      if (!move)
      {
        printf("Error while parsing move '%s'", move_str);
        fclose(fp);
        return false;
      }

      uo_square square_from = uo_move_square_from(move);
      uo_square square_to = uo_move_square_to(move);
      uo_move_type move_type_promo = uo_move_get_type(move) & uo_move_type__promo_Q;

      size_t node_count_expected;

      ptr = strtok(NULL, "\n");

      if (sscanf(ptr, " %zu", &node_count_expected) != 1)
      {
        printf("Error while node count for move\n");
        fclose(fp);
        return false;
      }

      for (int64_t i = 0; i < move_count; ++i)
      {
        uo_move move = search->head[i - move_count];
        if (uo_move_square_from(move) == square_from && uo_move_square_to(move) == square_to)
        {
          if (move_type_promo & uo_move_type__promo && (move_type_promo != (uo_move_get_type(move) & uo_move_type__promo_Q)))
          {
            continue;
          }

          // move found

          uo_move_ex move_ex = uo_position_make_move(&search->position, move);
          size_t node_count = depth == 1 ? 1 : uo_search_perft(search, depth - 1);

          if (node_count != node_count_expected)
          {
            printf("TEST 'move_generation' FAILED: generated node count %zu for move '%s' did not match expected count %zu for fen '%s'\n", node_count, move_str, node_count_expected, fen);
            uo_position_print_diagram(&search->position, buf);
            printf("\n%s", buf);
            uo_position_print_fen(&search->position, buf);
            printf("\nFen: %s\n", buf);

            fclose(fp);
            return false;
          }

          uo_position_unmake_move(&search->position, move_ex);

          search->head[i - move_count] = 0;
          goto next_move;
        }
      }

      printf("TEST 'move_generation' FAILED: move '%s' was not generated for fen '%s'", move_str, fen);
      fclose(fp);
      return false;

    next_move:;
    }

    for (int64_t i = 0; i < move_count; ++i)
    {
      uo_move move = search->head[i - move_count];
      if (move)
      {
        uo_square square_from = uo_move_square_from(move);
        uo_square square_to = uo_move_square_to(move);
        printf("TEST 'move_generation' FAILED: move '%c%d%c%d' is not a legal move for fen '%s'",
          'a' + uo_square_file(square_from), 1 + uo_square_rank(square_from),
          'a' + uo_square_file(square_to), 1 + uo_square_rank(square_to),
          fen);

        fclose(fp);
        return false;
      }
    }

    search->head -= move_count;
    ++test_count;
  }

  fclose(fp);

  if (passed)
  {
    printf("TEST 'move_generation' PASSED: total of %zd positions tested.", test_count);
  }

  return passed;
}
