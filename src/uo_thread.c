#include "uo_thread.h"

#include <stdlib.h>
#include <inttypes.h>

#ifdef WIN32

#include <Windows.h>

// threads

typedef struct uo_thread
{
  HANDLE handle;
  DWORD dwThreadId;
  uo_thread_function *function;
  void *data;
} uo_thread;

DWORD WINAPI uo_thread_start(LPVOID lpParam)
{
  uo_thread *thread = lpParam;
  thread->data = thread->function(thread->data);
  return 0;
}

uo_thread *uo_thread_create(uo_thread_function function, void *arg)
{
  uo_thread *thread = malloc(sizeof * thread);
  if (!thread) return NULL;

  thread->function = function;
  thread->data = arg;

  thread->handle = CreateThread(
    NULL,                   // default security attributes
    0,                      // use default stack size  
    uo_thread_start,        // thread function name
    thread,                 // argument to thread function 
    0,                      // use default creation flags 
    &thread->dwThreadId);   // returns the thread identifier

  if (!thread->handle)
  {
    free(thread);
    return NULL;
  }

  return thread;
}

void *uo_thread_join(uo_thread *thread)
{
  WaitForSingleObject(thread->handle, INFINITE);
  CloseHandle(thread->handle);
  void *thread_return = thread->data;
  free(thread);
  return thread_return;
}


void *uo_thread_terminate(uo_thread *thread)
{
  TerminateThread(thread->handle, 0);
  return uo_thread_join(thread);
}

// mutex

typedef struct uo_mutex
{
  HANDLE handle;
} uo_mutex;

uo_mutex *uo_mutex_create()
{
  uo_mutex *mutex = malloc(sizeof * mutex);
  mutex->handle = CreateMutex(
    NULL,              // default security attributes
    FALSE,             // initially not owned
    NULL);             // unnamed mutex

  return mutex;
}

void uo_mutex_destroy(uo_mutex *mutex)
{
  CloseHandle(mutex->handle);
  free(mutex);
}
void uo_mutex_lock(uo_mutex *mutex)
{
  WaitForSingleObject(mutex->handle, INFINITE);
}

void uo_mutex_unlock(uo_mutex *mutex)
{
  ReleaseMutex(mutex->handle);
}


// semaphore

typedef struct uo_semaphore
{
  HANDLE handle;
} uo_semaphore;

uo_semaphore *uo_semaphore_create(unsigned value)
{
  uo_semaphore *semaphore = malloc(sizeof * semaphore);
  semaphore->handle = CreateSemaphore(
    NULL,           // default security attributes
    value,          // initial count
    0x100,          // maximum count
    NULL);          // unnamed semaphore
  return semaphore;
}

void uo_semaphore_destroy(uo_semaphore *semaphore)
{
  CloseHandle(semaphore->handle);
  free(semaphore);
}

void uo_semaphore_wait(uo_semaphore *semaphore)
{
  WaitForSingleObject(semaphore->handle, INFINITE);
}

void uo_semaphore_release(uo_semaphore *semaphore)
{
  ReleaseSemaphore(semaphore->handle, 1, NULL);
}

// atomic operations

void uo_atomic_init(volatile uo_atomic_int *target, int value)
{
  InterlockedExchange(target, value);
}

int uo_atomic_compare_exchange(volatile uo_atomic_int *target, int expected, int value)
{
  return InterlockedCompareExchange(target, value, expected);
}

int uo_atomic_load(volatile uo_atomic_int *target)
{
  return *target;
  //return InterlockedCompareExchange(target, 0, 0);
}

void uo_atomic_store(volatile uo_atomic_int *target, int value)
{
  InterlockedExchange(target, value);
}

int uo_atomic_increment(volatile uo_atomic_int *target)
{
  return InterlockedIncrement(target);
}

int uo_atomic_decrement(volatile uo_atomic_int *target)
{
  return InterlockedDecrement(target);
}

int uo_atomic_add(volatile uo_atomic_int *target, int value)
{
  return InterlockedAdd(target, value);
}

int uo_atomic_sub(volatile uo_atomic_int *target, int value)
{
  return InterlockedAdd(target, -value);
}

#else

#include <semaphore.h>
#include <pthread.h>
#include <stdatomic.h>

// threads

typedef struct uo_thread
{
  pthread_t pthread;
} uo_thread;

uo_thread *uo_thread_create(uo_thread_function function, void *arg)
{
  uo_thread *thread = malloc(sizeof * thread);
  if (!thread) return NULL;

  int ret = pthread_create(&thread->pthread, 0, function, arg);
  if (ret != 0)
  {
    free(thread);
    return NULL;
  }

  return thread;
}

void *uo_thread_join(uo_thread *thread)
{
  void *thread_return;
  pthread_join(&thread->pthread, &thread_return);
  free(thread);
  return thread_return;
}

void *uo_thread_terminate(uo_thread *thread)
{
  pthread_cancel(&thread->pthread);
  return uo_thread_join(thread);
}

// mutex

typedef struct uo_mutex
{
  pthread_mutex_t mtx;
} uo_mutex;

uo_mutex *uo_mutex_create()
{
  uo_mutex *mutex = malloc(sizeof * mutex);
  pthread_mutex_init(&mutex->mtx, 0);
  return mutex;
}

void uo_mutex_destroy(uo_mutex *mutex)
{
  pthread_mutex_destroy(&mutex->mtx);
  free(mutex);
}
void uo_mutex_lock(uo_mutex *mutex)
{
  pthread_mutex_lock(&mutex->mtx);
}

void uo_mutex_unlock(uo_mutex *mutex)
{
  pthread_mutex_unlock(&mutex->mtx);
}

// semaphore

typedef struct uo_semaphore
{
  sem_t sem;
} uo_semaphore;

uo_semaphore *uo_semaphore_create(unsigned value)
{
  uo_semaphore *semaphore = malloc(sizeof * semaphore);
  sem_init(&semaphore->sem, 0, value);
  return semaphore;
}

void uo_semaphore_destroy(uo_semaphore *semaphore)
{
  sem_destroy(&semaphore->sem);
  free(semaphore);
}

void uo_semaphore_wait(uo_semaphore *semaphore)
{
  sem_wait(&semaphore->sem);
}

void uo_semaphore_release(uo_semaphore *semaphore)
{
  sem_post(&semaphore->sem);
}

// atomic operations

void uo_atomic_init(volatile uo_atomic_int *target, int value)
{
  atomic_init(target, value);
}

int uo_atomic_compare_exchange(volatile uo_atomic_int *target, int expected, int value)
{
  return atomic_compare_exchange_strong(target, &expected, value) ? expected : atomic_load(target);
}

int uo_atomic_load(volatile uo_atomic_int *target)
{
  return atomic_load(target);
}

void uo_atomic_store(volatile uo_atomic_int *target, int value)
{
  atomic_store(target, value);
}

int uo_atomic_add(volatile uo_atomic_int *target, int value)
{
  return atomic_fetch_add(target, value) + value;
}

int uo_atomic_sub(volatile uo_atomic_int *target, int value)
{
  return atomic_fetch_sub(target, value) - value;
}

int uo_atomic_increment(volatile uo_atomic_int *target)
{
  return atomic_fetch_add(target, 1) + 1;
}

int uo_atomic_decrement(volatile uo_atomic_int *target)
{
  return atomic_fetch_sub(target, 1) - 1;
}

#endif
