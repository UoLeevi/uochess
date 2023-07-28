#ifndef UO_ENGINE_H
#define UO_ENGINE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "uo_position.h"
#include "uo_ttable.h"
#include "uo_book.h"
#include "uo_tb.h"
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
    size_t move_overhead;
    bool debug;
    bool use_own_book;
    char eval_filename[0x100];
    char book_filename[0x100];
    char nn_dir[0x100];
    char test_data_dir[0x100];
    char dataset_dir[0x100];
    struct
    {
      struct
      {
        char dir[0x100];
        size_t probe_depth;
        size_t probe_limit;
        bool rule50;
      } syzygy;
    } tb;
  } uo_engine_options;

  typedef struct uo_search_queue_item
  {
    uo_engine_thread *thread;
    uo_move move;
    int16_t value;
    size_t nodes;
    size_t depth;
    bool incomplete;
    uo_move *line;
  } uo_search_queue_item;

  typedef struct uo_search_queue {
    uo_atomic_flag busy;
    uo_atomic_int pending_count;
    uo_atomic_int count;
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
    uo_atomic_flag busy;
    uo_atomic_int cutoff;
    uint8_t nmp_min_ply;
    uo_move_cache move_cache[0x1000];
    uo_move pv[UO_MAX_PLY];
    uo_move **secondary_pvs;
  } uo_engine_thread;

  typedef struct uo_engine_thread_queue {
    uo_atomic_int count;
    uo_atomic_flag busy;
    int head;
    int tail;
    uo_engine_thread **threads;
  } uo_engine_thread_queue;

  typedef struct uo_engine
  {
    uo_tb tb;
    uo_ttable ttable;
    uo_book *book;
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
      bool is_continuation;
      bool is_ponderhit;
    } ponder;
    uo_move pv[UO_MAX_PLY];
    uo_move **secondary_pvs;
    volatile uo_atomic_int stopped;
    bool exit;
    struct
    {
      int argc;
      char **argv;
    } process_info;
    struct
    {
      char *name;
    } engine_info;
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

  static inline void uo_engine_clear_hash()
  {
    uo_ttable_clear(&engine.ttable);
  }

  static inline bool uo_engine_is_stopped()
  {
    return uo_atomic_load(&engine.stopped) == 1;
  }

  typedef struct uo_abtentry
  {
    int16_t *alpha;
    int16_t *beta;
    int8_t depth;
    uo_tdata data;
    uo_move bestmove;
    int16_t value;
    int16_t alpha_initial;
    int16_t beta_initial;
  } uo_abtentry;

  static inline void uo_engine_prefetch_entry(uint64_t key)
  {
    uint64_t mask = engine.ttable.hash_mask;
    uint64_t hash = key & mask;
    uo_tentry *entry = engine.ttable.entries + hash;
    uo_prefetch(entry);
  }

  // Retrieves an entry from the transposition table or from the opening book.
  // Returns true if an entry was found and the score represents valid score for the position when searched
  // on given depth and alpha/beta values.
  // Returns false if no entry was found or the score is not valid.
  // If an entry was found, the abtentry.data contains the values which were stored in the transposition table
  // even in the case that the score is not valid given search depth and alpha/beta values.
  static inline bool uo_engine_lookup_entry(uo_position *position, uo_abtentry *abtentry)
  {
    // Step 1. Save initial alpha-beta boundaries
    abtentry->alpha_initial = *abtentry->alpha;
    abtentry->beta_initial = *abtentry->beta;

    // Step 2. Probe opening book if enabled
    if (engine.book)
    {
      const uo_book_entry *book_entry = uo_book_get(engine.book, position);

      if (book_entry
        && book_entry->bestmove
        && uo_position_is_legal_move(position, book_entry->bestmove))
      {
        int16_t value = uo_score_adjust_from_ttable(position->ply, book_entry->value);
        abtentry->value = abtentry->data.value = value;
        abtentry->bestmove = abtentry->data.bestmove = book_entry->bestmove;
        abtentry->depth = abtentry->data.depth = book_entry->depth;
        abtentry->data.type = uo_score_type__exact;
        return true;
      }
    }

    // Step 3. Probe transposition table
    bool found = uo_ttable_get(&engine.ttable, position->key, position->root_ply, &abtentry->data);

    // Step 4. Return if no match was found
    if (!found) return false;

    abtentry->bestmove = abtentry->data.bestmove;
    assert(!abtentry->bestmove || uo_position_is_legal_move(position, abtentry->bestmove));

    // Step 5. Adjust mate-in and table base scores by accounting plies
    int16_t value = abtentry->data.value = uo_score_adjust_from_ttable(position->ply, abtentry->data.value);

    // Step 6. Return if transposition table entry from more shallow depth search
    if (abtentry->data.depth < abtentry->depth)
    {
      return false;
    }

    switch (abtentry->data.type)
    {
      // Step 7. In case the transposition table entry value is exact, return value.
      case uo_score_type__exact:
        abtentry->value = value;
        return true;

      case uo_score_type__lower_bound:
        // Step 8. Beta cutoff
        if (value >= abtentry->beta_initial)
        {
          abtentry->value = value;
          return true;
        }
        else if (position->ply
          && value > abtentry->alpha_initial)
        {
          // Step 9. Update alpha
          abtentry->alpha_initial = value;
          *abtentry->alpha = value; // - 1;
        }

        break;

      case uo_score_type__upper_bound:
        // Step 10. Beta cutoff, based on alpha value
        if (value <= abtentry->alpha_initial)
        {
          abtentry->value = value;
          return true;
        }
        else if (position->ply
          && value < abtentry->beta_initial)
        {
          // Step 11. Update beta
          abtentry->beta_initial = value;
          *abtentry->beta = value; // + 1;
        }

        break;
    }

    // Step 12. Return false to indicate that search should be continued
    return false;
  }

  // Stores an entry to the transposition table and return final score.
  static inline int16_t uo_engine_store_entry(const uo_position *position, uo_abtentry *abtentry)
  {
    // If existing transposition table entry was result of lower depth search. Update the entry.
    if (abtentry->data.depth < abtentry->depth
      || (abtentry->data.depth == abtentry->depth && abtentry->data.type != uo_score_type__exact))
    {
      abtentry->data.depth = abtentry->depth;
      abtentry->data.bestmove = abtentry->bestmove;
      abtentry->data.value = uo_score_adjust_to_ttable(position->ply, abtentry->value);
      abtentry->data.type =
        abtentry->value >= abtentry->beta_initial ? uo_score_type__lower_bound :
        abtentry->value <= abtentry->alpha_initial ? uo_score_type__upper_bound :
        uo_score_type__exact;

      uo_ttable_set(&engine.ttable, position->key, position->root_ply, &abtentry->data);
    }

    // if transposition table entry is from deeper search. Adjust value based on value bounds from previous search
    else if (abtentry->data.type == uo_score_type__lower_bound)
    {
      abtentry->value = uo_max(abtentry->data.value, abtentry->value);
    }
    else if (abtentry->data.type == uo_score_type__upper_bound)
    {
      abtentry->value = uo_min(abtentry->data.value, abtentry->value);
    }

    return abtentry->value;
  }

  static inline void uo_engine_thread_lock(uo_engine_thread *thread)
  {
    uo_atomic_lock(&thread->busy);
  }

  static inline void uo_engine_thread_unlock(uo_engine_thread *thread)
  {
    uo_atomic_unlock(&thread->busy);
  }

  static inline bool uo_engine_thread_is_stopped(uo_engine_thread *thread)
  {
    return uo_engine_is_stopped() || (thread->owner && uo_atomic_load(&thread->cutoff));
  }

  void uo_engine_start_search(void);

  static inline void uo_engine_reset_search_params(uint8_t seach_type)
  {
    assert(uo_engine_is_stopped());
    engine.search_params = (uo_search_params){
      .seach_type = seach_type,
      .depth = UO_MAX_PLY,
      .alpha = -uo_score_checkmate,
      .beta = uo_score_checkmate
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

  uo_process *uo_engine_start_new_process(char *cmdline);

#ifdef __cplusplus
}
#endif

#endif
