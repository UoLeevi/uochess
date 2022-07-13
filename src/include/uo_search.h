#ifndef UO_SEARCH_H
#define UO_SEARCH_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_position.h"
#include "uo_ttable.h"
#include "uo_thread.h"
#include "uo_def.h"

#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>

  typedef struct uo_search_init_options
  {
    size_t threads;
    uint16_t multipv;
    size_t hash_size;
  } uo_search_init_options;

  typedef struct uo_search uo_search;

  typedef struct uo_search_thread
  {
    uo_thread *thread;
    uo_search *search;
    uo_position position;
  } uo_search_thread;

  typedef struct uo_search_thread_work
  {
    union
    {
      uo_search_thread *thread;
      uo_thread_function *function;
    };
    void *data;
  } uo_search_thread_work;

#define uo_search_work_queue_max_count 0x100

  typedef struct uo_search_work_queue {
    uo_semaphore *semaphore;
    uo_mutex *mutex;
    int head;
    int tail;
    uo_search_thread_work work[uo_search_work_queue_max_count];
  } uo_search_work_queue;

  typedef struct uo_search
  {
    uo_ttable ttable;
    uo_tentry *pv;
    uo_search_thread *threads;
    size_t thread_count;
    uo_search_work_queue work_queue;
    uo_mutex *stdout_mutex;
    uo_mutex *position_mutex;
    uo_position position;
    volatile uo_atomic_int stop;
    bool exit;
  } uo_search;

  typedef struct uo_search_params
  {
    uint8_t depth;
    uint64_t movetime;
    uint16_t multipv;
  } uo_search_params;

  void uo_search_init(uo_search *search, uo_search_init_options *options);

  static inline void uo_search_lock_position(uo_search *search)
  {
    uo_mutex_lock(search->position_mutex);
  }

  static inline void uo_search_unlock_position(uo_search *search)
  {
    uo_mutex_unlock(search->position_mutex);
  }

  static inline void uo_search_lock_stdout(uo_search *search)
  {
    uo_mutex_lock(search->stdout_mutex);
  }

  static inline void uo_search_unlock_stdout(uo_search *search)
  {
    uo_mutex_unlock(search->stdout_mutex);
  }

  void uo_search_start(uo_search *search, uo_search_params params);

  void uo_search_stop(uo_search *search);

#ifdef __cplusplus
  }
#endif

#endif
