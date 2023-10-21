#include "uo_engine.h"

#include <stddef.h>
#include <stdlib.h>

uo_engine_options engine_options;
uo_engine engine;

void uo_engine_load_default_options()
{
  char *envopt;

  engine_options.multipv = 1;
  envopt = getenv("UO_OPT_MULTIPV");
  if (envopt)
  {
    size_t multipv = strtoul(envopt, NULL, 10);
    if (multipv > 0)
    {
      engine_options.multipv = multipv;
    }
  }

  engine_options.threads = 4;
  envopt = getenv("UO_OPT_THREADS");
  if (envopt)
  {
    size_t threads = strtoul(envopt, NULL, 10);
    if (threads > 0)
    {
      engine_options.threads = threads;
    }
  }

  engine_options.hash_size = 256;
  envopt = getenv("UO_OPT_HASH");
  if (envopt)
  {
    size_t hash_size = strtoul(envopt, NULL, 10);
    if (hash_size > 0)
    {
      engine_options.hash_size = hash_size;
    }
  }

  engine_options.move_overhead = 10;

  engine_options.use_own_book = true;

  strcpy(engine_options.book_filename, "books/default-book.txt");
  envopt = getenv("UO_OPT_BOOKFILE");
  if (envopt)
  {
    strcpy(engine_options.book_filename, envopt);
  }

  engine_options.tb.syzygy.probe_depth = 1;
  engine_options.tb.syzygy.rule50 = true;
  engine_options.tb.syzygy.probe_limit = 7;

  strcpy(engine_options.tb.syzygy.dir, "syzygy_tables");
  envopt = getenv("UO_OPT_SYGYZYPATH");
  if (envopt)
  {
    strcpy(engine_options.tb.syzygy.dir, envopt);
  }

  strcpy(engine_options.eval_filename, "nn/nn-eval-test.pt");
  envopt = getenv("UO_OPT_EVALFILE");
  if (envopt)
  {
    strcpy(engine_options.eval_filename, envopt);
  }

  envopt = getenv("UO_OPT_NNDIR");
  if (envopt)
  {
    strcpy(engine_options.nn_dir, envopt);
  }

  envopt = getenv("UO_OPT_TESTDATADIR");
  if (envopt)
  {
    strcpy(engine_options.test_data_dir, envopt);
  }

  envopt = getenv("UO_OPT_DATASETDIR");
  if (envopt)
  {
    strcpy(engine_options.dataset_dir, envopt);
  }
}

uo_engine_thread *uo_engine_run_thread(uo_thread_function *function, void *data)
{
  uo_engine_thread *thread;

  while (!uo_atomic_queue_try_dequeue(&engine.thread_queue, (void **)&thread))
  {
    // Queue is empty, wait
  }

  thread->function = function;
  thread->data = data;
  uo_atomic_lock(&thread->busy);
  uo_semaphore_release(thread->semaphore);
  return thread;
}

uo_engine_thread *uo_engine_run_thread_if_available(uo_thread_function *function, void *data)
{
  uo_engine_thread *thread;

  if (!uo_atomic_queue_try_dequeue(&engine.thread_queue, (void **)&thread))
  {
    // Queue is empty
    return NULL;
  }

  thread->function = function;
  thread->data = data;
  uo_atomic_lock(&thread->busy);
  uo_semaphore_release(thread->semaphore);

  return thread;
}

void *uo_engine_thread_run(void *arg)
{
  uo_engine_thread *thread = arg;
  void *thread_return = NULL;

  while (!engine.exit)
  {
    while (!uo_atomic_queue_try_enqueue(&engine.thread_queue, thread))
    {
      if (engine.exit) return NULL;
      // Queue is full, wait
    }

    uo_semaphore_wait(thread->semaphore);
    uo_atomic_store(&thread->cutoff, 0);
    thread->owner = NULL;
    thread_return = thread->function(thread);
  }

  return thread_return;
}

void uo_engine_init()
{
  // stopped flag
  uo_atomic_init(&engine.stopped, 1);

  // position mutex
  engine.position_mutex = uo_mutex_create();

  // stdout_mutex
  engine.stdout_mutex = uo_mutex_create();

  // threads
  engine.thread_count = engine_options.threads + 1; // one additional thread for timer
  engine.threads = calloc(engine.thread_count, sizeof(uo_engine_thread));

  // thread_queue
  uo_atomic_queue_init(&engine.thread_queue, engine.thread_count, NULL);

  // search_queue
  uo_atomic_queue_init(&engine.search_queue, UO_PARALLEL_MAX_COUNT, NULL);

  // nn eval_filename
  char *eval_filename = engine_options.eval_filename[0] ? engine_options.eval_filename : NULL;

  for (size_t i = 0; i < engine.thread_count; ++i)
  {
    uo_engine_thread *thread = engine.threads + i;
    thread->semaphore = uo_semaphore_create(0);
    uo_atomic_flag_init(&thread->busy);
    uo_atomic_init(&thread->cutoff, 0);
    thread->index = i;
    thread->thread = uo_thread_create(uo_engine_thread_run, thread);
  }

  // hash table
  size_t capacity = engine_options.hash_size * (size_t)1000000 / sizeof * engine.ttable.entries;
  uo_ttable_init(&engine.ttable, uo_msb(capacity) + 1);

  // opening book
  if (engine_options.use_own_book && *engine_options.book_filename)
  {
    engine.book = uo_book_create(engine_options.book_filename);
  }

  // syzygy
  engine.tb.enabled = *engine_options.tb.syzygy.dir != '\0';
  if (engine.tb.enabled)
  {
    strcpy(engine.tb.dir, engine_options.tb.syzygy.dir);
    engine.tb.probe_limit = engine_options.tb.syzygy.probe_limit;
    engine.tb.probe_depth = engine_options.tb.syzygy.probe_depth;
    engine.tb.rule50 = engine_options.tb.syzygy.rule50;
    uo_tb_init(&engine.tb);
  }

  // multipv
  if (engine_options.multipv)
  {
    engine.secondary_pvs = calloc(engine_options.multipv, sizeof engine.pv);

    for (size_t i = 0; i < engine.thread_count; ++i)
    {
      engine.threads[i].info.secondary_pvs = calloc(engine_options.multipv, sizeof engine.pv);
    }
  }

  // load startpos
  uo_position_from_fen(&engine.position, uo_fen_startpos);
}

void uo_engine_reconfigure()
{
  // threads
  if (engine_options.threads + 1 != engine.thread_count)
  {
    // free previous threads
    for (size_t i = 0; i < engine.thread_count; ++i)
    {
      uo_engine_thread *thread = engine.threads + i;
      uo_thread_terminate(thread->thread);
      uo_semaphore_destroy(thread->semaphore);
    }

    // threads
    engine.thread_count = engine_options.threads + 1; // one additional thread for timer
    free(engine.threads);
    engine.threads = calloc(engine.thread_count, sizeof(uo_engine_thread));

    // thread_queue
    free(engine.thread_queue.items);
    uo_atomic_queue_init(&engine.thread_queue, engine.thread_count, NULL);

    for (size_t i = 0; i < engine.thread_count; ++i)
    {
      uo_engine_thread *thread = engine.threads + i;
      thread->semaphore = uo_semaphore_create(0);
      uo_atomic_flag_init(&thread->busy);
      uo_atomic_init(&thread->cutoff, 0);
      thread->index = i;
      thread->thread = uo_thread_create(uo_engine_thread_run, thread);
    }

    // multipv
    if (engine_options.multipv)
    {
      free(engine.secondary_pvs);
      engine.secondary_pvs = calloc(engine_options.multipv, sizeof engine.pv);

      for (size_t i = 0; i < engine.thread_count; ++i)
      {
        engine.threads[i].info.secondary_pvs = calloc(engine_options.multipv, sizeof engine.pv);
      }
    }
  }

  // hash table
  uo_ttable_free(&engine.ttable);
  size_t capacity = engine_options.hash_size * (size_t)1000000 / sizeof * engine.ttable.entries;
  uo_ttable_init(&engine.ttable, uo_msb(capacity) + 1);

  // opening book
  if (engine.book)
  {
    uo_book_free(engine.book);
  }

  if (engine_options.use_own_book && *engine_options.book_filename)
  {
    engine.book = uo_book_create(engine_options.book_filename);
  }
}

static uo_thread_function *uo_search_thread_run_function[] = {
  [uo_seach_type__principal_variation] = uo_engine_thread_run_principal_variation_search,
  [uo_seach_type__quiescence] = uo_engine_thread_run_quiescence_search
};

void uo_engine_start_search()
{
  uo_atomic_store(&engine.stopped, 0);
  uo_engine_run_thread(uo_search_thread_run_function[engine.search_params.seach_type], NULL);
}

uo_process *uo_engine_start_new_process(char *cmdline)
{
  if (!cmdline) cmdline = engine.process_info.argv[0];
  return uo_process_create(cmdline);
}
