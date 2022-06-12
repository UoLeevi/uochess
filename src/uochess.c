#include "uo_macro.h"
#include "uo_bitboard.h"
#include "uo_position.h"
#include "uo_moves.h"
#include "uo_util.h"

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

enum state
{
  INIT,
  OPTIONS,
  READY,
  POSITION
} state = INIT;

uo_position pos;

static void engine_init(void)
{
  uo_bitboard_init();
  uo_moves_init();
}

static void process_cmd__init(void)
{
  if (ptr && strcmp(ptr, "uci") == 0)
  {
    printf("id name uochess\n");
    printf("id author Leevi Uotinen\n");
    printf("uciok\n");
    state = OPTIONS;
  }
}

static void process_cmd__options(void)
{
  char *ptr = buf;
  if (ptr && strcmp(ptr, "isready") == 0)
  {
    printf("readyok\n");
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
      uo_position *ret = uo_position_from_fen(&pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
      state = POSITION;
      //printf("pos: %p\n", ret);
      //uo_position_to_diagram(&pos, buf);
      //printf("diagram:\n%s", buf);

      ptr = strtok(NULL, "\n ");
    }
    else if (ptr && strcmp(ptr, "fen") == 0)
    {
      ptr = strtok(NULL, "\n");

      char *fen = strtok(ptr, "m"); /* moves */

      if (fen)
      {
        ptr = strtok(NULL, "\n ");
        --ptr;
        *ptr = 'm';
      }
      else
      {
        fen = strtok(ptr, "\n");
        ptr = strtok(NULL, "\n ");
      }


      uo_position *ret = uo_position_from_fen(&pos, fen);
      state = POSITION;
      //printf("pos: %p\n", ret);
      //uo_position_to_diagram(&pos, buf);
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

        uo_position nodes[27] = { 0 };
        uo_position *node = nodes;

        uo_bitboard moves = uo_position_moves(&pos, square_from, nodes);

        if (!(moves & uo_square_bitboard(square_to)))
        {
          // not a legal move
          return;
        }

        uo_square i = -1;

        while (uo_bitboard_next_square(moves, &i) && i != square_to)
        {
          ++node;
        }

        pos = *node;
      }
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

  if (ptr && strcmp(ptr, "go") == 0)
  {
    ptr = strtok(NULL, "\n ");

    if (ptr && strcmp(ptr, "perft") == 0)
    {
      ptr = strtok(NULL, "\n ");

      int depth;
      if (sscanf(ptr, "%d", &depth) == 1)
      {
        int nodes_count = 0;
        uo_position nodes[27] = { 0 };
        uo_square square = -1;

        while (uo_bitboard_next_square(uo_bitboard_all, &square))
        {
          uo_bitboard moves = uo_position_moves(&pos, square, nodes);
          uo_square i = -1;

          while (uo_bitboard_next_square(moves, &i))
          {
            printf("%c%d%c%d: 1\n",
              'a' + uo_square_file(square), 1 + uo_square_rank(square),
              'a' + uo_square_file(i), 1 + uo_square_rank(i));

            ++nodes_count;
          }
        }

        printf("\nNodes searched: %d\n\n", nodes_count);
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

#define uo_run_test(test_name)                                  \
if (argc == 2 || strcmp(argv[1], uo_nameof(test_name)) == 0)    \
{                                                               \
  if (!uo_cat(uo_test_, test_name)())                           \
  {                                                             \
    printf("test `%s` failed.\n", uo_nameof(test_name));        \
    return 1;                                                   \
  }                                                             \
}

int main(
  int argc,
  char **argv)
{
  printf("Uochess 0.1 by Leevi Uotinen\n");
  engine_init();

  // Use command line argument `test` to perform tests
  if (argc > 1 && strcmp(argv[1], "test") == 0)
  {
    uo_run_test(moves);

    printf("Tests passed.\n");
    return 0;
  }

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
