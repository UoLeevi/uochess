#include "uo_misc.h"

#include <stddef.h>

uo_file_stream *uo_file_stream_open_read(const char *filepath, size_t buf_size, char error_message[256])
{
  FILE *fp = fopen(filepath, "r");
  if (!fp)
  {
    if (error_message)
    {
      sprintf(error_message, "Cannot open file.");
    }

    return NULL;
  }

  uo_file_stream *file_stream = NULL;

  if (buf_size == 0)
  {
    // default buffer size
    buf_size = 0x1000;
  }

  file_stream = malloc(sizeof * file_stream + buf_size);
  file_stream->buf = ((char *)(void *)file_stream) + sizeof * file_stream;
  file_stream->buf_size = buf_size;
  file_stream->fp = fp;

  return file_stream;
}

void uo_file_stream_close(uo_file_stream *file_stream)
{
  fclose(file_stream->fp);
  free(file_stream);
}
