#include "uo_book.h"
#include "uo_misc.h"
#include "uo_util.h"

static inline uo_book_entry *uo_book_set(uo_book *book, uint64_t key)
{
  uint64_t mask = book->hash_mask;
  uint64_t hash = key & mask;
  uint64_t i = hash;
  uo_book_entry *entry = book->entries + i;

  // 1. Look for matching key or stop on first empty key

  while (entry->key && entry->key != key)
  {
    i = (i + 1) & mask;
    entry = book->entries + i;
  }

  // 2. Set the key and return entry

  entry->key = key;

  return entry;
}

static inline bool uo_book_init(uo_book *book, const char *filepath)
{
  // Step 1. Open opening book file for reading
  uo_file_mmap *file_mmap = uo_file_mmap_open_read(filepath);
  if (!file_mmap) return false;

  char *ptr = uo_file_mmap_readline(file_mmap);
  if (!ptr)
  {
    uo_file_mmap_close(file_mmap);
    return false;
  }

  // Step 2. Allocate sufficient memory

  uint64_t capacity = file_mmap->size / 100 // fen is about 100 bytes
    * 10 // one position could have maybe 10 move pv on average
    * 8 // let's make some room to minimize collisions
    ;

  uint8_t hash_bits = uo_msb(capacity) + 1;
  capacity = (uint64_t)1 << (hash_bits - 1);
  uint64_t lock_count = (uint64_t)1 << UO_TTABLE_LOCK_BITS;

  book->entries = calloc(capacity, sizeof * book->entries);
  book->hash_mask = capacity - 1;

  // Step 3. Read book file line by line

  uo_position position;

  while (ptr)
  {
    if (strlen(ptr) == 0)
    {
      ptr = uo_file_mmap_readline(file_mmap);
      continue;
    }

    char *fen = ptr;
    char *eval = strchr(*ptr, ',') + 1;

    if (!eval)
    {
      // Invalid line, evaluation is missing
      ptr = uo_file_mmap_readline(file_mmap);
      continue;
    }

    if (!uo_position_from_fen(&position, fen))
    {
      // Invalid fen
      ptr = uo_file_mmap_readline(file_mmap);
      continue;
    }

    uo_book_entry *entry = uo_book_set(book, position.key);

    uint8_t color = uo_color(position.flags);
    assert(color == uo_white || color == uo_black);

    char *end;
    if (eval[0] == '#')
    {
      int mate_distance = strtol(eval + 1, &end, 10);
      if (color == uo_black) mate_distance = -mate_distance;
      entry->value = mate_distance >= 0
        ? uo_score_checkmate - mate_distance * 2
        : -uo_score_checkmate - mate_distance * 2;
    }
    else
    {
      int score = strtol(eval, &end, 10);
      if (color == uo_black) score = -score;
      entry->value = score;
    }

    char *pv = strchr(*eval, ' pv ') + 4;
    if (pv)
    {
      // TODO: Parse moves
      return;
    }
  }
}
