#include "uo_bitboard.h"
#include "uo_position.h"
#include "uo_util.h"

#include <stdlib.h>
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

#define UO_MAX_PLY 128
#define UO_BRANCING_FACTOR 35

struct
{
  uo_position position;
  uo_move candidates[0x100];
  uo_move_ex movestack[UO_MAX_PLY * UO_BRANCING_FACTOR];
  uo_move_ex *head;
} search;

enum state
{
  INIT,
  OPTIONS,
  READY,
  POSITION
} state = INIT;

static void engine_init(void)
{
  uo_bitboard_init();
  search.head = &search.movestack;
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
      uo_position *ret = uo_position_from_fen(&search.position, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
      state = POSITION;
      //printf("position: %p\n", ret);
      //uo_position_to_diagram(&search.position, buf);
      //printf("diagram:\n%s", buf);

      ptr = strtok(NULL, "\n ");
    }
    else if (ptr && strcmp(ptr, "fen") == 0)
    {
      ptr = strtok(NULL, "\n");

      char *fen = strtok(ptr, "m"); /* moves */
      char *ptr = strtok(NULL, "\n ");

      if (ptr)
      {
        --ptr;
        *ptr = 'm';
      }

      uo_position *ret = uo_position_from_fen(&search.position, fen);
      state = POSITION;
      //printf("position: %p\n", ret);
      //uo_position_to_diagram(&search.position, buf);
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

        int nodes_count = uo_position_get_moves(&search.position, search.candidates);

        for (int i = 0; i < nodes_count; ++i)
        {
          uo_move move = search.candidates[i];
          if (square_from == uo_move_square_from(move)
            && square_to == uo_move_square_to(move))
          {
            *search.head++ = uo_position_make_move(&search.position, move);
            goto next_move;
          }
        }

        // Not a legal move
        return;

        next_move:;
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
        int nodes_count = uo_position_get_moves(&search.position, search.candidates);

        for (int i = 0; i < nodes_count; ++i)
        {
          uo_move move = search.candidates[i];
          uo_square square_from = uo_move_square_from(move);
          uo_square square_to = uo_move_square_to(move);

          printf("%c%d%c%d: 1\n",
            'a' + uo_square_file(square_from), 1 + uo_square_rank(square_from),
            'a' + uo_square_file(square_to), 1 + uo_square_rank(square_to));
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

int main(
  int argc,
  char **argv)
{
  printf("Uochess 0.1 by Leevi Uotinen\n");
  engine_init();

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
