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
#include "uo_evaluation.h"
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

  typedef struct uo_search_queue_item
  {
    uo_engine_thread *thread;
    uo_move move;
    int16_t value;
    size_t nodes;
    size_t depth;
  } uo_search_queue_item;

  typedef struct uo_search_queue {
    volatile uo_atomic_int busy;
    volatile uo_atomic_int pending_count;
    volatile uo_atomic_int count;
    int head;
    int tail;
    bool init;
    uo_search_queue_item items[UO_PARALLEL_MAX_COUNT];
    uo_engine_thread *threads[UO_PARALLEL_MAX_COUNT];
  } uo_search_queue;

  typedef struct uo_engine_thread
  {
    uint8_t id;
    uo_thread *thread;
    uo_engine_thread *owner;
    uo_semaphore *semaphore;
    uo_thread_function *function;
    void *data;
    uo_position position;
    uo_search_info info;
    volatile uo_atomic_int busy;
    volatile uo_atomic_int cutoff;
  } uo_engine_thread;

  typedef struct uo_engine_thread_queue {
    volatile uo_atomic_int count;
    volatile uo_atomic_int busy;
    int head;
    int tail;
    uo_engine_thread **threads;
  } uo_engine_thread_queue;

  typedef struct uo_engine
  {
    uo_ttable ttable;
    uo_tentry *pv;
    uo_engine_thread *threads;
    size_t thread_count;
    uo_engine_thread_queue thread_queue;
    uo_mutex *stdout_mutex;
    uo_mutex *position_mutex;
    uo_position position;
    uo_search_params search_params;
    struct
    {
      uint64_t key;
      int16_t value;
      uo_move move;
    } ponder;
    volatile uo_atomic_int stopped;
    bool exit;
  } uo_engine;

  extern uo_engine_options engine_options;
  extern uo_engine engine;

  void uo_engine_load_default_options();
  void uo_engine_init();
  void uo_engine_reconfigure();

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
    return uo_atomic_load(&engine.stopped) == 1;
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

    int16_t value;

    if (abtentry->entry)
    {
      value = uo_score_adjust_for_mate_from_ttable(position, abtentry->entry->value);
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
      abtentry->value = value;
      uo_engine_unlock_ttable();
      return true;
    }

    if (abtentry->entry->type == uo_tentry_type__lower_bound)
    {
      if (value >= abtentry->beta)
      {
        abtentry->value = value;
        uo_engine_unlock_ttable();
        return true;
      }

      abtentry->hardalpha = value;
      abtentry->hardbeta = UO_SCORE_CHECKMATE;
    }
    else // if (abtentry->entry->type == uo_tentry_type__upper_bound)
    {
      if (value <= abtentry->alpha)
      {
        abtentry->value = value;
        uo_engine_unlock_ttable();
        return true;
      }

      abtentry->hardalpha = -UO_SCORE_CHECKMATE;
      abtentry->hardbeta = value;
    }

    uo_engine_unlock_ttable();
    return false;
  }

  static inline int16_t uo_engine_store_entry(const uo_position *position, uo_abtentry *abtentry)
  {
    //if (abtentry->value < abtentry->hardalpha) abtentry->value = abtentry->hardalpha;
    //if (abtentry->value > abtentry->hardbeta) abtentry->value = abtentry->hardbeta;

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
      entry->value = uo_score_adjust_for_mate_to_ttable(abtentry->value);
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
        entry->value = uo_score_adjust_for_mate_to_ttable(abtentry->value);
        entry->type = type;
      }
      uo_engine_unlock_ttable();
    }

    return abtentry->value;
  }

  static inline void uo_engine_thread_lock(uo_engine_thread *thread)
  {
    uo_atomic_compare_exchange_wait(&thread->busy, 0, 1);
  }

  static inline void uo_engine_thread_unlock(uo_engine_thread *thread)
  {
    uo_atomic_store(&thread->busy, 0);
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

  void uo_search_queue_init(uo_search_queue *queue);

  bool uo_search_queue_try_enqueue(uo_search_queue *queue, uo_thread_function function, void *data);

  void uo_search_queue_post_result(uo_search_queue *queue, uo_search_queue_item *result);

  bool uo_search_queue_get_result(uo_search_queue *queue, uo_search_queue_item *result);

  bool uo_search_queue_try_get_result(uo_search_queue *queue, uo_search_queue_item *result);

  static inline void uo_engine_stop_search()
  {
    uo_atomic_store(&engine.stopped, 1);
  }

  uo_engine_thread *uo_engine_run_thread(uo_thread_function *function, void *data);

  uo_engine_thread *uo_engine_run_thread_if_available(uo_thread_function *function, void *data);

#ifdef __cplusplus
}
#endif

#endif
