#include "uo_zobrist.h"
#include "uo_piece.h"
#include "uo_util.h"

#include <inttypes.h>
#include <stdbool.h>

uint64_t uo_zobrist[0xEull << 6];
uint64_t *uo_zobrist_castling;
uint64_t *uo_zobrist_enpassant_file;

static bool init;
static uint64_t rand_seed = 7109;

void uo_zobrist_init()
{
  if (init)
    return;
  init = true;

  uo_rand_init(rand_seed);

  for (size_t i = 0; i < 0xEull << 6; ++i)
  {
    uo_zobrist[i] = uo_rand_u64();
  }

  uo_zobrist_enpassant_file = uo_zobrist;
  uo_zobrist_castling = uo_zobrist + 16;
}

