#ifndef UO_STRMAP_H
#define UO_STRMAP_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define UO_STRMAP_INITIAL_SIZE 11
#define UO_STRMAP_LOAD_FACTOR 0.75

  typedef struct uo_strmap_entry {
    char *key;
    void *value;
    size_t is_occupied;
    size_t is_deleted;
  } uo_strmap_entry;

  typedef struct uo_strmap {
    size_t size;
    size_t occupied;
    size_t deleted;
    uo_strmap_entry *table;
  } uo_strmap;

  static inline size_t uo_strmap_hash(uo_strmap *strmap, char *key)
  {
    size_t hash = 2166136261;

    for (size_t i = 0; i < strlen(key); i++)
    {
        hash = (hash ^ key[i]) * 16777619;
    }

    return hash % strmap->size;
  }

  static inline size_t uo_strmap_is_prime(size_t number)
  {
    if (number <= 2)
    {
      return (number == 2);
    }

    if (number % 2 == 0)
    {
      return 0;
    }

    for (size_t i = 3; i * i <= number; i += 2)
    {
      if (number % i == 0)
      {
        return 0;
      }
    }

    return 1;
  }

  static inline size_t uo_strmap_next_prime(size_t number)
  {
    while (!uo_strmap_is_prime(number))
    {
      number++;
    }

    return number;
  }

  static inline uo_strmap *uo_strmap_create()
  {
    uo_strmap *strmap = malloc(sizeof(uo_strmap));
    strmap->size = uo_strmap_next_prime(UO_STRMAP_INITIAL_SIZE);
    strmap->occupied = 0;
    strmap->deleted = 0;
    strmap->table = malloc(sizeof(uo_strmap_entry) * strmap->size);

    for (size_t i = 0; i < strmap->size; i++)
    {
      strmap->table[i].is_occupied = 0;
      strmap->table[i].is_deleted = 0;
    }

    return strmap;
  }

  static inline void uo_strmap_free(uo_strmap *strmap)
  {
    free(strmap->table);
    free(strmap);
  }

  static inline void uo_strmap_rehash(uo_strmap *strmap)
  {
    size_t old_size = strmap->size;
    uo_strmap_entry *old_table = strmap->table;

    strmap->size = uo_strmap_next_prime(strmap->size * 2);
    strmap->table = malloc(sizeof(uo_strmap_entry) * strmap->size);

    for (size_t i = 0; i < strmap->size; i++)
    {
      strmap->table[i].is_occupied = 0;
      strmap->table[i].is_deleted = 0;
    }

    strmap->occupied = 0;
    strmap->deleted = 0;

    for (size_t i = 0; i < old_size; i++)
    {
      if (old_table[i].is_occupied && !old_table[i].is_deleted)
      {
        uo_strmap_add(strmap, old_table[i].key, old_table[i].value);
      }
    }

    free(old_table);
  }

  static inline void uo_strmap_add(uo_strmap *strmap, char *key, void *value)
  {
    if (((float)strmap->occupied + strmap->deleted) / strmap->size >= UO_STRMAP_LOAD_FACTOR)
    {
      uo_strmap_rehash(strmap);
    }

    size_t index = uo_strmap_hash(strmap, key);
    while (strmap->table[index].is_occupied)
    {
      if (strmap->table[index].is_deleted)
      {
        strmap->deleted--;
        break;
      }

      if (strcmp(strmap->table[index].key, key) == 0)
      {
        strmap->table[index].value = value;
        return;
      }

      index = (index + 1) % strmap->size;
    }

    strmap->table[index].key = key;
    strmap->table[index].value = value;
    strmap->table[index].is_occupied = 1;
    strmap->table[index].is_deleted = 0;
    strmap->occupied++;
  }

  static inline void *uo_strmap_get(uo_strmap *strmap, char *key)
  {
    size_t index = uo_strmap_hash(strmap, key);
    while (strmap->table[index].is_occupied)
    {
      if (strmap->table[index].is_deleted)
      {
        index = (index + 1) % strmap->size;
        continue;
      }

      if (strcmp(strmap->table[index].key, key) == 0)
      {
        return strmap->table[index].value;
      }

      index = (index + 1) % strmap->size;
    }
    return NULL;
  }

  static inline void uo_strmap_remove(uo_strmap *strmap, char *key)
  {
    size_t index = uo_strmap_hash(strmap, key);
    while (strmap->table[index].is_occupied)
    {
      if (strcmp(strmap->table[index].key, key) == 0)
      {
        strmap->table[index].is_deleted = 1;
        strmap->deleted++;
        return;
      }

      index = (index + 1) % strmap->size;
    }
  }

#ifdef __cplusplus
}
#endif

#endif
