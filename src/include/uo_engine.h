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
#include <inttypes.h>

  typedef struct uo_engine_options
  {
    size_t threads;
    uint16_t multipv;
    size_t hash_size;
  } uo_engine_options;

  typedef struct uo_engine_thread
  {
    uo_thread *thread;
    uo_position position;
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

  static inline void uo_engine_ttable_store(uo_tentry *entry, const uo_position *position,
    uo_move bestmove, int16_t value, uint8_t depth, uint8_t type)
  {
    if (type & uo_tentry_type__quiesce)
    {
      if (position->ply <= uo_ttable_quiesce_max_ply) {
        uo_engine_lock_ttable();
        if (!entry || (entry->type & uo_tentry_type__quiesce))
        {
          if (!entry) entry = uo_ttable_set(&engine.ttable, position);
          entry->depth = 0;
          entry->bestmove = bestmove;
          entry->value = value;
          entry->type = type;
        }
        uo_engine_unlock_ttable();
      }
    }
    else
    {
      uo_engine_lock_ttable();
      if (!entry || (entry->depth <= depth))
      {
        if (!entry) entry = uo_ttable_set(&engine.ttable, position);
        entry->depth = depth;
        entry->bestmove = bestmove;
        entry->value = value;
        entry->type = type;
      }
      uo_engine_unlock_ttable();
    }
  }

  static inline void uo_engine_thread_load_position(uo_engine_thread *thread)
  {
    uo_mutex_lock(engine.position_mutex);
    uo_position_copy(&thread->position, &engine.position);
    uo_mutex_unlock(engine.position_mutex);
  }

  void uo_engine_start_search(uo_search_params *params);

  void uo_engine_start_quiescence_search(uo_search_params *params);

  static inline bool uo_engine_is_stopped()
  {
    return uo_atomic_compare_exchange(&engine.stopped, 1, 1);
  }

  static inline void uo_engine_stop_search()
  {
    uo_atomic_store(&engine.stopped, 1);
  }

#ifdef __cplusplus
  }
#endif

#endif
