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
  uo_pipe *pipe = malloc(sizeof * pipe);

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

size_t uo_pipe_read(uo_pipe *pipe, char *buffer, size_t len)
{
  DWORD len_read;

  while (ReadFile(pipe->rd, buffer, len - 1, &len_read, NULL) != FALSE)
  {
    /* add terminating zero */
    buffer[len_read] = '\0';

    return len_read;
  }

  return 0;
}

size_t uo_pipe_write(uo_pipe *pipe, const char *ptr, size_t len)
{
  DWORD len_written;
  WriteFile(pipe->wr, ptr, len, &len_written, NULL);
  return len_written;
}


// Processes

typedef struct uo_process
{
  PROCESS_INFORMATION piProcInfo;
  uo_pipe *stdin_pipe;
  uo_pipe *stdout_pipe;
} uo_process;

// see: https://learn.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output
uo_process *uo_process_create(char *cmdline)
{
  uo_process *process = malloc(sizeof * process);
  process->stdin_pipe = uo_pipe_create();
  process->stdout_pipe = uo_pipe_create();

  // Ensure the read handle to the pipe for STDOUT is not inherited.

  if (!SetHandleInformation(process->stdout_pipe->rd, HANDLE_FLAG_INHERIT, 0))
  {
    uo_pipe_close(process->stdin_pipe);
    uo_pipe_close(process->stdout_pipe);
    free(process);
    return NULL;
  }

  // Ensure the write handle to the pipe for STDIN is not inherited. 

  if (!SetHandleInformation(process->stdin_pipe->wr, HANDLE_FLAG_INHERIT, 0))
  {
    uo_pipe_close(process->stdin_pipe);
    uo_pipe_close(process->stdout_pipe);
    free(process);
    return NULL;
  }

  // Set up members of the PROCESS_INFORMATION structure. 

  ZeroMemory(&process->piProcInfo, sizeof(PROCESS_INFORMATION));

  // Set up members of the STARTUPINFO structure. 
  // This structure specifies the STDIN and STDOUT handles for redirection.

  STARTUPINFO siStartInfo;
  ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
  siStartInfo.cb = sizeof(STARTUPINFO);
  siStartInfo.hStdError = process->stdout_pipe->wr;
  siStartInfo.hStdOutput = process->stdout_pipe->wr;
  siStartInfo.hStdInput = process->stdin_pipe->rd;
  siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

  // Create the child process. 

  bool bSuccess = CreateProcess(NULL,
    cmdline,       // command line 
    NULL,          // process security attributes 
    NULL,          // primary thread security attributes 
    TRUE,          // handles are inherited 
    0,             // creation flags 
    NULL,          // use parent's environment 
    NULL,          // use parent's current directory 
    &siStartInfo,  // STARTUPINFO pointer 
    &process->piProcInfo);  // receives PROCESS_INFORMATION 

  // If an error occurs, exit the application. 
  if (!bSuccess)
  {
    uo_pipe_close(process->stdin_pipe);
    uo_pipe_close(process->stdout_pipe);
    free(process);
    return NULL;
  }

  // Close handles to the stdin and stdout pipes no longer needed by the child process.
  // If they are not explicitly closed, there is no way to recognize that the child process has ended.

  CloseHandle(process->stdout_pipe->wr);
  CloseHandle(process->stdin_pipe->rd);

  return process;
}


void uo_process_free(uo_process *process)
{
  CloseHandle(process->piProcInfo.hProcess);
  CloseHandle(process->piProcInfo.hThread);
  CloseHandle(process->stdout_pipe->rd);
  CloseHandle(process->stdin_pipe->wr);
  free(process->stdin_pipe);
  free(process->stdout_pipe);
  free(process);
}

size_t uo_process_write_stdin(uo_process *process, const char *ptr, size_t len)
{
  if (!len) len = strlen(ptr);
  return uo_pipe_write(process->stdin_pipe, ptr, len);
}

size_t uo_process_read_stdout(uo_process *process, char *buffer, size_t len)
{
  return uo_pipe_read(process->stdout_pipe, buffer, len);
}

#endif
