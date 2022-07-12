#include "uo_thread.h"

#include <stdlib.h>

#ifdef WIN32

#include <windows.h>

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
  uo_thread *thread = malloc(sizeof thread);
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
  void *thread_return = thread->data;
  free(thread);
  return thread_return;
}

#else

#include <pthread.h>

typedef struct uo_thread
{
  pthread_t pthread;
} uo_thread;

uo_thread *uo_thread_create(uo_thread_function function, void *arg)
{
  uo_thread *thread = malloc(sizeof thread);
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
  pthread_join(&thread->pthread, thread_return);
}

#endif
