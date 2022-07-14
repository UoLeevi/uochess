#include "uo_engine.h"

#include <stddef.h>

uo_engine_options engine_options;
uo_engine engine;

void uo_engine_load_default_options()
{
  engine_options.multipv = 1;
  engine_options.threads = 1;

  size_t capacity = (size_t)16000000 / sizeof * engine.ttable.entries;
  capacity = (size_t)1 << uo_msb(capacity);
  engine_options.hash_size = capacity * sizeof * engine.ttable.entries;
}

void uo_engine_queue_work(uo_thread_function *function, void *data)
{
  uo_engine_work_queue *work_queue = &engine.work_queue;
  uo_mutex_lock(work_queue->mutex);
  work_queue->work[work_queue->head] = (uo_engine_thread_work){ .function = function, .data = data };
  work_queue->head = (work_queue->head + 1) % uo_engine_work_queue_max_count;
  uo_mutex_unlock(work_queue->mutex);
  uo_semaphore_release(work_queue->semaphore);
}

void *uo_engine_thread_run(void *arg)
{
  uo_engine_thread *thread = arg;
  uo_engine_work_queue *work_queue = &engine.work_queue;
  void *thread_return = NULL;

  while (!engine.exit)
  {
    uo_semaphore_wait(work_queue->semaphore);
    uo_mutex_lock(work_queue->mutex);
    uo_engine_thread_work work = work_queue->work[work_queue->tail];
    work_queue->tail = (work_queue->tail + 1) % uo_engine_work_queue_max_count;
    uo_mutex_unlock(work_queue->mutex);
    uo_thread_function *function = work.function;
    work.thread = thread;
    thread_return = function(&work);
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

  // work_queue
  engine.work_queue.semaphore = uo_semaphore_create(0);
  engine.work_queue.mutex = uo_mutex_create();
  engine.work_queue.head = 0;
  engine.work_queue.tail = 0;

  // threads
  engine.thread_count = engine_options.threads;
  engine.threads = calloc(engine.thread_count, sizeof * engine.threads);

  for (size_t i = 0; i < engine.thread_count; ++i)
  {
    uo_engine_thread *thread = engine.threads + i;
    engine = engine;
    thread->thread = uo_thread_create(uo_engine_thread_run, thread);
  }

  // hash table
  size_t capacity = engine_options.hash_size / sizeof * engine.ttable.entries;
  uo_ttable_init(&engine.ttable, uo_msb(capacity) + 1);

  // multipv
  engine.pv = malloc(engine_options.multipv * sizeof * engine.pv);

  // load startpos
  uo_position_from_fen(&engine.position, uo_fen_startpos);
}

void uo_engine_start_search(uo_search_params *params)
{
  void *data = malloc(sizeof * params);
  memcpy(data, params, sizeof * params);
  uo_atomic_store(&engine.stopped, 0);
  uo_engine_queue_work(uo_engine_thread_run_negamax_search, data);
}
