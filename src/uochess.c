#include "uo_err.h"
#include "uo_cb.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef unsigned long long u64;


int printhex(char *str)
{
  while (*str)
  {
    printf("%X ", *str++);
  }

  printf("\n");
}

int printbitboard(u64 bitboard)
{
  for (int i = 0; i < 64; ++i)
  {
    printf((1ull << i) & bitboard ? "X" : ".");

    if (i % 8 == 7)
    {
      printf("\n");
    }
  }

  printf("\n");
}

char buf[1028];

typedef struct position
{
  u64 black_piece;

  u64 p;
  u64 n;
  u64 b;
  u64 r;
  u64 q;
  u64 k;

  bool white_to_move;

  // K Q k q
  // 1 2 4 8
  unsigned char castling;

  unsigned char enpassant;
  unsigned char halfmoves;
  unsigned int fullmove;

} position;

// see: https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation
position *position_from_fen(position *pos, char *fen)
{
  // clear position
  memset(pos, 0, sizeof *pos);

  // loop by rank
  for (int i = 0; i < 8; ++i)
  {
    // loop by file
    for (int j = 0; j < 8; ++j)
    {
      char c = *fen++;
      if (c == '\0')
      {
        return NULL;
      }

      // empty squares
      if (c > '0' && c <= ('8' - j))
      {
        j += c - '0' - 1;
        continue;
      }

      u64 *bitboard;
      bool black_piece;

      switch (c)
      {
      case 'P':
        bitboard = &pos->p;
        black_piece = false;
        break;
      case 'p':
        bitboard = &pos->p;
        black_piece = true;
        break;
      case 'N':
        bitboard = &pos->n;
        black_piece = false;
        break;
      case 'n':
        bitboard = &pos->n;
        black_piece = true;
        break;
      case 'B':
        bitboard = &pos->b;
        black_piece = false;
        break;
      case 'b':
        bitboard = &pos->b;
        black_piece = true;
        break;
      case 'R':
        bitboard = &pos->r;
        black_piece = false;
        break;
      case 'r':
        bitboard = &pos->r;
        black_piece = true;
        break;
      case 'Q':
        bitboard = &pos->q;
        black_piece = false;
        break;
      case 'q':
        bitboard = &pos->q;
        black_piece = true;
        break;
      case 'K':
        bitboard = &pos->k;
        black_piece = false;
        break;
      case 'k':
        bitboard = &pos->k;
        black_piece = true;
        break;

      default:
        return NULL;
      }

      u64 mask = 1ull << (i * 8 + j);

      *bitboard |= mask;

      if (black_piece)
      {
        pos->black_piece |= mask;
      }
    }

    if (i < 7 && *fen++ != '/')
    {
      return NULL;
    }
  }

  return pos;
}

char *position_to_diagram(position *pos, char diagram[72])
{
  char *ptr = diagram;

  for (int i = 0; i < 64; ++i)
  {
    u64 mask = 1ull << i;

    char piece;

    if (pos->p & mask)
    {
      piece = 'P';
    }
    else if (pos->n & mask)
    {
      piece = 'N';
    }
    else if (pos->b & mask)
    {
      piece = 'B';
    }
    else if (pos->r & mask)
    {
      piece = 'R';
    }
    else if (pos->q & mask)
    {
      piece = 'Q';
    }
    else if (pos->k & mask)
    {
      piece = 'K';
    }
    else
    {
      piece = '.';
    }

    if (piece != '.' && (pos->black_piece & mask))
    {
      piece += 0x20;
    }

    *ptr++ = piece;

    if (i % 8 == 7)
    {
      *ptr++ = '\n';
    }
  }

  *ptr++ = '\0';

  return diagram;
}

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

      position pos;
      position_from_fen(&pos, ptr);
      position_to_diagram(&pos, buf);
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