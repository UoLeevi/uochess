#ifndef UO_TTABLE_H
#define UO_TTABLE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_move.h"
#include "uo_thread.h"
#include "uo_search.h"
#include "uo_position.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

  typedef struct uo_tentry
  {
    uint32_t key;
    uo_move bestmove;
    int16_t value;
    uint8_t depth;
    uint8_t type;
    uint8_t refcount;
    uint8_t expiry_ply;
  } uo_tentry;

#define uo_tentry_type__exact ((uint8_t)1)
#define uo_tentry_type__upper_bound ((uint8_t)2)
#define uo_tentry_type__lower_bound ((uint8_t)4)

#define uo_ttable_max_probe 3
#define uo_ttable_expiry_ply 1

  typedef struct uo_ttable
  {
    uint64_t hash_mask;
    uint64_t count;
    uo_tentry *entries;
    volatile uo_atomic_int busy;
  } uo_ttable;

  static inline void uo_ttable_init(uo_ttable *ttable, uint8_t hash_bits)
  {
    uint64_t capacity = (uint64_t)1 << (hash_bits - 1);
    ttable->entries = calloc(capacity, sizeof * ttable->entries);
    ttable->hash_mask = capacity - 1;
    ttable->count = 0;
    uo_atomic_init(&ttable->busy, 0);
  }

  static inline void uo_ttable_clear(uo_ttable *ttable)
  {
    memset(ttable->entries, 0, (ttable->hash_mask + 1) * sizeof * ttable->entries);
    ttable->count = 0;
  }

  static inline void uo_ttable_free(uo_ttable *ttable)
  {
    free(ttable->entries);
  }

  static inline void uo_ttable_lock(uo_ttable *ttable)
  {
    uo_atomic_compare_exchange_wait(&ttable->busy, 0, 1);
  }

  static inline void uo_ttable_unlock(uo_ttable *ttable)
  {
    uo_atomic_store(&ttable->busy, 0);
  }

  static inline uo_tentry *uo_ttable_get(uo_ttable *ttable, const uo_position *position)
  {
    uint64_t mask = ttable->hash_mask;
    uint64_t hash = position->key & mask;
    uint32_t key = (uint32_t)position->key;
    uint64_t i = hash;
    uo_tentry *entry = ttable->entries + i;

    // Look for matching key and stop on first empty key

    while (entry->key && entry->key != key)
    {
      i = (i + 1) & mask;
      entry = ttable->entries + i;
    }

    // 2. If exact match was found, return entry

    if (entry->key)
    {
      return entry;
    }

    // 3. Loop backwards and remove consecutive expired entries
    uint8_t root_ply = position->root_ply;

    i = (i - 1) & mask;
    entry = ttable->entries + i;

    while (entry->key && !entry->refcount && entry->expiry_ply < root_ply)
    {
      memset(entry, 0, sizeof * entry);
      --ttable->count;

      i = (i - 1) & mask;
      entry = ttable->entries + i;
    }

    // 4. If load factor is above 75 %, remove couple extra entries

    if (ttable->count > ((mask + 1) * 3) >> 2)
    {
      while (entry->key && !entry->refcount)
      {
        memset(entry, 0, sizeof * entry);
        --ttable->count;

        i = (i - 1) & mask;
        entry = ttable->entries + i;
      }
    }

    return NULL;
  }

  static inline uo_tentry *uo_ttable_set(uo_ttable *ttable, const uo_position *position)
  {
    uint64_t mask = ttable->hash_mask;
    uint64_t hash = position->key & mask;
    uint32_t key = (uint32_t)position->key;
    uint64_t i = hash;
    uo_tentry *entry = ttable->entries + i;

    // 1. Look for matching key and stop on first empty key

    while (entry->key && entry->key != key)
    {
      i = (i + 1) & mask;
      entry = ttable->entries + i;
    }

    // 2. If exact match was found, return exisiting entry

    if (entry->key)
    {
      return entry;
    }

    // 3. No exact match was found, return first vacant slot or replace first expired entry or else replace `uo_ttable_max_probe` th entry

    uint8_t root_ply = position->root_ply;

    i = hash;
    entry = ttable->entries + i;

    for (size_t j = 0; j < uo_ttable_max_probe; ++j)
    {
      if (!entry->key)
      {
        entry->key = key;
        entry->expiry_ply = root_ply + uo_ttable_expiry_ply;
        ++ttable->count;
        return entry;
      }

      if (!entry->refcount && entry->expiry_ply < root_ply)
      {
        memset(entry, 0, sizeof * entry);
        entry->key = key;
        entry->expiry_ply = root_ply + uo_ttable_expiry_ply;
        return entry;
      }

      i = (i + 1) & mask;
      entry = ttable->entries + i;
    }

    while (entry->key && entry->refcount)
    {
      i = (i + 1) & mask;
      entry = ttable->entries + i;
    }

    if (!entry->key)
    {
      entry->key = key;
      entry->expiry_ply = root_ply + uo_ttable_expiry_ply;
      ++ttable->count;
      return entry;
    }

    memset(entry, 0, sizeof * entry);
    entry->key = key;
    entry->expiry_ply = root_ply + uo_ttable_expiry_ply;

    return entry;
  }

#ifdef __cplusplus
}
#endif

#endif
