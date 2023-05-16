#ifndef UO_BOOK_H
#define UO_BOOK_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_move.h"
#include "uo_thread.h"
#include "uo_position.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

  typedef struct uo_book_entry
  {
    uint64_t key;
    struct
    {
      uo_move bestmove;
      int16_t value;
      uint8_t depth;
    };
  } uo_book_entry;

  typedef struct uo_book
  {
    uint64_t hash_mask;
    uo_book_entry *entries;
  } uo_book;

  static inline bool uo_book_init(uo_book *book, const char *filepath);

  static inline void uo_book_free(uo_book *book)
  {
    free(book->entries);
  }

  static inline const uo_book_entry *uo_book_get(uo_book *book, const uo_position *position)
  {
    uint64_t mask = book->hash_mask;
    uint64_t hash = position->key & mask;
    uint32_t key = (uint32_t)position->key;
    uint64_t i = hash;
    uo_book_entry *entry = book->entries + i;

    // 1. Look for matching key or stop on first empty key

    while (entry->key && entry->key != key)
    {
      i = (i + 1) & mask;
      entry = book->entries + i;
    }

    return entry->key ? entry : NULL;
  }

#ifdef __cplusplus
}
#endif

#endif
