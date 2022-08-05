#ifndef UO_THREAD_H
#define UO_THREAD_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdatomic.h>

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
#include <stdatomic.h>

# define uo_atomic_int atomic_int
#endif // WIN32 

  void uo_atomic_init(volatile uo_atomic_int *target, int value);

  int uo_atomic_compare_exchange(volatile uo_atomic_int *target, int expected, int value);

  void uo_atomic_store(volatile uo_atomic_int *target, int value);

  int uo_atomic_load(volatile uo_atomic_int *target);

  int uo_atomic_increment(volatile uo_atomic_int *target);

  int uo_atomic_decrement(volatile uo_atomic_int *target);

# define uo_atomic_flag atomic_flag

# define uo_atomic_flag_clear atomic_flag_clear
# define uo_atomic_flag_test_and_set atomic_flag_test_and_set

# define uo_atomic_flag_init uo_atomic_flag_clear

  // busy wait
  static inline void uo_atomic_compare_exchange_wait(volatile uo_atomic_int *target, int expected, int value)
  {
    while (uo_atomic_compare_exchange(target, expected, value) != expected)
    {
      // noop
    }
  }

  static inline void uo_atomic_wait_until(volatile uo_atomic_int *target, int expected)
  {
    while (uo_atomic_compare_exchange(target, expected, expected) != expected)
    {
      // noop
    }
  }

  static inline void uo_atomic_wait_until_lte(volatile uo_atomic_int *target, int expected)
  {
    while (uo_atomic_compare_exchange(target, expected, expected) > expected)
    {
      // noop
    }
  }

  static inline void uo_atomic_wait_until_gte(volatile uo_atomic_int *target, int expected)
  {
    while (uo_atomic_compare_exchange(target, expected, expected) < expected)
    {
      // noop
    }
  }

  static inline void uo_atomic_lock(uo_atomic_flag *target)
  {
    while (uo_atomic_flag_test_and_set(target))
    {
      // noop
    }
  }

# define uo_atomic_unlock uo_atomic_flag_clear

#ifdef __cplusplus
}
#endif

#endif
