#ifndef UO_ENGINE_H
#define UO_ENGINE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_position.h"
#include "uo_ttable.h"
#include "uo_thread.h"
#include "uo_search.h"
#include "uo_def.h"

#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>

  typedef struct uo_engine_options
  {
    size_t threads;
    uint16_t multipv;
    size_t hash_size;
  } uo_engine_options;

  typedef struct uo_engine_thread
  {
    uint8_t id;
    uo_thread *thread;
    uo_position position;
    uo_search_info info;
  } uo_engine_thread;

  typedef struct uo_engine_thread_work
  {
    union
    {
      uo_engine_thread *thread;
      uo_thread_function *function;
    };
    void *data;
  } uo_engine_thread_work;

#define uo_engine_work_queue_max_count 0x100

  typedef struct uo_engine_work_queue {
    uo_semaphore *semaphore;
    uo_mutex *mutex;
    int head;
    int tail;
    uo_engine_thread_work work[uo_engine_work_queue_max_count];
  } uo_engine_work_queue;

  typedef struct uo_engine
  {
    uo_ttable ttable;
    uo_tentry *pv;
    uo_engine_thread *threads;
    size_t thread_count;
    uo_engine_work_queue work_queue;
    uo_mutex *stdout_mutex;
    uo_mutex *position_mutex;
    uo_position position;
    uo_search_params search_params;
    volatile uo_atomic_int stopped;
    bool exit;
  } uo_engine;

  extern uo_engine_options engine_options;
  extern uo_engine engine;

  void uo_engine_load_default_options();
  void uo_engine_init();

  static inline void uo_engine_lock_position()
  {
    uo_mutex_lock(engine.position_mutex);
  }

  static inline void uo_engine_unlock_position()
  {
    uo_mutex_unlock(engine.position_mutex);
  }

  static inline void uo_engine_lock_stdout()
  {
    uo_mutex_lock(engine.stdout_mutex);
  }

  static inline void uo_engine_unlock_stdout()
  {
    fflush(stdout);
    uo_mutex_unlock(engine.stdout_mutex);
  }

  static inline void uo_engine_lock_ttable()
  {
    uo_ttable_lock(&engine.ttable);
  }

  static inline void uo_engine_unlock_ttable()
  {
    uo_ttable_unlock(&engine.ttable);
  }

  static inline void uo_engine_clear_hash()
  {
    uo_engine_lock_ttable();
    uo_ttable_clear(&engine.ttable);
    uo_engine_unlock_ttable();
  }

  static inline bool uo_engine_is_stopped()
  {
    return uo_atomic_compare_exchange(&engine.stopped, 1, 1);
  }

  typedef struct uo_abtentry
  {
    const int16_t alpha;
    const int16_t beta;
    int8_t depth;
    uo_move bestmove;
    int16_t value;
    uo_tentry *entry;
    int16_t hardalpha;
    int16_t hardbeta;
  } uo_abtentry;

  // https://groups.google.com/g/rec.games.chess.computer/c/p8GbiiLjp0o/m/hKkpT8qfrhQJ
  static inline bool uo_engine_lookup_entry(const uo_position *position, uo_abtentry *abtentry)
  {
    uo_engine_lock_ttable();
    abtentry->entry = uo_ttable_get(&engine.ttable, position);

    if (abtentry->entry)
    {
      abtentry->bestmove = abtentry->entry->bestmove;

      if (abtentry->entry->depth < abtentry->depth)
      {
        abtentry->hardalpha = -UO_SCORE_CHECKMATE;
        abtentry->hardbeta = UO_SCORE_CHECKMATE;
        uo_engine_unlock_ttable();
        return false;
      }
    }
    else
    {
      abtentry->hardalpha = -UO_SCORE_CHECKMATE;
      abtentry->hardbeta = UO_SCORE_CHECKMATE;
      uo_engine_unlock_ttable();
      return false;
    }



    if (abtentry->entry->type == uo_tentry_type__exact)
    {
      abtentry->value = abtentry->entry->value;
      uo_engine_unlock_ttable();
      return true;
    }

    if (abtentry->entry->type == uo_tentry_type__lower_bound)
    {
      if (abtentry->entry->value >= abtentry->beta)
      {
        abtentry->value = abtentry->entry->value;
        uo_engine_unlock_ttable();
        return true;
      }

      abtentry->hardalpha = abtentry->entry->value;
      abtentry->hardbeta = UO_SCORE_CHECKMATE;
    }
    else // if (abtentry->entry->type == uo_tentry_type__upper_bound)
    {
      if (abtentry->entry->value <= abtentry->alpha)
      {
        abtentry->value = abtentry->entry->value;
        uo_engine_unlock_ttable();
        return true;
      }

      abtentry->hardalpha = -UO_SCORE_CHECKMATE;
      abtentry->hardbeta = abtentry->entry->value;
    }

    uo_engine_unlock_ttable();
    return false;
  }

  static inline int16_t uo_engine_store_entry(const uo_position *position, uo_abtentry *abtentry)
  {
    if (abtentry->value < abtentry->hardalpha) abtentry->value = abtentry->hardalpha;
    if (abtentry->value > abtentry->hardbeta) abtentry->value = abtentry->hardbeta;

    if (!abtentry->bestmove || uo_engine_is_stopped()) return abtentry->value;

    uint8_t type =
      abtentry->value >= abtentry->beta ? uo_tentry_type__lower_bound :
      abtentry->value <= abtentry->alpha ? uo_tentry_type__upper_bound :
      uo_tentry_type__exact;

    uo_tentry *entry = abtentry->entry;

    if (!entry)
    {
      uo_engine_lock_ttable();
      entry = abtentry->entry = uo_ttable_set(&engine.ttable, position);
      entry->depth = abtentry->depth;
      entry->bestmove = abtentry->bestmove;
      entry->value = abtentry->value;
      entry->type = type;
      uo_engine_unlock_ttable();
    }
    else
    {
      uo_engine_lock_ttable();
      if (entry->depth <= abtentry->depth)
      {
        entry->depth = abtentry->depth;
        entry->bestmove = abtentry->bestmove;
        entry->value = abtentry->value;
        entry->type = type;
      }
      uo_engine_unlock_ttable();
    }

    return abtentry->value;
  }

  void uo_engine_start_search();

  static inline void uo_engine_reset_search_params(uint8_t seach_type)
  {
    assert(uo_engine_is_stopped());
    engine.search_params = (uo_search_params){
      .seach_type = seach_type,
      .depth = UO_MAX_PLY,
      .alpha = -UO_SCORE_CHECKMATE,
      .beta = UO_SCORE_CHECKMATE
    };
  }

  static inline void uo_engine_stop_search()
  {
    uo_atomic_store(&engine.stopped, 1);
  }

  void uo_engine_queue_work(uo_thread_function *function, void *data);

#ifdef __cplusplus
}
#endif

#endif
