#include "uo_engine.h"
#include "uo_position.h"
#include "uo_evaluation.h"
#include "uo_thread.h"
#include "uo_global.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>
#include <assert.h>

void uo_engine_queue_work(uo_engine *engine, uo_thread_function *function, void *data)
{
  uo_engine_work_queue *work_queue = &engine->work_queue;
  uo_mutex_lock(work_queue->mutex);
  work_queue->work[work_queue->head] = (uo_engine_thread_work){ .function = function, .data = data };
  work_queue->head = (work_queue->head + 1) % uo_engine_work_queue_max_count;
  uo_mutex_unlock(work_queue->mutex);
  uo_semaphore_release(work_queue->semaphore);
}

void *uo_engine_thread_run(void *arg)
{
  uo_engine_thread *thread = arg;
  uo_engine_work_queue *work_queue = &thread->engine->work_queue;
  void *thread_return = NULL;

  while (!thread->engine->exit)
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

void uo_engine_init(uo_engine *engine, uo_engine_init_options *options)
{
  // stop flag
  uo_atomic_init(&engine->stop, 1);

  // position mutex
  engine->position_mutex = uo_mutex_create();

  // stdout_mutex
  engine->stdout_mutex = uo_mutex_create();

  // work_queue
  engine->work_queue.semaphore = uo_semaphore_create(0);
  engine->work_queue.mutex = uo_mutex_create();
  engine->work_queue.head = 0;
  engine->work_queue.tail = 0;

  // threads
  engine->thread_count = options->threads;
  engine->threads = calloc(engine->thread_count, sizeof * engine->threads);

  for (size_t i = 0; i < engine->thread_count; ++i)
  {
    uo_engine_thread *thread = engine->threads + i;
    thread->engine = engine;
    thread->thread = uo_thread_create(uo_engine_thread_run, thread);
  }

  // hash table
  size_t capacity = options->hash_size / sizeof * engine->ttable.entries;
  uo_ttable_init(&engine->ttable, uo_msb(capacity) + 1);

  // multipv
  engine->pv = malloc(options->multipv * sizeof * engine->pv);
}

void uo_engine_start_search(uo_engine *engine, uo_search_params *params)
{
  void *data = malloc(sizeof * params);
  memcpy(data, params, sizeof * params);
  uo_engine_queue_work(engine, uo_engine_thread_run_negamax_search, data);
}
