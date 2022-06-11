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

enum state
{
  INIT,
  OPTIONS,
  READY,
  POSITION
} state = INIT;

uo_position pos;

static void process_cmd__init(void)
{
  char *ptr = buf;

  if (strcmp(ptr, "uci\n") == 0)
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
  if (strcmp(ptr, "isready\n") == 0)
  {
    printf("readyok\n");
    state = READY;
  }
}

static void process_cmd__ready(void)
{
  char *ptr = buf;

  if (strncmp(ptr, "position ", sizeof "position " - 1) == 0)
  {
    ptr += sizeof "position " - 1;

    if (strncmp(ptr, "startpos", sizeof "startpos" - 1) == 0)
    {
      ptr += sizeof "startpos" - 1;

      uo_position *ret = uo_position_from_fen(&pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
      state = POSITION;
      //printf("pos: %p\n", ret);
      //uo_position_to_diagram(&pos, buf);
      //printf("diagram:\n%s", buf);
    }

    if (strncmp(ptr, "fen ", sizeof "fen " - 1) == 0)
    {
      ptr += sizeof "fen " - 1;

      uo_position *ret = uo_position_from_fen(&pos, ptr);
      state = POSITION;
      //printf("pos: %p\n", ret);
      //uo_position_to_diagram(&pos, buf);
      //printf("diagram:\n%s", buf);
    }
  }
}

static void process_cmd__position(void)
{
  char *ptr = buf;

  if (strncmp(ptr, "go ", sizeof "go " - 1) == 0)
  {
    ptr += sizeof "go " - 1;

    if (strncmp(ptr, "perft ", sizeof "perft " - 1) == 0)
    {
      ptr += sizeof "perft " - 1;

      int depth;
      if (sscanf(ptr, "%d", &depth) == 1) 
      {
        uo_position buffer[27] = {0}; 
        
        for (uo_square square = 0; square < 64; ++square)
        {
          uo_bitboard moves = uo_position_moves(&pos, square, buffer);
          uint16_t move_count = uo_popcount(moves);

          uo_square move = 0;

          for (uint16_t i = 0, j = 0; i < move_count; ++i)
          {
            while (moves & ~uo_square_bitboard(move))
            {
              ++move;
            }

            printf("%c%d%c%d: 1\n", 
                'a' + uo_square_file(square), 1 + uo_square_rank(square),
                'a' + uo_square_file(move), 1 + uo_square_rank(move)); 

            ++move;
          }         
        }

        int nodes = 0;
        printf("\nNodes searched: %d\n", nodes);      
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
  uo_bitboard_init();
  uo_moves_init();

  // Use command line argument `test` to perform tests
  if (argc > 1 && strcmp(argv[1], "test") == 0)
  {
    uo_run_test(moves);

    printf("Tests passed.\n");
    return 0;
  }

  while (fgets(buf, sizeof buf, stdin))
  {
    if (strcmp(buf, "quit\n") == 0)
    {
      return 0;
    }

    process_cmd[state]();
  }

  return 0;
}