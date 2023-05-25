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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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

#define uo_tentry_type__exact 3
#define uo_tentry_type__upper_bound 8
#define uo_tentry_type__lower_bound 4

#define uo_ttable_max_probe 4
#define uo_ttable_expiry_ply 3

  typedef struct uo_ttable
  {
    uint64_t hash_mask;
    uint64_t lock_mask;
    uo_atomic_int count;
    uo_tentry *entries;
    uo_atomic_flag locks[1 << UO_TTABLE_LOCK_BITS];
  } uo_ttable;

  static inline void uo_ttable_init(uo_ttable *ttable, uint8_t hash_bits)
  {
    uint64_t capacity = (uint64_t)1 << (hash_bits - 1);
    uint64_t lock_count = (uint64_t)1 << UO_TTABLE_LOCK_BITS;

    ttable->entries = calloc(capacity, sizeof * ttable->entries);
    ttable->hash_mask = capacity - 1;
    ttable->lock_mask = lock_count - 1;
    uo_atomic_init(&ttable->count, 0);

    for (size_t i = 0; i < lock_count; ++i)
    {
      uo_atomic_flag_init(&ttable->locks[i]);
    }
  }

  static inline void uo_ttable_clear(uo_ttable *ttable)
  {
    memset(ttable->entries, 0, (ttable->hash_mask + 1) * sizeof * ttable->entries);
    uo_atomic_store(&ttable->count, 0);

    uint64_t lock_count = (uint64_t)1 << UO_TTABLE_LOCK_BITS;

    for (size_t i = 0; i < lock_count; ++i)
    {
      uo_atomic_flag_clear(&ttable->locks[i]);
    }
  }

  static inline void uo_ttable_free(uo_ttable *ttable)
  {
    free(ttable->entries);
  }

  static inline bool uo_ttable_get(uo_ttable *ttable, const uo_position *position, uo_tdata *data)
  {
    uint64_t mask = ttable->hash_mask;
    uint64_t hash = position->key & mask;
    uint32_t key = (uint32_t)position->key;
    uint64_t i = hash;
    uo_tentry *entry = ttable->entries + i;

    // 1. Look for matching key or stop on first empty key

    while (entry->key && entry->key != key)
    {
      i = (i + 1) & mask;
      entry = ttable->entries + i;
    }

    // 2. If exact match was found, return entry

    if (entry->key)
    {
      uo_atomic_flag *lock = &ttable->locks[i & ttable->lock_mask];
      uo_atomic_lock(lock);

      if (entry->key != key)
      {
        uo_atomic_unlock(lock);
        return uo_ttable_get(ttable, position, data);
      }

      data->data = entry->data;

      uo_atomic_unlock(lock);
      return true;
    }

    // 3. Loop backwards and remove consecutive expired entries
    uint8_t root_ply = position->root_ply;

    i = (i - 1) & mask;
    entry = ttable->entries + i;

    uo_atomic_flag *lock = &ttable->locks[i & ttable->lock_mask];
    uo_atomic_lock(lock);

    int count_removed = 0;

    while (entry->key && entry->expiry_ply < root_ply)
    {
      entry->key = 0;
      entry->data = 0;
      ++count_removed;

      if (lock != &ttable->locks[i & ttable->lock_mask]) break;

      i = (i - 1) & mask;
      entry = ttable->entries + i;
    }

    uo_atomic_unlock(lock);

    // 4. If load factor is above 75 %, remove couple extra entries

    if (ttable->count > ((mask + 1) * 3) >> 2)
    {
      uo_atomic_flag *lock = &ttable->locks[i & ttable->lock_mask];
      uo_atomic_lock(lock);

      while (entry->key)
      {
        entry->key = 0;
        entry->data = 0;
        ++count_removed;

        if (lock != &ttable->locks[i & ttable->lock_mask]) break;

        i = (i - 1) & mask;
        entry = ttable->entries + i;
      }

      uo_atomic_unlock(lock);
    }

    if (count_removed) uo_atomic_sub(&ttable->count, count_removed);

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

    while (entry->key && entry->key != key)
    {
      i = (i + 1) & mask;
      entry = ttable->entries + i;
    }

    // 2. If exact match was found, return exisiting entry

    if (entry->key)
    {
      uo_atomic_flag *lock = &ttable->locks[i & ttable->lock_mask];
      uo_atomic_lock(lock);

      if (entry->key != key)
      {
        uo_atomic_unlock(lock);
        uo_ttable_set(ttable, position, data);
        return;
      }

      entry->data = data->data;
      entry->expiry_ply = position->root_ply + uo_ttable_expiry_ply;

      uo_atomic_unlock(lock);
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
        uo_atomic_flag *lock = &ttable->locks[i & ttable->lock_mask];
        uo_atomic_lock(lock);

        bool is_vacant = !entry->key;

        entry->data = data->data;
        entry->expiry_ply = root_ply + uo_ttable_expiry_ply;
        entry->key = key;

        if (is_vacant) uo_atomic_increment(&ttable->count);

        uo_atomic_unlock(lock);
        return;
      }

      if (entry->expiry_ply < root_ply)
      {
        uo_atomic_flag *lock = &ttable->locks[i & ttable->lock_mask];
        uo_atomic_lock(lock);

        bool is_vacant = !entry->key;

        entry->data = data->data;
        entry->expiry_ply = root_ply + uo_ttable_expiry_ply;
        entry->key = key;

        if (is_vacant) uo_atomic_increment(&ttable->count);

        uo_atomic_unlock(lock);
        return;
      }

      i = (i + 1) & mask;
      entry = ttable->entries + i;
    }

    while (entry->key)
    {
      i = (i + 1) & mask;
      entry = ttable->entries + i;
    }

    if (!entry->key)
    {
      uo_atomic_flag *lock = &ttable->locks[i & ttable->lock_mask];
      uo_atomic_lock(lock);

      bool is_vacant = !entry->key;

      entry->data = data->data;
      entry->expiry_ply = root_ply + uo_ttable_expiry_ply;
      entry->key = key;

      if (is_vacant) uo_atomic_increment(&ttable->count);

      uo_atomic_unlock(lock);
      return;
    }

    uo_atomic_flag *lock = &ttable->locks[i & ttable->lock_mask];
    uo_atomic_lock(lock);

    bool is_vacant = !entry->key;

    entry->data = data->data;
    entry->expiry_ply = root_ply + uo_ttable_expiry_ply;
    entry->key = key;

    if (is_vacant) uo_atomic_increment(&ttable->count);

    uo_atomic_unlock(lock);
    return;
  }

#ifdef __cplusplus
}
#endif

#endif
