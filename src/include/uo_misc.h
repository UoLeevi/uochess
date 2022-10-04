#ifndef UO_MISC_H
#define UO_MISC_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef WIN32
# include <Windows.h>
#endif // WIN32

  // Time functions

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

#ifdef WIN32
# define uo_sleep_msec Sleep
#else
  static inline void uo_sleep_msec(uint64_t msec)
  {
    struct timespec rem;
    struct timespec req = { .tv_sec = msec / 1000, tv_nsec = (msec % 1000) * 1000000 };
    nanosleep(&req, &rem);
  }
#endif

  // Command line argument parsing

  // helper for parsing named command line arguments
  static inline char *uo_arg_parse(int argc, char **argv, const char *argname, int argvalue_index)
  {
    for (size_t i = 1; i < argc - argvalue_index; i++)
    {
      if (strcmp(argv[i], argname) == 0)
      {
        return argv[i + argvalue_index];
      }
    }

    return NULL;
  }

  // Memory-Mapped Files
  // see: https://docs.microsoft.com/en-us/previous-versions/ms810613(v=msdn.10)

  typedef struct uo_file_mmap_handle uo_file_mmap_handle;
  typedef struct uo_file_mmap
  {
    uo_file_mmap_handle *handle;
    char *ptr;
    char *line;
    size_t size;
  } uo_file_mmap;

  uo_file_mmap *uo_file_mmap_open_read(const char *filepath);

  void uo_file_mmap_close(uo_file_mmap *file_mmap);

  char *uo_file_mmap_readline(uo_file_mmap *file_mmap);


  // Pipes

  typedef struct uo_pipe uo_pipe;

  uo_pipe *uo_pipe_create();
  void uo_pipe_close(uo_pipe *pipe);

  void uo_pipe_read(uo_pipe *pipe);
  void uo_pipe_write(uo_pipe *pipe);


  // Processes

  typedef struct uo_process uo_process;

  uo_process *uo_process_create(char *cmdline);
  void uo_process_free(uo_process *process);

#ifdef __cplusplus
}
#endif

#endif
