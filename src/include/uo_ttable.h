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

  typedef union uo_tdata
  {
    uint64_t data;
    struct
    {
      uo_move bestmove;
      int16_t value;
      uint8_t depth;
      uint8_t type;
    };
  } uo_tdata;

  typedef struct uo_tentry
  {
    uint32_t key;
    union
    {
      uint64_t data;
      struct
      {
        uo_move bestmove;
        int16_t value;
        uint8_t depth;
        uint8_t type;
        uint8_t expiry_ply;
      };
    };
  } uo_tentry;

#define uo_tentry_type__exact ((uint8_t)0)
#define uo_tentry_type__upper_bound ((uint8_t)1)
#define uo_tentry_type__lower_bound ((uint8_t)2)

#define uo_ttable_max_probe 3
#define uo_ttable_expiry_ply 2

  typedef struct uo_ttable
  {
    uint64_t hash_mask;
    uo_atomic_int count;
    uo_tentry *entries;
    //volatile uo_atomic_int busy;
  } uo_ttable;

  static inline void uo_ttable_init(uo_ttable *ttable, uint8_t hash_bits)
  {
    uint64_t capacity = (uint64_t)1 << (hash_bits - 1);
    ttable->entries = calloc(capacity, sizeof * ttable->entries);
    ttable->hash_mask = capacity - 1;
    uo_atomic_init(&ttable->count, 0);
    //uo_atomic_init(&ttable->busy, 0);
  }

  static inline void uo_ttable_clear(uo_ttable *ttable)
  {
    memset(ttable->entries, 0, (ttable->hash_mask + 1) * sizeof * ttable->entries);
    uo_atomic_store(&ttable->count, 0);
  }

  static inline void uo_ttable_free(uo_ttable *ttable)
  {
    free(ttable->entries);
  }

  //static inline void uo_ttable_lock(uo_ttable *ttable)
  //{
  //  uo_atomic_compare_exchange_wait(&ttable->busy, 0, 1);
  //}

  //static inline void uo_ttable_unlock(uo_ttable *ttable)
  //{
  //  uo_atomic_store(&ttable->busy, 0);
  //}

  static inline bool uo_tentry_test_key(const uo_tentry *entry, uint32_t key)
  {
    return (entry->key ^ (uint32_t)entry->data) == key;
  }

  static inline uint32_t uo_tentry_create_key(const uo_tentry *entry, uint32_t key)
  {
    return key ^ (uint32_t)entry->data;
  }

  static inline bool uo_ttable_get(uo_ttable *ttable, const uo_position *position, uo_tdata *data)
  {
    uint64_t mask = ttable->hash_mask;
    uint64_t hash = position->key & mask;
    uint32_t key = (uint32_t)position->key;
    uint64_t i = hash;
    uo_tentry *entry = ttable->entries + i;

    // 1. Look for matching key or stop on first empty key

    while (entry->key && !uo_tentry_test_key(entry, key))
    {
      i = (i + 1) & mask;
      entry = ttable->entries + i;
    }

    // 2. If exact match was found, return entry

    if (entry->key)
    {
      data->data = entry->data;
      return true;
    }

    // 3. Loop backwards and remove consecutive expired entries
    uint8_t root_ply = position->root_ply;

    i = (i - 1) & mask;
    entry = ttable->entries + i;

    while (entry->key && /*!entry->thread_id &&*/ entry->expiry_ply < root_ply)
    {
      entry->key = 0;
      entry->data = 0;
      uo_atomic_decrement(&ttable->count);

      i = (i - 1) & mask;
      entry = ttable->entries + i;
    }

    // 4. If load factor is above 75 %, remove couple extra entries

    if (ttable->count > ((mask + 1) * 3) >> 2)
    {
      while (entry->key /*&& !entry->thread_id*/)
      {
        entry->key = 0;
        entry->data = 0;
        uo_atomic_decrement(&ttable->count);

        i = (i - 1) & mask;
        entry = ttable->entries + i;
      }
    }

    return false;
  }

  static inline void uo_ttable_set(uo_ttable *ttable, const uo_position *position, const uo_tdata *data)
  {
    uint64_t mask = ttable->hash_mask;
    uint64_t hash = position->key & mask;
    uint32_t key = (uint32_t)position->key;
    uint64_t i = hash;
    uo_tentry *entry = ttable->entries + i;

    // 1. Look for matching key and stop on first empty key

    while (entry->key && !uo_tentry_test_key(entry, key))
    {
      i = (i + 1) & mask;
      entry = ttable->entries + i;
    }

    // 2. If exact match was found, return exisiting entry

    if (entry->key)
    {
      entry->data = data->data;
      return;
    }

    // 3. No exact match was found, return first vacant slot or replace first expired entry or else replace `uo_ttable_max_probe` th entry

    uint8_t root_ply = position->root_ply;

    i = hash;
    entry = ttable->entries + i;

    for (size_t j = 0; j < uo_ttable_max_probe; ++j)
    {
      if (!entry->key)
      {
        entry->data = data->data;
        entry->expiry_ply = root_ply + uo_ttable_expiry_ply;
        entry->key = uo_tentry_create_key(entry, key);
        uo_atomic_increment(&ttable->count);
        return;
      }

      if (/*!entry->thread_id && */entry->expiry_ply < root_ply)
      {
        entry->data = data->data;
        entry->expiry_ply = root_ply + uo_ttable_expiry_ply;
        entry->key = uo_tentry_create_key(entry, key);
        return;
      }

      i = (i + 1) & mask;
      entry = ttable->entries + i;
    }

    while (entry->key/* && entry->thread_id*/)
    {
      i = (i + 1) & mask;
      entry = ttable->entries + i;
    }

    if (!entry->key)
    {
      entry->data = data->data;
      entry->expiry_ply = root_ply + uo_ttable_expiry_ply;
      entry->key = uo_tentry_create_key(entry, key);
      uo_atomic_increment(&ttable->count);
      return;
    }

    entry->data = data->data;
    entry->expiry_ply = root_ply + uo_ttable_expiry_ply;
    entry->key = uo_tentry_create_key(entry, key);
    return;
  }

#ifdef __cplusplus
}
#endif

#endif
