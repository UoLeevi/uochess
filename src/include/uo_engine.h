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
#include <stddef.h>
#include <inttypes.h>

  typedef struct uo_engine_init_options
  {
    size_t threads;
    uint16_t multipv;
    size_t hash_size;
  } uo_engine_init_options;

  typedef struct uo_engine uo_engine;

  typedef struct uo_engine_thread
  {
    uo_thread *thread;
    uo_engine *engine;
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
    volatile uo_atomic_int stop;
    bool exit;
  } uo_engine;

  void uo_engine_init(uo_engine *engine, uo_engine_init_options *options);

  static inline void uo_engine_lock_position(uo_engine *engine)
  {
    uo_mutex_lock(engine->position_mutex);
  }

  static inline void uo_engine_unlock_position(uo_engine *engine)
  {
    uo_mutex_unlock(engine->position_mutex);
  }

  static inline void uo_engine_lock_stdout(uo_engine *engine)
  {
    uo_mutex_lock(engine->stdout_mutex);
  }

  static inline void uo_engine_unlock_stdout(uo_engine *engine)
  {
    uo_mutex_unlock(engine->stdout_mutex);
  }

  static inline void uo_engine_thread_load_position(uo_engine_thread *thread)
  {
    uo_mutex_lock(thread->engine->position_mutex);
    uo_position *position = &thread->engine->position;
    uo_position_copy(&thread->position, position);
    uo_mutex_unlock(thread->engine->position_mutex);
  }

  void uo_engine_start_search(uo_engine *engine, uo_search_params *params);

  static inline void uo_engine_stop_search(uo_engine *engine)
  {
    uo_atomic_store(&engine->stop, 1);
  }

#ifdef __cplusplus
}
#endif

#endif
