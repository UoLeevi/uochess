#include "uo_misc.h"

#include <stddef.h>

#ifdef WIN32

#include <Windows.h>

typedef struct uo_file_mmap_handle
{
  HANDLE hFile;
  HANDLE hMap;
  LPVOID lpBasePtr;
} uo_file_mmap_handle;

uo_file_mmap *uo_file_mmap_open_read(const char *filepath)
{
  HANDLE hFile;
  HANDLE hMap;
  LPVOID lpBasePtr;
  LARGE_INTEGER liFileSize;

  hFile = CreateFile(filepath,
    GENERIC_READ,                          // dwDesiredAccess
    0,                                     // dwShareMode
    NULL,                                  // lpSecurityAttributes
    OPEN_EXISTING,                         // dwCreationDisposition
    FILE_ATTRIBUTE_NORMAL,                 // dwFlagsAndAttributes
    0);                                    // hTemplateFile

  if (hFile == INVALID_HANDLE_VALUE)
  {
    return NULL;
  }

  if (!GetFileSizeEx(hFile, &liFileSize))
  {
    CloseHandle(hFile);
    return NULL;
  }

  if (liFileSize.QuadPart == 0)
  {
    CloseHandle(hFile);
    return NULL;
  }

  hMap = CreateFileMapping(
    hFile,
    NULL,                          // Mapping attributes
    PAGE_READONLY,                 // Protection flags
    0,                             // MaximumSizeHigh
    0,                             // MaximumSizeLow
    NULL);                         // Name

  if (hMap == 0)
  {
    CloseHandle(hFile);
    return NULL;
  }

  lpBasePtr = MapViewOfFile(
    hMap,
    FILE_MAP_COPY, // dwDesiredAccess, see: https://stackoverflow.com/a/55020707
    0,             // dwFileOffsetHigh
    0,             // dwFileOffsetLow
    0);            // dwNumberOfBytesToMap

  if (lpBasePtr == NULL)
  {
    CloseHandle(hMap);
    CloseHandle(hFile);
    return NULL;
  }

  uo_file_mmap *file_mmap = malloc(sizeof * file_mmap + sizeof * file_mmap->handle);
  file_mmap->handle = (void *)(((char *)(void *)file_mmap) + sizeof * file_mmap);
  file_mmap->size = liFileSize.QuadPart;
  file_mmap->ptr = lpBasePtr;
  file_mmap->line = lpBasePtr;
  file_mmap->handle->hFile = hFile;
  file_mmap->handle->hMap = hMap;
  file_mmap->handle->lpBasePtr = lpBasePtr;

  return file_mmap;
}

void uo_file_mmap_close(uo_file_mmap *file_mmap)
{
  UnmapViewOfFile(file_mmap->handle->lpBasePtr);
  CloseHandle(file_mmap->handle->hMap);
  CloseHandle(file_mmap->handle->hFile);
  free(file_mmap);
}

char *uo_file_mmap_readline(uo_file_mmap *file_mmap)
{
  if (!file_mmap->line) return NULL;

  char *line = file_mmap->line;
  char *brk = strpbrk(file_mmap->line, "\r\n");

  if (brk)
  {
    if (*brk == '\r')
    {
      *brk++ = '\0';
    }

    if (*brk == '\n')
    {
      *brk++ = '\0';
    }

    file_mmap->line = brk;
  }
  else
  {
    file_mmap->line = NULL;
  }

  return line;
}

// Pipes

typedef struct uo_pipe
{
  HANDLE rd;
  HANDLE wr;
  SECURITY_ATTRIBUTES saAttr;
} uo_pipe;

uo_pipe *uo_pipe_create()
{
  uo_pipe *pipe = malloc(sizeof *pipe);

  // Set the bInheritHandle flag so pipe handles are inherited. 

  pipe->saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  pipe->saAttr.bInheritHandle = TRUE;
  pipe->saAttr.lpSecurityDescriptor = NULL;

  // Create a pipe

  if (!CreatePipe(&pipe->rd, &pipe->wr, &pipe->saAttr, 0))
  {
    free(pipe);
    return NULL;
  }

  return pipe;
}

void uo_pipe_close(uo_pipe *pipe)
{
  CloseHandle(pipe->rd);
  CloseHandle(pipe->wr);
  free(pipe);
}

void uo_pipe_read(uo_pipe *pipe);
void uo_pipe_write(uo_pipe *pipe);


// Processes

typedef struct uo_process
{

} uo_process;

uo_process *uo_process_create();
void uo_process_free(uo_process *process);

#endif
