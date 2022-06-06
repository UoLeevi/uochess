#include "uo_err.h"
#include "uo_cb.h"
#include "uo_bitboard.h"
#include "uo_position.h"

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

int main(
    int argc,
    char **argv)
{
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