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

  typedef struct uo_time
  {
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
  } uo_time;

  static inline void uo_time_now(uo_time *time)
  {
    QueryPerformanceCounter(&time->counter);
    QueryPerformanceFrequency(&time->frequency);
  }

  static inline double uo_time_diff_msec(const uo_time *start, const uo_time *end)
  {
    return (end->counter.QuadPart - start->counter.QuadPart) * 1000.0 / start->frequency.QuadPart;
  }

  static inline double uo_time_elapsed_msec(const uo_time *time)
  {
    uo_time time_now;
    QueryPerformanceCounter(&time_now.counter);
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

  static inline char *uo_line_arg_parse(const char *line, const char *argname, int argvalue_index, char **end)
  {
    if (!line) return NULL;
    char *start = strstr(line, argname);
    if (!start) return NULL;

    *end = strchr(start, ' ');

    while (argvalue_index--)
    {
      if (!*end) return NULL;

      start = *end + 1;

      while (isspace(*start))
      {
        ++start;
      }

      *end = strchr(start, ' ');
    }

    return start;
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

  size_t uo_pipe_read(uo_pipe *pipe, char *buffer, size_t len);
  size_t uo_pipe_write(uo_pipe *pipe, const char *ptr, size_t len);


  // Processes

  typedef struct uo_process uo_process;

  uo_process *uo_process_create(char *cmdline);
  void uo_process_free(uo_process *process);
  size_t uo_process_write_stdin(uo_process *process, const char *ptr, size_t len);
  size_t uo_process_read_stdout(uo_process *process, char *buffer, size_t len);


  // File paths

  static inline char *uo_filename_from_path(char *path)
  {
    char *slash = strrchr(path, '/');
    char *backslash = strrchr(path, '\\');

    return
      slash > backslash ? slash + 1 :
      backslash == NULL ? NULL :
      backslash + 1;
  }

  static inline char *uo_file_extension_from_path(char *path)
  {
    return strrchr(path, '.');
  }

  // Cache prefetch hint

  static inline void uo_prefetch(void *addr)
  {
#   if defined(__INTEL_COMPILER)
    __asm__("");
#   endif

#   if defined(__INTEL_COMPILER) || defined(_MSC_VER)
    _mm_prefetch((char *)addr, _MM_HINT_T0);
#   else
    __builtin_prefetch(addr);
#   endif
  }

#ifdef __cplusplus
}
#endif

#endif
