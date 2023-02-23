#include "uo_nn.h"
#include "uo_math.h"
#include "uo_misc.h"
#include "uo_global.h"
#include "uo_util.h"
#include "uo_engine.h"

#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

// see: https://arxiv.org/pdf/1412.6980.pdf
// see: https://arxiv.org/pdf/1711.05101.pdf
#define uo_nn_adam_learning_rate 1e-4
#define uo_nn_adam_beta1 0.9
#define uo_nn_adam_beta2 0.999
#define uo_nn_adam_epsilon 1e-8
#define uo_nn_adam_weight_decay 1e-2

#define uo_score_centipawn_to_win_prob(cp) (atanf(score / 290.680623072f) / 3.096181612f + 0.5f)
#define uo_score_centipawn_to_q_score(cp) (atanf(score / 111.714640912f) / 1.5620688421f)
#define uo_score_win_prob_to_centipawn(winprob) (int16_t)(290.680623072f * tanf(3.096181612f * (win_prob - 0.5f)))
#define uo_score_q_score_to_centipawn(q_score) (int16_t)(111.714640912f * tanf(1.5620688421f * q_score))

void uo_print_nn(FILE *const fp, uo_nn *nn)
{
  fprintf(fp,
    "ir_version: 3\n"
    "producer_name: \"uochess\"\n");

  //uo_print_nn_graph(fp, nn->graph, nn->graph_size);

  fprintf(fp,
    "opset_import {\n"
    "  domain: \"\"\n"
    "  version: 7\n"
    "}\n");
}

void uo_nn_save_to_file(uo_nn *nn, char *filepath)
{
  FILE *fp = fopen(filepath, "w");
  uo_print_nn(fp, nn);
  fclose(fp);
}

//int16_t uo_nn_evaluate(uo_nn *nn, const uo_position *position)
//{
//  uo_nn_load_position(nn, position, 0);
//  uo_nn_feed_forward(nn);
//  return uo_score_q_score_to_centipawn(*nn->y);
//  //return uo_score_win_prob_to_centipawn(*nn->y);
//}

uo_nn *uo_nn_read_from_file(uo_nn *nn, char *filepath, size_t batch_size)
{
  uo_file_mmap *file_mmap = uo_file_mmap_open_read(filepath);
  if (!file_mmap) return NULL;

  if (nn == NULL)
  {
    nn = malloc(sizeof * nn);
  }




  uo_file_mmap_close(file_mmap);

  return nn;
}

//typedef struct uo_nn_eval_state
//{
//  uo_file_mmap *file_mmap;
//  char *buf;
//  size_t buf_size;
//} uo_nn_eval_state;

//void uo_nn_train_eval_select_batch(uo_nn *nn, size_t iteration, uo_tensor *y_true)
//{
//  size_t batch_size = (*nn->inputs)->tensor->dim_sizes[0];
//  uo_tensor *own_floats = nn->inputs[0];
//  uo_tensor *own_mask = nn->inputs[1];
//  uo_tensor *enemy_floats = nn->inputs[2];
//  uo_tensor *enemy_mask = nn->inputs[3];
//  uo_tensor *shared_mask = nn->inputs[4];
//
//  uo_nn_eval_state *state = nn->state;
//  size_t i = (size_t)uo_rand_between(0.0f, (float)state->file_mmap->size);
//  i += iteration;
//  i %= state->file_mmap->size - batch_size * 160;
//
//  char *ptr = state->buf;
//  memcpy(ptr, state->file_mmap->ptr + i, state->buf_size);
//
//  char *fen = strchr(ptr, '\n') + 1;
//  char *eval = strchr(fen, ',') + 1;
//
//  uo_position position;
//
//  //char *tempnnfile = uo_aprintf("%s/nn-eval-temp.nnuo", engine_options.nn_dir);
//  //uo_nn_save_to_file(nn, tempnnfile);
//  //uo_nn nn;
//  //uo_nn_read_from_file(&nn, tempnnfile, batch_size);
//
//  for (size_t j = 0; j < batch_size; ++j)
//  {
//    bool matein = eval[0] == '#';
//    if (matein)
//    {
//      // Let's skip positions which lead to mate
//
//      ////win_prob = (eval[1] == '+' && color == uo_white) || (eval[1] == '-' && color == uo_black) ? 1.0f : 0.0f;
//      //q_score = (eval[1] == '+' && color == uo_white) || (eval[1] == '-' && color == uo_black) ? 1.0f : -1.0f;
//      fen = strchr(eval, '\n') + 1;
//      eval = strchr(fen, ',') + 1;
//
//      --j;
//      continue;
//    }
//
//    uo_position_from_fen(&position, fen);
//    uint8_t color = uo_color(position.flags);
//
//    //uint8_t color = uo_nn_load_fen(nn, fen, j);
//    assert(color == uo_white || color == uo_black);
//
//    // Copy position to input tensors
//    {
//      memcpy(own_floats->data.s + j * own_floats->dim_sizes[1], position.nn_input.halves[color].floats.vector, own_floats->dim_sizes[1] * sizeof(float));
//      memcpy(own_mask->data.u + j * own_mask->dim_sizes[1], position.nn_input.halves[color].mask.vector, own_mask->dim_sizes[1] * sizeof(uint32_t));
//      memcpy(enemy_floats->data.s + j * enemy_floats->dim_sizes[1], position.nn_input.halves[!color].floats.vector, enemy_floats->dim_sizes[1] * sizeof(float));
//      memcpy(enemy_mask->data.u + j * enemy_mask->dim_sizes[1], position.nn_input.halves[!color].mask.vector, enemy_mask->dim_sizes[1] * sizeof(uint32_t));
//      memcpy(shared_mask->data.u + j * shared_mask->dim_sizes[1], position.nn_input.shared.mask.vector, shared_mask->dim_sizes[1] * sizeof(uint32_t));
//    }
//
//    //uo_nn_load_position(&nn, &position, j);
//
//    //float win_prob;
//    float q_score;
//
//    char *end;
//    float score = (float)strtol(eval, &end, 10);
//
//    if (color == uo_black) score = -score;
//
//    q_score = uo_score_centipawn_to_q_score(score);
//    //win_prob = uo_score_centipawn_to_win_prob(score);
//
//    fen = strchr(end, '\n') + 1;
//    eval = strchr(fen, ',') + 1;
//
//    //y_true->data.s[j] = win_prob;
//    y_true->data.s[j] = q_score;
//  }
//
//  //assert(memcmp(nn->X, nn.X, nn.batch_size * nn.n_X * sizeof(float)) == 0);
//}

//void uo_nn_train_eval_report_progress(uo_nn *nn, size_t iteration, float error, float learning_rate)
//{
//  printf("iteration: %zu, error: %g, learning_rate: %g\n", iteration, error, learning_rate);
//}

bool uo_nn_train_eval(char *dataset_filepath, char *nn_init_filepath, char *nn_output_file, float learning_rate, size_t iterations, size_t batch_size)
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

  uo_rand_init(time(NULL));

  //if (!batch_size) batch_size = 0x100;

  //uo_nn_eval_state state = {
  //  .file_mmap = file_mmap,
  //  .buf_size = batch_size * 100,
  //  .buf = malloc(batch_size * 100),
  //};

  //allocated_mem[allocated_mem_count++] = state.buf;

  uo_nn nn;

  if (nn_init_filepath)
  {
    //uo_nn_read_from_file(&nn, nn_init_filepath, batch_size);
  }
  else
  {
    uo_rand_init(time(NULL));

    // TODO
  }

  //nn.state = &state;

  //if (!iterations) iterations = 1000;

  //uo_tensor *y_true = uo_tensor_create('s', 2, (size_t[]) { batch_size, 1 });

  //uo_nn_train_eval_select_batch(&nn, 0, y_true);
  //uo_nn_forward(&nn);
  //float loss_avg = uo_nn_loss_mse(*nn.outputs, y_true->data.s);

  //for (size_t i = 1; i < 1000000; ++i)
  //{
  //  uo_nn_reset(&nn);
  //  uo_nn_train_eval_select_batch(&nn, i, y_true);
  //  uo_nn_forward(&nn);

  //  float loss = uo_nn_loss_mse(*nn.outputs, y_true->data.s);

  //  loss_avg = loss_avg * 0.95 + loss * 0.05;

  //  if (loss_avg < 0.0001) break;

  //  if (i % 1000 == 0)
  //  {
  //    uo_nn_train_eval_report_progress(&nn, i, loss_avg, learning_rate);
  //  }

  //  uo_nn_loss_grad_mse(*nn.outputs, y_true->data.s);
  //  uo_nn_backward(&nn);
  //  uo_nn_update_initializers(&nn);
  //}

  //bool passed = loss_avg < 0.0001;

  //if (!passed)
  //{
  //  uo_file_mmap_close(file_mmap);
  //  //if (nn_output_file) uo_nn_save_to_file(&nn, nn_output_file);
  //  while (allocated_mem_count--) free(allocated_mem[allocated_mem_count]);
  //  return false;
  //}

  //for (size_t i = 0; i < 1000; ++i)
  //{
  //  uo_nn_reset(&nn);
  //  uo_nn_train_eval_select_batch(&nn, i, y_true);
  //  uo_nn_forward(&nn);

  //  float mse = uo_nn_loss_mse(*nn.outputs, y_true->data.s);
  //  float rmse = sqrt(mse);

  //  if (rmse > 0.1)
  //  {
  //    uo_file_mmap_close(file_mmap);
  //    //if (nn_output_file) uo_nn_save_to_file(&nn, nn_output_file);
  //    while (allocated_mem_count--) free(allocated_mem[allocated_mem_count]);
  //    return false;
  //  }
  //}

  //uo_file_mmap_close(file_mmap);
  ////if (nn_output_file) uo_nn_save_to_file(&nn, nn_output_file);
  //while (allocated_mem_count--) free(allocated_mem[allocated_mem_count]);
  //return true;
}

void uo_nn_select_batch_test_xor(uo_nn *nn, size_t iteration)
{
  float *X = nn->inputs->data;
  float *Y_true = nn->true_outputs->data;

  for (size_t i = 0; i < nn->batch_size; ++i)
  {
    float x0 = uo_rand_percent() > 0.5f ? 1.0 : 0.0;
    float x1 = uo_rand_percent() > 0.5f ? 1.0 : 0.0;
    float y = (int)x0 ^ (int)x1;
    Y_true[i] = y;
    X[i * 2] = x0;
    X[i * 2 + 1] = x1;
  }
}

void uo_nn_report_test_xor(uo_nn *nn, size_t iteration, float error, float learning_rate)
{
  printf("iteration: %zu, error: %g, learning_rate: %g\n", iteration, error, learning_rate);
}

bool uo_test_nn_train_xor(char *test_data_dir)
{
  if (!test_data_dir) return false;

  void *allocated_mem[1];
  size_t allocated_mem_count = 0;

  char *model_dirpath = uo_aprintf("%s/model_xor", engine_options.nn_dir);
  allocated_mem[allocated_mem_count++] = model_dirpath;

  uo_rand_init(time(NULL));

  size_t batch_size = 256;

  uo_nn *nn = uo_nn_create_xor(batch_size);
  uo_nn_init_optimizer(nn);

  uo_nn_select_batch_test_xor(nn, 0);

  uo_nn_forward(nn);
  float loss_avg = uo_nn_compute_loss(nn);

  for (size_t i = 1; i < 10000; ++i)
  {
    uo_nn_select_batch_test_xor(nn, i);
    uo_nn_forward(nn);

    float loss = uo_nn_compute_loss(nn);
    loss_avg = loss_avg * 0.95 + loss * 0.05;

    if (loss_avg < 0.001) break;

    if (i % 100 == 0)
    {
      uo_nn_report_test_xor(nn, i, loss_avg, 0);
    }

    uo_nn_backward(nn);
  }

  bool passed = loss_avg < 0.001;

  if (!passed)
  {
    //uo_print_nn(stdout, &nn);
    return false;
  }

  //uo_print_nn(stdout, &nn);
  //uo_nn_save_to_file(&nn, filepath);
  return true;
}

void uo_nn_generate_dataset(char *dataset_filepath, char *engine_filepath, char *engine_option_commands, size_t position_count)
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

    uo_position_randomize(&position);

    while (!uo_position_is_quiescent(&position))
    {
      uo_position_randomize(&position);
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

    uo_process_write_stdin(engine_process, "go depth 6\n", 0);

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

      ptr = strstr(buffer, "info depth 6");
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

