#include "uo_nn.h"
#include "uo_math.h"
#include "uo_misc.h"
#include "uo_global.h"

#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

// see: https://arxiv.org/pdf/1412.6980.pdf
// see: https://arxiv.org/pdf/1711.05101.pdf
#define uo_nn_adam_learning_rate 1e-13
#define uo_nn_adam_beta1 0.9
#define uo_nn_adam_beta2 0.999
#define uo_nn_adam_epsilon 1e-8
#define uo_nn_adam_weight_decay 1e-2

typedef struct uo_nn_layer_param
{
  size_t n;
  const char *function;
} uo_nn_layer_param;

typedef struct uo_nn_layer
{
  float *W_t;   // transpose of weights matrix, m x n
  float *dW_t;  // transpose of derivative of loss wrt weights, m x n
  float *Z;   // output before activation matrix, batch_size x n
  float *dZ;  // derivative of loss wrt output before activation, batch_size x n
  float *A;   // output matrix, batch_size x n
  float *dA;  // derivative of loss wrt activation, batch_size x n
  size_t m_W;   // weights row count
  size_t n_W;   // weights column count

  uo_nn_function_param func;

  struct
  {
    float *m; // first moment vector
    float *v; // second moment vector
    float *m_hat; // bias-corrected first moment matrix, m x n
    float *v_hat; // bias-corrected second moment matrix, m x n
  } adam;

} uo_nn_layer;

#define uo_nn_temp_var_count 5

typedef struct uo_nn
{
  size_t layer_count;
  uo_nn_layer *layers;
  size_t batch_size;
  size_t n_X;
  float *X;
  size_t n_y;
  float *y;
  uo_nn_loss_function *loss_func;
  uo_nn_loss_function *loss_func_d;

  // Some allocated memory to use in calculations
  float *temp[uo_nn_temp_var_count];

  // User data
  void *state;

  struct
  {
    size_t t; // timestep
  } adam;
} uo_nn;

void uo_nn_init(uo_nn *nn, size_t layer_count, size_t batch_size, uo_nn_layer_param *layer_params)
{
  // Step 1. Allocate layers array and set options
  nn->batch_size = batch_size;
  nn->layer_count = layer_count;
  nn->layers = calloc(nn->layer_count + 1, sizeof * nn->layers);
  nn->adam.t = 1;

  // Step 2. Allocate input layer
  uo_nn_layer *input_layer = nn->layers;
  nn->n_X = layer_params[0].n + 1;
  size_t size_input = batch_size * nn->n_X;
  size_t size_max = size_input;
  nn->X = input_layer->A = calloc(size_input, sizeof(float));

  // Step 3. Allocate hidden layers and output layer
  uo_nn_layer *layer = input_layer;

  for (size_t layer_index = 1; layer_index <= layer_count; ++layer_index)
  {
    ++layer;
    bool is_output_layer = layer_index == nn->layer_count;
    // For hidden layers, let's leave room for bias terms when computing output matrix
    int bias_offset = is_output_layer ? 0 : 1;

    size_t m_W = layer->m_W = layer_params[layer_index - 1].n + 1;
    size_t n_W = layer->n_W = layer_params[layer_index].n;
    size_t size_weights = m_W * n_W;
    size_max = uo_max(size_max, size_weights);
    size_t n_A = n_W + bias_offset;
    size_t size_output = batch_size * n_A;
    size_max = uo_max(size_max, size_output);

    float *mem;

    const uo_nn_function_param *function_param = uo_nn_get_function_by_name(layer_params[layer_index].function);
    layer->func = *function_param;

    if (layer->func.activation.df)
    {
      size_t size_total = size_output * 4 + size_weights * 6;
      mem = calloc(size_total, sizeof(float));

      layer->Z = mem;
      mem += size_output;
      layer->dZ = mem;
      mem += size_output;
      layer->A = mem;
      mem += size_output;
      layer->dA = mem;
      mem += size_output;
    }
    else
    {
      size_t size_total = size_output * 2 + size_weights * 6;
      mem = calloc(size_total, sizeof(float));

      layer->Z = layer->A = mem;
      mem += size_output;
      layer->dZ = layer->dA = mem;
      mem += size_output;
    }

    layer->W_t = mem;
    mem += size_weights;
    layer->dW_t = mem;
    mem += size_weights;
    layer->adam.m = mem;
    mem += size_weights;
    layer->adam.v = mem;
    mem += size_weights;
    layer->adam.m_hat = mem;
    mem += size_weights;
    layer->adam.v_hat = mem;
  }

  // Step 4. Set output layer and loss function
  uo_nn_layer *output_layer = nn->layers + layer_count;
  nn->n_y = output_layer->n_W;
  nn->y = output_layer->A;
  nn->loss_func = output_layer->func.loss.f;
  nn->loss_func_d = output_layer->func.loss.df;

  // Step 5. Initialize weights to random values
  for (size_t i = 1; i <= layer_count; ++i)
  {
    uo_nn_layer *layer = nn->layers + i;
    size_t m_W = layer->m_W;
    size_t n_W = layer->n_W;
    size_t count = m_W * n_W;
    float scale = 1.0f / sqrtf((float)(m_W - 1));

    for (size_t j = 0; j < count; ++j)
    {
      layer->W_t[j] = scale * ((((float)rand() / (float)RAND_MAX) - 0.5f) * 2.0f);
    }
  }

  // Step 6. Allocate space for temp data
  float *temp = calloc(size_max * uo_nn_temp_var_count, sizeof(float));
  for (size_t i = 0; i < uo_nn_temp_var_count; ++i)
  {
    nn->temp[i] = temp + size_max * i;
  }
}

void uo_nn_load_position(uo_nn *nn, const uo_position *position, size_t index)
{
  size_t n_X = nn->n_X;
  float *input = nn->X + n_X * index;
  memset(input, 0, n_X * sizeof(float));

  // Step 1. Piece configuration

  for (uo_square i = 0; i < 64; ++i)
  {
    uo_piece piece = position->board[i];

    if (piece <= 1)
    {
      // Empty square
      input[i] = 1.0f;
      continue;
    }

    size_t offset = uo_color(piece) ? 64 * 5 + 48 : 0;

    switch (piece)
    {
      case uo_piece__P:
        offset += 64 * 5 - 8;
        break;

      case uo_piece__N:
        offset += 64 * 4;
        break;

      case uo_piece__B:
        offset += 64 * 3;
        break;

      case uo_piece__R:
        offset += 64 * 2;
        break;

      case uo_piece__Q:
        offset += 64 * 1;
        break;
    }

    input[64 + offset + i] = 1.0f;
  }

  // Step 2. Castling
  size_t offset_castling = 64 + (64 * 5 + 48) * 2;
  input[offset_castling + 0] = uo_position_flags_castling_OO(position->flags);
  input[offset_castling + 1] = uo_position_flags_castling_OOO(position->flags);
  input[offset_castling + 2] = uo_position_flags_castling_enemy_OO(position->flags);
  input[offset_castling + 3] = uo_position_flags_castling_enemy_OOO(position->flags);

  // Step 3. Enpassant
  size_t offset_enpassant = offset_castling + 4;
  size_t enpassant_file = uo_position_flags_enpassant_file(position->flags);
  if (enpassant_file)
  {
    input[offset_enpassant + enpassant_file - 1] = 1.0;
  }
}

int8_t uo_nn_load_fen(uo_nn *nn, const char *fen, size_t index)
{
  char piece_placement[72];
  char active_color;
  char castling[5];
  char enpassant[3];
  int rule50;
  int fullmove;

  int count = sscanf(fen, "%71s %c %4s %2s %d %d",
    piece_placement, &active_color,
    castling, enpassant,
    &rule50, &fullmove);

  if (count != 6)
  {
    return -1;
  }

  size_t n_X = nn->n_X;
  float *input = nn->X + n_X * index;
  memset(input, 0, n_X * sizeof(float));

  uint8_t color = active_color == 'b' ? uo_black : uo_white;
  size_t flip_if_black = color == uo_black ? 56 : 0;

  // Step 1. Piece placement

  char *ptr = piece_placement;

  char c;

  // loop by rank
  for (int i = 7; i >= 0; --i)
  {
    // loop by file
    for (int j = 0; j < 8; ++j)
    {
      size_t index = (i * 8 + j) ^ flip_if_black;

      c = *ptr++;

      // empty squares
      if (c > '0' && c <= ('8' - j))
      {
        j += c - '0' - 1;
        input[index] = 1.0f;
        continue;
      }

      uo_piece piece = uo_piece_from_char(c);

      if (!piece)
      {
        return -1;
      }

      size_t offset = uo_color(piece) != color ? 64 * 5 + 48 : 0;

      switch (uo_piece_type(piece))
      {
        case uo_piece__P:
          offset += 64 * 5 - 8;
          break;

        case uo_piece__N:
          offset += 64 * 4;
          break;

        case uo_piece__B:
          offset += 64 * 3;
          break;

        case uo_piece__R:
          offset += 64 * 2;
          break;

        case uo_piece__Q:
          offset += 64 * 1;
          break;
      }

      input[64 + offset + index] = 1.0f;
    }

    c = *ptr++;

    if (i != 0 && c != '/')
    {
      return -1;
    }
  }

  // Step 2. Castling rights

  size_t offset_castling = 64 + (64 * 5 + 48) * 2;

  ptr = castling;
  c = *ptr++;

  if (c == '-')
  {
    c = *ptr++;
  }
  else
  {
    if (c == 'K')
    {
      input[offset_castling + (color == uo_white ? 0 : 2)] = 1.0f;
      c = *ptr++;
    }

    if (c == 'Q')
    {
      input[offset_castling + (color == uo_white ? 1 : 3)] = 1.0f;
      c = *ptr++;
    }

    if (c == 'k')
    {
      input[offset_castling + (color == uo_white ? 2 : 0)] = 1.0f;
      c = *ptr++;
    }

    if (c == 'q')
    {
      input[offset_castling + (color == uo_white ? 3 : 1)] = 1.0f;
      c = *ptr++;
    }
  }

  // Step 3. Enpassant

  size_t offset_enpassant = offset_castling + 4;

  ptr = enpassant;
  c = *ptr++;

  if (c != '-')
  {
    uint8_t file = c - 'a';
    if (file < 0 || file >= 8)
    {
      return -1;
    }

    c = *ptr++;
    uint8_t rank = c - '1';
    if (rank < 0 || rank >= 8)
    {
      return -1;
    }

    if (file)
    {
      input[offset_enpassant + file] = 1.0;
    }
  }

  return color;
}

float uo_nn_calculate_loss(uo_nn *nn, float *y_true)
{
  float *losses = nn->temp[0];
  uo_vec_map2func_ps(y_true, nn->y, losses, nn->n_y * nn->batch_size, nn->loss_func);
  float loss = uo_vec_mean_ps(losses, nn->n_y * nn->batch_size);
  return loss;
}

void uo_nn_feed_forward(uo_nn *nn)
{
  for (size_t layer_index = 1; layer_index <= nn->layer_count; ++layer_index)
  {
    uo_nn_layer *layer = nn->layers + layer_index;
    bool is_output_layer = layer_index == nn->layer_count;
    // For hidden layers, let's leave room for bias terms when computing output matrix
    int bias_offset = is_output_layer ? 0 : 1;

    // Step 1. Set bias input
    size_t n_X = layer->m_W;
    float *X = layer[-1].A;
    for (size_t i = 1; i <= nn->batch_size; ++i)
    {
      X[i * n_X - 1] = 1.0f;
    }

    // Step 2. Matrix multiplication Z = XW
    size_t m_W = layer->m_W;
    size_t n_W = layer->n_W;
    float *W_t = layer->W_t;
    float *Z = layer->Z;
    uo_matmul_ps(X, W_t, Z, nn->batch_size, n_W, m_W, bias_offset, 0, 0);

    // Step 3. Apply activation function A = f(Z)
    if (layer->func.activation.f)
    {
      size_t n_A = n_W + bias_offset;
      float *A = layer->A;
      uo_vec_mapfunc_ps(Z, A, nn->batch_size * n_A, layer->func.activation.f);
    }
  }
}

void uo_nn_backprop(uo_nn *nn, float *y_true, float lr_multiplier)
{
  // Step 1. Derivative of loss wrt output
  float *dA = nn->layers[nn->layer_count].dA;
  uo_vec_map2func_ps(y_true, nn->y, dA, nn->batch_size * nn->n_y, nn->loss_func_d);

  for (size_t layer_index = nn->layer_count; layer_index > 0; --layer_index)
  {
    uo_nn_layer *layer = nn->layers + layer_index;
    bool is_output_layer = layer_index == nn->layer_count;
    // For hidden layers, ignore bias terms when computing derivatives
    int bias_offset = is_output_layer ? 0 : 1;

    float *dZ = layer->dZ;

    if (layer->func.activation.df)
    {
      // Step 2. Derivative of loss wrt Z
      size_t n_A = layer->n_W + bias_offset;
      float *Z = layer->Z;
      float *dA = layer->dA;
      uo_vec_mapfunc_mul_ps(Z, dA, dZ, nn->batch_size * n_A, layer->func.activation.df);
    }

    // Step 3. Derivative of loss wrt weights
    size_t m_W = layer->m_W;
    size_t n_W = layer->n_W;
    float *dW_t = layer->dW_t;
    float *X = layer[-1].A;
    uo_matmul_t_ps(X, dZ, dW_t, m_W, n_W, nn->batch_size, 0, 0, bias_offset);

    if (layer_index > 1)
    {
      // Step 4. Derivative of loss wrt input
      float *dX = layer[-1].dA;
      float *W_t = layer->W_t;
      float *W = nn->temp[0];
      uo_transpose_ps(W_t, W, n_W, m_W);
      uo_matmul_ps(dZ, W, dX, nn->batch_size, m_W, n_W, 0, bias_offset, 0);
    }

    // Step 5. Update weights using Adam update
    float *W_t = layer->W_t;
    float *m = layer->adam.m;
    float *v = layer->adam.v;
    float *m_hat = layer->adam.m_hat;
    float *v_hat = layer->adam.v_hat;
    float t = nn->adam.t;
    float learning_rate = lr_multiplier * uo_nn_adam_learning_rate;

    for (size_t i = 0; i < m_W * n_W; ++i)
    {
      m[i] = uo_nn_adam_beta1 * m[i] + (1.0f - uo_nn_adam_beta1) * dW_t[i];
      v[i] = uo_nn_adam_beta2 * v[i] + (1.0f - uo_nn_adam_beta2) * (dW_t[i] * dW_t[i]);
      m_hat[i] = m[i] / (1.0f - powf(uo_nn_adam_beta1, t));
      v_hat[i] = v[i] / (1.0f - powf(uo_nn_adam_beta2, t));
      //W_t[i] -= (i + 1) % n_W_t == 0 ? 0.0f : (uo_nn_adam_weight_decay * uo_nn_adam_learning_rate * W_t[i]);
      W_t[i] -= learning_rate * m_hat[i] / (sqrtf(v_hat[i] + uo_nn_adam_epsilon));
    }
  }

  // Step 6. Increment Adam update timestep
  nn->adam.t++;
}

typedef void uo_nn_select_batch(uo_nn *nn, size_t iteration, float *X, float *y_true);
typedef void uo_nn_report(uo_nn *nn, size_t iteration, float error, float learning_rate);

bool uo_nn_train(uo_nn *nn, uo_nn_select_batch *select_batch, float error_threshold, size_t error_sample_size, size_t max_iterations, uo_nn_report *report, size_t report_interval)
{
  size_t output_size = nn->batch_size * nn->n_y;
  float avg_error, prev_avg_err, min_avg_err;

  float *y_true = malloc(output_size * sizeof(float));

  select_batch(nn, 0, nn->X, y_true);
  uo_nn_feed_forward(nn);

  float loss = uo_nn_calculate_loss(nn, y_true);

  if (isnan(loss))
  {
    free(y_true);
    return false;
  }

  float lr_multiplier = 1.0f;
  prev_avg_err = min_avg_err = avg_error = loss;

  for (size_t i = 1; i < max_iterations; ++i)
  {
    select_batch(nn, i, nn->X, y_true);
    uo_nn_feed_forward(nn);

    float loss = uo_nn_calculate_loss(nn, y_true);

    if (isnan(loss))
    {
      free(y_true);
      return false;
    }

    avg_error -= avg_error / error_sample_size;
    avg_error += loss / error_sample_size;

    if (i > error_sample_size && avg_error < error_threshold)
    {
      report(nn, i, avg_error, lr_multiplier * uo_nn_adam_learning_rate);
      free(y_true);
      return true;
    }

    if (i % report_interval == 0)
    {
      report(nn, i, avg_error, lr_multiplier * uo_nn_adam_learning_rate);

      if (avg_error > prev_avg_err)
      {
        lr_multiplier *= 0.65f;
      }
      else if (avg_error < min_avg_err)
      {
        lr_multiplier *= 1.05f;
        min_avg_err = avg_error;
      }

      prev_avg_err = avg_error;
    }

    uo_nn_backprop(nn, y_true, lr_multiplier);
  }

  free(y_true);
  return false;
}

int16_t uo_nn_evaluate(uo_nn *nn, const uo_position *position)
{
  uo_nn_load_position(nn, position, 0);
  uo_nn_feed_forward(nn);
  float win_prob = *nn->y;
  return (int16_t)(400.0f * log10f(win_prob / (1.0f - win_prob)));
}

void uo_print_nn(FILE *const fp, uo_nn *nn)
{
  fprintf(fp, "Layers: %zu\n\n", nn->layer_count);

  for (size_t layer_index = 1; layer_index <= nn->layer_count; ++layer_index)
  {
    uo_nn_layer *layer = nn->layers + layer_index;
    size_t m_W = layer->m_W;
    size_t n_W = layer->n_W;

    fprintf(fp, "Layer %zu (%zu x %zu)", layer_index, m_W, n_W);

    if (layer->func.name)
    {
      fprintf(fp, " - %s", layer->func.name);
    }

    fprintf(fp, ":");
    float *W_t = layer->W_t;
    float *W = nn->temp[0];
    uo_transpose_ps(W_t, W, n_W, m_W);
    uo_print_matrix(fp, W, m_W, n_W);
    fprintf(fp, "\n\n");
  }
}

void uo_nn_save_to_file(uo_nn *nn, char *filepath)
{
  FILE *fp = fopen(filepath, "w");
  uo_print_nn(fp, nn);
  fclose(fp);
}

uo_nn *uo_nn_read_from_file(uo_nn *nn, char *filepath, size_t batch_size)
{
  uo_file_mmap *file_mmap = uo_file_mmap_open_read(filepath);
  if (!file_mmap) return NULL;

  if (nn == NULL)
  {
    nn = malloc(sizeof * nn);
  }

  size_t layer_count;

  char *ptr = file_mmap->ptr;
  ptr = strstr(ptr, "Layers: ");
  sscanf(ptr, "Layers: %zu", &layer_count);

  uo_nn_layer_param *layer_params = malloc((layer_count + 1) * sizeof * layer_params);
  char **layer_weight_matrices = malloc(layer_count * sizeof * layer_weight_matrices);

  for (size_t layer_index = 1; layer_index <= layer_count; ++layer_index)
  {
    size_t m_W;
    size_t n_W;

    ptr = strstr(ptr, "Layer ");
    int ret = sscanf(ptr, "Layer %*zu (%zu x %zu) - %s:", &m_W, &n_W, buf);

    layer_weight_matrices[layer_index - 1] = ptr = strchr(ptr, '[');

    layer_params[layer_index - 1].n = m_W - 1;
    layer_params[layer_index].n = n_W;

    if (ret == 3)
    {
      char *colon = strchr(buf, ':');
      *colon = '\0';

      const uo_nn_function_param *function_param = uo_nn_get_function_by_name(buf);
      layer_params[layer_index].function = function_param->name;
    }
  }

  uo_nn_init(nn, layer_count, batch_size, layer_params);
  free(layer_params);

  for (size_t layer_index = 1; layer_index <= layer_count; ++layer_index)
  {
    uo_nn_layer *layer = nn->layers + layer_index;
    size_t m_W = layer->m_W;
    size_t n_W = layer->n_W;
    float *W = nn->temp[0];
    uo_parse_matrix(layer_weight_matrices[layer_index - 1], &W, &m_W, &n_W);

    float *W_t = layer->W_t;
    uo_transpose_ps(W, W_t, m_W, n_W);
  }

  free(layer_weight_matrices);
  uo_file_mmap_close(file_mmap);

  return nn;
}

typedef struct uo_nn_eval_state
{
  uo_file_mmap *file_mmap;
  char *buf;
  size_t buf_size;
} uo_nn_eval_state;

void uo_nn_select_batch_test_eval(uo_nn *nn, size_t iteration, float *X, float *y_true)
{
  uo_nn_eval_state *state = nn->state;
  size_t i = (float)rand() / (float)RAND_MAX * state->file_mmap->size;
  i += iteration;
  i %= state->file_mmap->size - nn->batch_size * 160;

  char *ptr = state->buf;
  memcpy(ptr, state->file_mmap->ptr + i, state->buf_size);

  char *fen = strchr(ptr, '\n') + 1;
  char *eval = strchr(fen, ',') + 1;

  for (size_t j = 0; j < nn->batch_size; ++j)
  {
    uint8_t color = uo_nn_load_fen(nn, fen, j);

    assert(color == uo_white || color == uo_black);

    float win_prob;

    bool matein = eval[0] == '#';

    if (matein)
    {
      win_prob = (eval[1] == '+' && color == uo_white) || (eval[1] == '-' && color == uo_black) ? 1.0f : 0.0f;
      fen = strchr(eval, '\n') + 1;
      eval = strchr(fen, ',') + 1;
    }
    else
    {
      char *end;
      float score = (float)strtol(eval, &end, 10);
      score = color ? -score : score;
      win_prob = 1.0f / (1.0f + powf(10.0f, (-score / 400.0f)));

      fen = strchr(end, '\n') + 1;
      eval = strchr(fen, ',') + 1;
    }

    y_true[j] = win_prob;
  }
}

void uo_nn_report_test_eval(uo_nn *nn, size_t iteration, float error, float learning_rate)
{
  printf("iteration: %zu, error: %g, learning_rate: %g\n", iteration, error, learning_rate);
}

bool uo_test_nn_train_eval(char *test_data_dir, bool init_from_file)
{
  if (!test_data_dir) return false;

  size_t len_data_dir = strlen(test_data_dir);

  char *eval_filepath = buf;
  if (eval_filepath != test_data_dir) strcpy(eval_filepath, test_data_dir);

  strcpy(eval_filepath + len_data_dir, "/evaluations.csv");

  uo_file_mmap *file_mmap = uo_file_mmap_open_read(eval_filepath);

  if (!file_mmap) return false;

  char *nn_filepath = buf;
  strcpy(nn_filepath + len_data_dir, "/nn-test-eval.nnuo");

  srand(time(NULL));

  size_t batch_size = 512;
  uo_nn_eval_state state = {
    .file_mmap = file_mmap,
    .buf_size = batch_size * 100,
    .buf = malloc(batch_size * 100),
  };

  uo_nn nn;

  if (init_from_file)
  {
    uo_nn_read_from_file(&nn, nn_filepath, batch_size);
  }
  else
  {
    uo_nn_init(&nn, 4, batch_size, (uo_nn_layer_param[]) {
      { 815 },
      { 255, "swish" },
      { 63, "swish" },
      { 31, "swish" },
      { 1,  "sigmoid_loss_binary_kl_divergence" }
    });
  }

  nn.state = &state;

  bool passed = uo_nn_train(&nn, uo_nn_select_batch_test_eval, pow(0.1, 2), 1000, 50000, uo_nn_report_test_eval, 1000);

  if (!passed)
  {
    uo_file_mmap_close(file_mmap);
    free(state.buf);
    uo_nn_save_to_file(&nn, nn_filepath);
    return false;
  }

  float *y_true = uo_alloca(batch_size * sizeof(float));

  for (size_t i = 0; i < 1000; ++i)
  {
    uo_nn_select_batch_test_eval(&nn, i, nn.X, y_true);
    uo_nn_feed_forward(&nn);

    float mse = uo_nn_calculate_loss(&nn, y_true);
    float rmse = sqrt(mse);

    if (rmse > 0.1)
    {
      uo_file_mmap_close(file_mmap);
      free(state.buf);
      uo_nn_save_to_file(&nn, nn_filepath);
      return false;
    }
  }

  uo_file_mmap_close(file_mmap);
  free(state.buf);
  uo_nn_save_to_file(&nn, nn_filepath);
  return true;
}

void uo_nn_select_batch_test_xor(uo_nn *nn, size_t iteration, float *X, float *y_true)
{
  for (size_t j = 0; j < nn->batch_size; ++j)
  {
    float x0 = rand() > (RAND_MAX / 2) ? 1.0 : 0.0;
    float x1 = rand() > (RAND_MAX / 2) ? 1.0 : 0.0;
    float y = (int)x0 ^ (int)x1;
    y_true[j] = y;
    X[j * nn->n_X] = x0;
    X[j * nn->n_X + 1] = x1;
  }
}

void uo_nn_report_test_xor(uo_nn *nn, size_t iteration, float error, float learning_rate)
{
  printf("iteration: %zu, error: %g, learning_rate: %g\n", iteration, error, learning_rate);
}

bool uo_test_nn_train_xor(char *test_data_dir)
{
  if (!test_data_dir) return false;

  char *filepath = buf;

  strcpy(filepath, test_data_dir);
  strcpy(filepath + strlen(test_data_dir), "/nn-test-xor.nnuo");

  srand(time(NULL));

  size_t batch_size = 256;
  uo_nn nn;
  uo_nn_init(&nn, 3, batch_size, (uo_nn_layer_param[]) {
    { 2 },
    { 2, "swish" },
    { 2, "sigmoid" },
    { 1, "sigmoid_loss_binary_cross_entropy" }
  });

  bool passed = uo_nn_train(&nn, uo_nn_select_batch_test_xor, pow(1e-3, 2), 100, 400000, uo_nn_report_test_xor, 1000);

  if (!passed)
  {
    uo_print_nn(stdout, &nn);
    return false;
  }

  float *y_true = uo_alloca(batch_size * sizeof(float));

  for (size_t i = 0; i < 1000; ++i)
  {
    uo_nn_select_batch_test_xor(&nn, i, nn.X, y_true);
    uo_nn_feed_forward(&nn);

    float mse = uo_nn_calculate_loss(&nn, y_true);
    float rmse = sqrt(mse);

    if (rmse > 0.001)
    {
      uo_print_nn(stdout, &nn);
      return false;
    }
  }

  uo_print_nn(stdout, &nn);
  uo_nn_save_to_file(&nn, filepath);
  return true;
}
