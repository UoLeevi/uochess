#ifndef UO_MISC_H
#define UO_MISC_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <time.h>

  typedef struct timespec uo_time;

  static inline void uo_time_now(uo_time *time)
  {
    timespec_get(time, TIME_UTC);
  }

  static inline double uo_time_diff_msec(const uo_time *start, const uo_time *end)
  {
    double time_msec = difftime(end->tv_sec, start->tv_sec) * 1000.0;
    time_msec += (end->tv_nsec - start->tv_nsec) / 1000000.0;
    return time_msec;
  }

  static inline double uo_time_elapsed_msec(const uo_time *time)
  {
    uo_time time_now;
    uo_time_now(&time_now);
    return uo_time_diff_msec(time, &time_now);
  }

#ifdef __cplusplus
}
#endif

#endif
