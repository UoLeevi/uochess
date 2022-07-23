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
    uo_thread *thread;
    uo_tentry *entry;
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
    uo_engine_lock_position();
    engine.position.stack->moves_generated = false;
    engine.position.update_status.checks_and_pins = false;
    uo_engine_unlock_position();
    uo_engine_unlock_ttable();
  }

  static inline bool uo_engine_lookup_entry(const uo_position *position, uo_tentry **entry, uo_move *bestmove, int16_t *value, int16_t *alpha, int16_t *beta, uint8_t depth)
  {
    uo_engine_lock_ttable();
    uo_tentry *tte = *entry = uo_ttable_get(&engine.ttable, position);

    if (!tte || tte->depth < depth)
    {
      uo_engine_unlock_ttable();
      return false;
    }

    if (tte->type == uo_tentry_type__exact)
    {
      *bestmove = tte->bestmove;
      *value = tte->value;
      uo_engine_unlock_ttable();
      return true;
    }

    if (tte->type == uo_tentry_type__lower_bound && tte->value > *alpha)
    {
      *alpha = tte->value;
    }
    else if (tte->type == uo_tentry_type__upper_bound && tte->value < *beta)
    {
      *bestmove = tte->bestmove;
      *beta = tte->value;
    }

    if (*alpha >= *beta)
    {
      *value = tte->value;
      uo_engine_unlock_ttable();
      return true;
    }

    uo_engine_unlock_ttable();
    return false;
  }

  static inline void uo_engine_store_entry(const uo_position *position, uo_tentry *entry, uo_move bestmove, int16_t value, int16_t alpha, int16_t beta, uint8_t depth)
  {
    assert(bestmove);

    uint8_t type =
      value >= beta ? uo_tentry_type__lower_bound :
      value <= alpha ? uo_tentry_type__upper_bound :
      uo_tentry_type__exact;

    if (!entry)
    {
      uo_engine_lock_ttable();
      entry = uo_ttable_set(&engine.ttable, position);
      entry->depth = depth;
      entry->bestmove = bestmove;
      entry->value = value;
      entry->type = type;
      uo_engine_unlock_ttable();
    }
    else
    {
      uo_engine_lock_ttable();
      if (entry->depth <= depth)
      {
        entry->depth = depth;
        entry->bestmove = bestmove;
        entry->value = value;
        entry->type = type;
      }
      uo_engine_unlock_ttable();
    }
  }

  void uo_engine_start_search();

  static inline bool uo_engine_is_stopped()
  {
    return uo_atomic_compare_exchange(&engine.stopped, 1, 1);
  }

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
