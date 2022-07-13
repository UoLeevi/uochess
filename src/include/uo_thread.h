#ifndef UO_THREAD_H
#define UO_THREAD_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>


  // threads

  typedef struct uo_thread uo_thread;

  typedef void *uo_thread_function(void *arg);

  uo_thread *uo_thread_create(uo_thread_function function, void *arg);

  void *uo_thread_join(uo_thread *thread);


  // mutex

  typedef struct uo_mutex uo_mutex;

  uo_mutex *uo_mutex_create();

  void uo_mutex_destroy(uo_mutex *mutex);

  void uo_mutex_lock(uo_mutex *mutex);

  void uo_mutex_unlock(uo_mutex *mutex);

  // semaphore

  typedef struct uo_semaphore uo_semaphore;

  uo_semaphore *uo_semaphore_create(unsigned value);

  void uo_semaphore_destroy(uo_semaphore *semaphore);

  void uo_semaphore_wait(uo_semaphore *semaphore);

  void uo_semaphore_release(uo_semaphore *semaphore);


  // atomic operations

#ifdef WIN32
#include <Windows.h>
# define uo_atomic_int LONG
#else
# include <stdatomic.h>
# define uo_atomic_int atomic_int
#endif // WIN32 

  void uo_atomic_init(volatile uo_atomic_int *target, int value);

  bool uo_atomic_compare_exchange(volatile uo_atomic_int *target, int expected, int value);

  void uo_atomic_store(volatile uo_atomic_int *target, int value);

  // busy wait
  static inline void uo_atomic_compare_exchange_wait(volatile uo_atomic_int *target, int expected, int value)
  {
    while (!uo_atomic_compare_exchange(target, expected, value))
    {
      // noop
    }
  }


#ifdef __cplusplus
}
#endif

#endif
