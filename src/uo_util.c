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
  xoshiro256p(&state);
  xoshiro256p(&state);
}


#if !defined(uo_popcnt)
#include <limits.h>

uint8_t uo_popcnt(uint64_t u64)
{
  uint64_t v = u64;
  v = v - ((v >> 1) & (uint64_t)~(uint64_t)0 / 3);                                            // temp
  v = (v & (uint64_t)~(uint64_t)0 / 15 * 3) + ((v >> 2) & (uint64_t)~(uint64_t)0 / 15 * 3);   // temp
  v = (v + (v >> 4)) & (uint64_t)~(uint64_t)0 / 255 * 15;                                     // temp
  return (uint64_t)(v * ((uint64_t)~(uint64_t)0 / 255)) >> (sizeof(uint64_t) - 1) * CHAR_BIT; // count
}
#endif


typedef int (uo_cmp)(void *, const uint64_t, const uint64_t);

// see: https://en.wikipedia.org/wiki/Quicksort#Lomuto_partition_scheme
void uo_quicksort(uint64_t *arr, int lo, int hi, uo_cmp cmp, void *context)
{
  if (lo >= hi || lo < 0)
  {
    return;
  }

  int p = uo_partition(arr, lo, hi, cmp, context);

  uo_quicksort(arr, lo, p - 1, cmp, context);
  uo_quicksort(arr, p + 1, hi, cmp, context);
}

int uo_partition(uint64_t *arr, int lo, int hi, uo_cmp cmp, void *context)
{
  uint64_t pivot = arr[hi];
  uint64_t temp;

  int i = lo - 1;

  for (int j = lo; i < hi - 1; ++j)
  {
    if (cmp(arr + j, pivot, cmp, context) < 1)
    {
      ++i;

      temp = arr[i];
      arr[i] = arr[j];
      arr[j] = temp;
    }
  }

  i = i + 1;
  temp = arr[i];
  arr[i] = arr[hi];
  arr[hi] = temp;
  return i;
}
