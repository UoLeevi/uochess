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
    uint8_t priority;
    bool keep;
  } uo_tentry;

#define uo_tentry_type__exact ((uint8_t)1)
#define uo_tentry_type__lower_bound ((uint8_t)2)
#define uo_tentry_type__upper_bound ((uint8_t)4)

#define uo_tentry_initial_priority ((uint8_t)2)

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
    uint64_t mask = ttable->hash_mask;
    uint64_t hash = key & mask;
    uint64_t i = hash;
    uo_tentry *entry = ttable->entries + i;

    // 1. Look for matching key and stop on first empty key

    while (entry->key && entry->key != key)
    {
      i = (i + 1) & mask;
      entry = ttable->entries + i;
    }

    // 2. If exact match was found, increment priority and return

    if (entry->key)
    {
      ++entry->priority;
      return entry;
    }

    // 3. No exact match was found, replace first item with priority 0

    i = hash;
    entry = ttable->entries + i;

    while (entry->key && (entry->keep || --entry->priority))
    {
      i = (i + 1) & mask;
      entry = ttable->entries + i;
    }

    if (entry->key)
    {
      memset(entry, 0, sizeof * entry);
    }

    entry->key = key;
    entry->priority = uo_tentry_initial_priority;

    return entry;
  }

  // If used, must be called before `uo_ttable_get` is called for the next time
  static inline void uo_tentry_release_if_unused(uo_tentry *entry)
  {
    if (!entry->type)
    {
      entry->key = 0;
      entry->priority = 0;
    }
  }

#ifdef __cplusplus
}
#endif

#endif
