#ifndef UO_TTABLE_H
#define UO_TTABLE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_move.h"
#include "uo_thread.h"
#include "uo_search.h"
#include "uo_misc.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

  typedef struct uo_tentry
  {
    uint64_t key;
    uo_tdata data;
  } uo_tentry;

#define uo_score_type__exact 3
#define uo_score_type__upper_bound 8
#define uo_score_type__lower_bound 4

#define uo_ttable_max_probe 4
#define uo_ttable_expiry_ply 2

  typedef struct uo_ttable
  {
    int64_t *counts;
    size_t thread_count;
    uint64_t hash_mask;
    uo_tentry *entries;
  } uo_ttable;

  static inline void uo_ttable_init(uo_ttable *ttable, uint8_t hash_bits, size_t thread_count)
  {
    uint64_t capacity = (uint64_t)1 << (hash_bits - 1);

    ttable->entries = calloc(capacity, sizeof * ttable->entries);
    ttable->counts = calloc(thread_count, sizeof * ttable->counts);
    ttable->hash_mask = capacity - 1;
    ttable->thread_count = thread_count;
  }

  static inline size_t uo_ttable_count(uo_ttable *ttable)
  {
    int64_t count = 0;

    for (size_t i = 0; i < ttable->thread_count; ++i)
    {
      count += ttable->counts[i];
    }

    return count;
  }

  static inline void uo_ttable_clear(uo_ttable *ttable)
  {
    if (uo_ttable_count(ttable) == 0) return;

    memset(ttable->entries, 0, (ttable->hash_mask + 1) * sizeof * ttable->entries);
    memset(ttable->counts, 0, ttable->thread_count * sizeof * ttable->counts);

  }

  static inline void uo_ttable_free(uo_ttable *ttable)
  {
    free(ttable->entries);
    free(ttable->counts);
  }

  static inline bool uo_ttable_get(uo_ttable *ttable, uint64_t key, uint16_t root_ply, uo_tdata *data, int thread_index)
  {
    uint64_t mask = ttable->hash_mask;
    uint64_t hash = key & mask;
    uint64_t i = hash;
    uo_tentry *entry = ttable->entries + i;

    // 1. Look for matching key or stop on first empty key

    uint64_t tdata = entry->data.data;
    uo_mfence();
    while (entry->key && (entry->key ^ tdata) != key)
    {
      i = (i + 1) & mask;
      entry = ttable->entries + i;
      uo_mfence();
      tdata = entry->data.data;
    }

    // 2. If exact match was found, return entry

    if (entry->key)
    {
      data->data = tdata;
      return true;
    }

    // 3. Loop backwards and remove consecutive expired entries

    i = (i - 1) & mask;
    entry = ttable->entries + i;

    int count_removed = 0;

    while (entry->key && entry->data.root_ply + uo_ttable_expiry_ply < root_ply)
    {
      entry->key = 0;
      entry->data.data = 0;
      ++count_removed;

      i = (i - 1) & mask;
      entry = ttable->entries + i;
    }

    ttable->counts[thread_index] -= count_removed;

    return false;
  }

  static inline void uo_ttable_set(uo_ttable *ttable, uint64_t key, const uo_tdata *data, int thread_index)
  {
    uint64_t mask = ttable->hash_mask;
    uint64_t hash = key & mask;
    uint64_t tdata = data->data;
    uint64_t i = hash;
    uo_tentry *entry = ttable->entries + i;

    // 1. Look for matching key and stop on first empty key

    while (entry->key && (entry->key ^ entry->data.data) != key)
    {
      i = (i + 1) & mask;
      entry = ttable->entries + i;
    }

    // 2. If exact match was found, return exisiting entry

    if (entry->key)
    {
      entry->key = key ^ data->data;
      entry->data.data = data->data;
      return;
    }

    // 3. No exact match was found, return first vacant slot or replace first expired entry or else replace `uo_ttable_max_probe` th entry

    i = hash;
    entry = ttable->entries + i;

    for (size_t j = 0; j < uo_ttable_max_probe; ++j)
    {
      if (!entry->key)
      {
        bool is_vacant = !entry->key;

        entry->data.data = data->data;
        entry->key = key ^ data->data;

        if (is_vacant) ++ttable->counts[thread_index];
        return;
      }

      if (entry->data.root_ply + uo_ttable_expiry_ply < data->root_ply)
      {
        bool is_vacant = !entry->key;

        entry->data.data = data->data;
        entry->key = key ^ data->data;

        if (is_vacant) ++ttable->counts[thread_index];
        return;
      }

      i = (i + 1) & mask;
      entry = ttable->entries + i;
    }

    bool is_vacant = !entry->key;

    entry->data.data = data->data;
    entry->key = key ^ data->data;

    if (is_vacant) ++ttable->counts[thread_index];
    return;
  }

#ifdef __cplusplus
}
#endif

#endif
