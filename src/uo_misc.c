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
    FILE_MAP_READ | FILE_MAP_COPY, // dwDesiredAccess
    0,                             // dwFileOffsetHigh
    0,                             // dwFileOffsetLow
    0);                            // dwNumberOfBytesToMap

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

#endif
