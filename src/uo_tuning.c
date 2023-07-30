#include "uo_tuning.h"
#include "uo_misc.h"
#include "uo_engine.h"

#include <stdint.h>

void uo_tuning_generate_dataset(char *dataset_filepath, char *engine_filepath, char *engine_option_commands, size_t position_count)
{
  char *ptr;
  char buffer[0x1000];

  uo_rand_init(time(NULL));

  uo_process *engine_process = uo_process_create(engine_filepath);
  FILE *fp = fopen(dataset_filepath, "a");

  uo_process_write_stdin(engine_process, "uci\n", 0);
  if (engine_option_commands)
  {
    uo_process_write_stdin(engine_process, engine_option_commands, 0);
  }

  uo_position position;

  for (size_t i = 0; i < position_count; ++i)
  {
    uo_process_write_stdin(engine_process, "isready\n", 0);
    ptr = buffer;
    do
    {
      uo_process_read_stdout(engine_process, buffer, sizeof buffer);
      ptr = strstr(buffer, "readyok");
    } while (!ptr);

    uo_position_randomize(&position, NULL);

    while (!uo_position_is_quiescent(&position))
    {
      uo_position_randomize(&position, NULL);
    }

    ptr = buffer;
    ptr += sprintf(buffer, "position fen ");
    ptr += uo_position_print_fen(&position, ptr);
    ptr += sprintf(ptr, "\n");

    uo_process_write_stdin(engine_process, buffer, 0);

    uo_process_write_stdin(engine_process, "isready\n", 0);
    ptr = buffer;
    do
    {
      uo_process_read_stdout(engine_process, buffer, sizeof buffer);
      ptr = strstr(buffer, "readyok");
    } while (!ptr);

    uo_process_write_stdin(engine_process, "go depth 1\n", 0);

    do
    {
      uo_process_read_stdout(engine_process, buffer, sizeof buffer);

      char *mate = strstr(buffer, "score mate ");
      char *nomoves = strstr(buffer, "bestmove (none)");
      if (mate || nomoves)
      {
        --i;
        goto next_position;
      }

      ptr = strstr(buffer, "info depth 1");
    } while (!ptr);

    ptr = strstr(ptr, "score");

    int16_t score;
    if (sscanf(ptr, "score cp %hd", &score) == 1)
    {
      if (uo_color(position.flags) == uo_black) score *= -1.0f;

      char *ptr = buffer;
      ptr += uo_position_print_fen(&position, ptr);
      ptr += sprintf(ptr, ",%+d\n", score);
      fprintf(fp, "%s", buffer);
    }

    if ((i + 1) % 1000 == 0)
    {
      printf("positions generated: %zu\n", i + 1);
    }

  next_position:
    uo_process_write_stdin(engine_process, "ucinewgame\n", 0);
  }

  uo_process_write_stdin(engine_process, "quit\n", 0);

  uo_process_free(engine_process);
  fclose(fp);
}

bool uo_tuning_calculate_eval_mean_square_error(char *dataset_filepath, double *mse)
{
  void *allocated_mem[3];
  size_t allocated_mem_count = 0;

  if (!dataset_filepath)
  {
    if (!*engine_options.dataset_dir) return false;
    dataset_filepath = uo_aprintf("%s/dataset_depth_6.csv", engine_options.dataset_dir);
    allocated_mem[allocated_mem_count++] = dataset_filepath;
  }

  uo_file_mmap *file_mmap = uo_file_mmap_open_read(dataset_filepath);
  if (!file_mmap)
  {
    while (allocated_mem_count--) free(allocated_mem[allocated_mem_count]);
    return false;
  }

  uo_position position;
  *mse = 0.0;
  double count = 0.0;

  char *line = uo_file_mmap_readline(file_mmap);
  while (line && *line)
  {
    char *fen = line;
    char *eval = strchr(fen, ',') + 1;

    // Exclude positions where mate is detected
    if (eval[0] == '#')
    {
      line = uo_file_mmap_readline(file_mmap);
      continue;
    }

    uo_position_from_fen(&position, fen);
    uint8_t color = uo_color(position.flags);

    char *end;
    double cp_expected = (double)strtol(eval, &end, 10);
    if (color == uo_black) cp_expected = -cp_expected;
    double q_score_expected = uo_score_centipawn_to_q_score(cp_expected);
    double cp_actual = uo_position_evaluate(&position);
    double q_score_actual = uo_score_centipawn_to_q_score(cp_actual);
    double diff = q_score_expected - q_score_actual;

    count = count + 1.0;
    *mse = *mse * (count - 1.0) / count + diff * diff / count;

    line = uo_file_mmap_readline(file_mmap);
  }

  uo_file_mmap_close(file_mmap);
  //if (nn_output_file) uo_nn_save_parameters_to_file(nn, nn_output_file);
  while (allocated_mem_count--) free(allocated_mem[allocated_mem_count]);
  return true;
}
