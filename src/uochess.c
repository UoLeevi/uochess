#include "uo_macro.h"
#include "uo_bitboard.h"
#include "uo_position.h"
#include "uo_moves.h"

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
  READY
} state = INIT;

void process_cmd__init()
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

void process_cmd__options()
{
  char *ptr = buf;
  if (strcmp(ptr, "isready\n") == 0)
  {
    printf("readyok\n");
    state = READY;
  }
}

void process_cmd__ready()
{
  char *ptr = buf;

  if (strncmp(ptr, "position ", sizeof "position " - 1) == 0)
  {
    ptr += sizeof "position " - 1;

    if (strncmp(ptr, "fen ", sizeof "fen " - 1) == 0)
    {
      ptr += sizeof "fen " - 1;

      uo_position pos;
      uo_position *ret = uo_position_from_fen(&pos, ptr);
      printf("pos: %p\n", ret);
      uo_position_to_diagram(&pos, buf);
      printf("diagram:\n%s", buf);
    }
  }
}

void (*process_cmd[])() = {
    process_cmd__init,
    process_cmd__options,
    process_cmd__ready};

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