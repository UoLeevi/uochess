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
    int16_t score;
    uint8_t depth;
    uint8_t type;
    int8_t priority;
    bool keep;
  } uo_tentry;

#define uo_tentry_type__exact ((uint8_t)1)
#define uo_tentry_type__lower_bound ((uint8_t)2)
#define uo_tentry_type__upper_bound ((uint8_t)4)

  typedef struct uo_ttable
  {
    uint64_t hash_mask;
    uint64_t count;
    int8_t priority_initial;
    uo_tentry *entries;
  } uo_ttable;

  static inline void uo_ttable_init(uo_ttable *ttable, uint8_t hash_bits)
  {
    uint64_t capacity = (uint64_t)1 << (hash_bits - 1);
    ttable->entries = calloc(capacity, sizeof * ttable->entries);
    ttable->hash_mask = capacity - 1;
    ttable->count = 0;
    ttable->priority_initial = hash_bits >> 1;
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

    // 3. No match was found, remove first item with priority 0

    int8_t priority_decrement = uo_msb(ttable->count + 1) >> 2;

    while (i && --i >= hash)
    {
      entry = ttable->entries + i;

      if (!entry->keep && (entry->priority -= priority_decrement) <= 0)
      {
        break;
      }
    }

    if (entry->key)
    {
      memset(entry, 0, sizeof * entry);
      --ttable->count;
    }

    return NULL;
  }

  static inline uo_tentry *uo_ttable_set(uo_ttable *ttable, uint64_t key)
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

    int8_t priority_decrement = uo_msb(ttable->count + 1);

    while (entry->key && entry->keep || (entry->priority -= priority_decrement) > 0)
    {
      i = (i + 1) & mask;
      entry = ttable->entries + i;
    }

    if (entry->key)
    {
      memset(entry, 0, sizeof * entry);
      --ttable->count;
    }

    entry->key = key;
    entry->priority = ttable->priority_initial;
    ++ttable->count;

    return entry;
  }

#ifdef __cplusplus
}
#endif

#endif
