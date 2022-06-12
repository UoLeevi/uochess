#include "uo_util.h"

static inline uint64_t rol64(uint64_t x, int k)
{
  return (x << k) | (x >> (64 - k));
}

struct xoshiro256p_state {
  uint64_t s[4];
};

static struct xoshiro256p_state state;

// https://en.wikipedia.org/wiki/Xorshift#xoshiro256+
static inline uint64_t xoshiro256p(struct xoshiro256p_state *state)
{
  uint64_t *s = state->s;
  uint64_t const result = s[0] + s[3];
  uint64_t const t = s[1] << 17;

  s[2] ^= s[0];
  s[3] ^= s[1];
  s[1] ^= s[2];
  s[0] ^= s[3];

  s[2] ^= t;
  s[3] = rol64(s[3], 45);

  return result;
}

uint64_t uo_rand()
{
  return xoshiro256p(&state);
}

void uo_rand_init(uint64_t seed)
{
  state.s[0] = seed;
  xoshiro256p(&state);
}
