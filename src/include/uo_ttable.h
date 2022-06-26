#ifndef UO_TTABLE_H
#define UO_TTABLE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_move.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

  typedef struct uo_tentry
  {
    uint64_t key;
    uo_move bestmove;
    uint8_t depth;
    int16_t score;
    /*
      0 = exact
      1 = lower bound
      2 = upper bound
      3 = mate
    */
    uint8_t type;
    bool keep;
  } uo_tentry;

#define uo_tentry_type__exact ((uint8_t)1) 
#define uo_tentry_type__lower_bound ((uint8_t)2) 
#define uo_tentry_type__upper_bound ((uint8_t)4) 

  typedef struct uo_ttable
  {
    uint64_t hash_mask;
    uo_tentry *entries;
  } uo_ttable;

  static inline void uo_ttable_init(uo_ttable *ttable, uint8_t hash_bits)
  {
    uint64_t capacity = (uint64_t)1 << (hash_bits - 1);
    ttable->entries = calloc(capacity, sizeof * ttable->entries);
    ttable->hash_mask = capacity - 1;
  }

  static inline void uo_ttable_free(uo_ttable *ttable)
  {
    free(ttable->entries);
  }

  static inline uo_tentry *uo_ttable_get(uo_ttable *ttable, uint64_t key)
  {
    uint64_t hash = key & ttable->hash_mask;
    uo_tentry *entry = ttable->entries + hash;

    while (true)
    {
      if (!entry->key)
      {
        entry->key = key;
        return entry;
      }
      else if (entry->key == key)
      {
        return entry;
      }
      else if (!entry->keep)
      {
        memset(entry - 1, 0, sizeof * entry);
        entry->key = key;
        return entry;
      }

      ++hash;
      hash = (hash + 1) & ttable->hash_mask;
      entry = ttable->entries + hash;
    }
  }

#ifdef __cplusplus
}
#endif

#endif
