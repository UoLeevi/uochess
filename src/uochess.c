#include "uo_bitboard.h"
#include "uo_position.h"
#include "uo_moves.h"
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

#define UO_SEARCHTREE_INITIAL_CAPACITY 0x10000;


typedef struct uo_searchtree
{
  uo_position *root;
  uo_position *head;
  size_t capacity;
} uo_searchtree;

uo_searchtree searchtree;

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
  searchtree.capacity = UO_SEARCHTREE_INITIAL_CAPACITY;
  searchtree.root = malloc(searchtree.capacity * sizeof(*searchtree.root));
  searchtree.head = searchtree.root;

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
      char *ptr = strtok(NULL, "\n ");

      if (ptr)
      {
        --ptr;
        *ptr = 'm';
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

        uo_position *node = searchtree.head;

        uo_bitboard moves = uo_position_moves(&pos, square_from, node);

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
        uo_square square = -1;

        while (uo_bitboard_next_square(uo_bitboard_all, &square))
        {
          uo_bitboard moves = uo_position_moves(&pos, square, searchtree.head);
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
