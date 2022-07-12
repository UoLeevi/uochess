#ifndef UO_THREAD_H
#define UO_THREAD_H

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct uo_thread uo_thread;

  typedef void *uo_thread_function(void *arg);

  uo_thread *uo_thread_create(uo_thread_function function, void *arg);

  void *uo_thread_join(uo_thread *thread);

#ifdef __cplusplus
}
#endif

#endif
