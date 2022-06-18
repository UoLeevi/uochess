#include "uo_test.h"
#include "uo_global.h"
#include "uo_position.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

bool uo_test_move_generation(char *test_data_dir)
{
  if (!test_data_dir) return false;

  bool passed = true;

  char *filepath = buf;

  strcpy(filepath, test_data_dir);
  strcpy(filepath + strlen(test_data_dir), "/move_generation.txt");

  FILE *fp = fopen(filepath, "r");
  if (!fp)
  {
    printf("Cannot open file '%s'", filepath);
    return false;
  }

  size_t moves_count;
  uo_move moves[0x100];
  uo_position position;

  while (fgets(buf, sizeof buf, fp))
  {
    char *ptr = buf;
    ptr = strtok(ptr, " ");

    if (!ptr || strcmp(ptr, "position"))
    {
      printf("Expected to read 'position', instead read '%s'", ptr);
      fclose(fp);
      return false;
    }

    ptr = strtok(NULL, " ");

    if (!ptr || strcmp(ptr, "fen"))
    {
      printf("Expected to read 'fen', instead read '%s'", ptr);
      fclose(fp);
      return false;
    }

    char *fen = strtok(NULL, "\n");
    ptr = fen + strlen(fen) + 1;

    if (!uo_position_from_fen(&position, fen))
    {
      printf("Error while parsing fen '%s'", fen);
      fclose(fp);
      return false;
    }

    moves_count = uo_position_get_moves(&position, moves);

    while (fgets(ptr, ptr - buf, fp))
    {
      ptr = strtok(ptr, "\n");

      if (!ptr || strlen(ptr) == 0) break;

      uo_move move = uo_move_parse(ptr);
      if (!move)
      {
        printf("Error while parsing move '%s'", ptr);
        fclose(fp);
        return false;
      }

      uo_square square_from = uo_move_square_from(move);
      uo_square square_to = uo_move_square_to(move);
      uo_move_type move_type_promo = uo_move_get_type(move) & uo_move_type__promo_Q;

      for (int i = 0; i < moves_count; ++i)
      {
        uo_move move = moves[i];
        if (uo_move_square_from(move) == square_from && uo_move_square_to(move) == square_to)
        {
          if (move_type_promo & uo_move_type__promo && (move_type_promo != (uo_move_get_type(move) & uo_move_type__promo_Q)))
          {
            continue;
          }

          // move found
          moves[i] = 0;
          goto next_move;
        }
      }

      printf("TEST 'move_generation' FAILED: move '%s' was not generated for fen '%s'", ptr, fen);
      fclose(fp);
      return false;

    next_move:;
    }

    for (int i = 0; i < moves_count; ++i)
    {
      uo_move move = moves[i];
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

  }

  fclose(fp);

  return passed;
}
