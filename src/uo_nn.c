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
#define uo_nn_adam_learning_rate 1e-4
#define uo_nn_adam_beta1 0.9
#define uo_nn_adam_beta2 0.999
#define uo_nn_adam_epsilon 1e-8
#define uo_nn_adam_weight_decay 1e-2

uo_avx_float uo_nn_loss_function_mean_squared_error(uo_avx_float y_true, uo_avx_float y_pred)
{
  __m256 sub = _mm256_sub_ps(y_pred, y_true);
  __m256 mul = _mm256_mul_ps(sub, sub);
  return mul;
}

uo_avx_float uo_nn_loss_function_mean_squared_error_d(uo_avx_float y_true, uo_avx_float y_pred)
{
  return _mm256_sub_ps(y_pred, y_true);
}

uo_nn_loss_function_param loss_mse = {
  .f = uo_nn_loss_function_mean_squared_error,
  .df = uo_nn_loss_function_mean_squared_error_d
};

typedef struct uo_nn_layer_param
{
  size_t n;
  char *activation_function;
} uo_nn_layer_param;

typedef struct uo_nn_layer
{
  float *W;   // weights matrix, m x n
  float *dW;  // derivative of loss wrt weights, m x n
  float *Z;   // output before activation matrix, batch_size x n
  float *dZ;  // derivative of loss wrt output before activation, batch_size x n
  float *A;   // output matrix, batch_size x n
  float *dA;  // derivative of loss wrt activation, batch_size x n
  size_t m_W;   // weights row count
  size_t n_W;   // weights column count
  uo_nn_activation_function *activation_func;
  uo_nn_activation_function *activation_func_d;

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

  struct
  {
    size_t t; // timestep
  } adam;
} uo_nn;

void uo_nn_init(uo_nn *nn, size_t layer_count, size_t batch_size, uo_nn_layer_param *layer_params, uo_nn_loss_function_param loss)
{
  // Step 1. Allocate layers array and set options
  nn->batch_size = batch_size;
  nn->loss_func = loss.f;
  nn->loss_func_d = loss.df;
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

    const uo_nn_activation_function_param *activation_function_param = uo_nn_get_activation_function_by_name(layer_params[layer_index].activation_function);

    layer->activation_func = activation_function_param->f;

    if (layer->activation_func)
    {
      layer->activation_func_d = activation_function_param->df;
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

    layer->W = mem;
    mem += size_weights;
    layer->dW = mem;
    mem += size_weights;
    layer->adam.m = mem;
    mem += size_weights;
    layer->adam.v = mem;
    mem += size_weights;
    layer->adam.m_hat = mem;
    mem += size_weights;
    layer->adam.v_hat = mem;
  }

  // Step 4. Set output layer
  uo_nn_layer *output_layer = nn->layers + layer_count;
  nn->n_y = output_layer->n_W;
  nn->y = output_layer->A;

  // Step 5. Initialize weights to random values
  for (size_t i = 1; i <= layer_count; ++i)
  {
    uo_nn_layer *layer = nn->layers + i;
    size_t count = layer->m_W * layer->n_W;

    for (size_t j = 0; j < count; ++j)
    {
      layer->W[j] = (((float)rand() / (float)RAND_MAX) - 0.5f) * 2.0f;
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

  // Step 1. Piece locations

  for (uo_square i = 0; i < 64; ++i)
  {
    uo_piece piece = position->board[i];

    if (piece <= 1) continue;

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

    input[offset + i] = 1.0;
  }

  // Step 2. Castling
  size_t offset_castling = (64 * 5 + 48) * 2;
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

    // Step 2. Calculate dot product Z = XW
    size_t m_W = layer->m_W;
    size_t n_W = layer->n_W;
    float *W = layer->W;
    float *W_t = nn->temp[0];
    uo_transpose_ps(W, W_t, m_W, n_W);

    float *Z = layer->Z;
    uo_matmul_ps(X, W_t, Z, nn->batch_size, n_W, m_W, bias_offset);

    // Step 3. Apply activation function A = f(Z)
    if (layer->activation_func)
    {
      size_t n_A = n_W + bias_offset;
      float *A = layer->A;
      uo_vec_mapfunc_ps(Z, A, nn->batch_size * n_A, layer->activation_func);
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

    if (layer->activation_func_d)
    {
      // Step 2. Derivative of activation wrt Z
      float *dAdZ = nn->temp[0];
      size_t n_A = layer->n_W + bias_offset;
      float *Z = layer->Z;
      uo_vec_mapfunc_ps(Z, dAdZ, nn->batch_size * n_A, layer->activation_func_d);

      // Step 3. Derivative of loss wrt Z
      float *dA = layer->dA;
      uo_vec_mul_ps(dAdZ, dA, dZ, nn->batch_size * n_A);
    }

    // Step 4. Derivative of loss wrt weights
    size_t m_W = layer->m_W;
    size_t n_W = layer->n_W;
    float *dW = layer->dW;

    float *X = layer[-1].A;
    float *X_t = nn->temp[1];
    uo_transpose_ps(X, X_t, nn->batch_size, m_W);

    size_t n_dZ = n_W + bias_offset;
    float *dZ_t = nn->temp[2];
    uo_transpose_ps(dZ, dZ_t, nn->batch_size, n_dZ);

    uo_matmul_ps(X_t, dZ_t, dW, m_W, n_W, nn->batch_size, 0);

    if (layer_index > 1)
    {
      // Step 5. Derivative of loss wrt input
      float *dX = layer[-1].dA;
      float *W = layer->W;
      uo_matmul_ps(dZ, W, dX, nn->batch_size, m_W, n_W, 0);
    }

    // Step 6. Update weights using Adam update
    float *W = layer->W;
    float *m = layer->adam.m;
    float *v = layer->adam.v;
    float *m_hat = layer->adam.m_hat;
    float *v_hat = layer->adam.v_hat;
    float t = nn->adam.t;
    float learning_rate = lr_multiplier * uo_nn_adam_learning_rate;

    for (size_t i = 0; i < m_W * n_W; ++i)
    {
      m[i] = uo_nn_adam_beta1 * m[i] + (1.0f - uo_nn_adam_beta1) * dW[i];
      v[i] = uo_nn_adam_beta2 * v[i] + (1.0f - uo_nn_adam_beta2) * (dW[i] * dW[i]);
      m_hat[i] = m[i] / (1.0f - powf(uo_nn_adam_beta1, t));
      v_hat[i] = v[i] / (1.0f - powf(uo_nn_adam_beta2, t));
      //W[i] -= (i + 1) % n_W == 0 ? 0.0f : (uo_nn_adam_weight_decay * uo_nn_adam_learning_rate * W[i]);
      W[i] -= learning_rate * m_hat[i] / (sqrtf(v_hat[i] + uo_nn_adam_epsilon));
    }
  }

  // Step 7. Increment Adam update timestep
  nn->adam.t++;
}

typedef void uo_nn_select_batch(uo_nn *nn, size_t iteration, float *X, float *y_true);
typedef void uo_nn_report(uo_nn *nn, size_t iteration, float error);

bool uo_nn_train(uo_nn *nn, uo_nn_select_batch *select_batch, float error_threshold, size_t error_sample_size, size_t max_iterations, uo_nn_report *report, size_t report_interval)
{
  size_t counter = 0;
  size_t output_size = nn->batch_size * nn->n_y;

  float *y_true = malloc(output_size * sizeof(float));

  for (size_t i = 0; i < max_iterations; ++i)
  {
    select_batch(nn, i, nn->X, y_true);

    uo_nn_feed_forward(nn);

    float loss = uo_nn_calculate_loss(nn, y_true);

    if (isnan(loss))
    {
      free(y_true);
      return false;
    }

    if (loss < error_threshold)
    {
      ++counter;

      if (counter >= error_sample_size)
      {
        report(nn, i, loss);
        free(y_true);
        return true;
      }
    }
    else
    {
      counter = 0;
    }

    if (i % report_interval == 0)
    {
      report(nn, i, loss);
    }

    float lr_multiplier = 1.0f - (float)i / (float)max_iterations;
    uo_nn_backprop(nn, y_true, lr_multiplier);
  }

  free(y_true);
  return false;
}

void uo_print_nn(FILE *const fp, uo_nn *nn)
{
  for (size_t layer_index = 1; layer_index <= nn->layer_count; ++layer_index)
  {
    uo_nn_layer *layer = nn->layers + layer_index;
    size_t m_W = layer->m_W;
    size_t n_W = layer->n_W;
    const char *activation_function = uo_nn_get_activation_function_name(layer->activation_func);
    fprintf(fp, "Layer %zu (%zu x %zu)", layer_index, m_W, n_W);

    if (activation_function)
    {
      fprintf(fp, " - %s", activation_function);
    }

    fprintf(fp, ":");
    uo_print_matrix(fp, layer->W, m_W, n_W);
    fprintf(fp, "\n\n");
  }
}

void uo_nn_save_to_file(uo_nn *nn, char *filepath)
{
  FILE *fp = fopen(filepath, "w");
  uo_print_nn(fp, nn);
  fclose(fp);
}

uo_nn *uo_nn_read_from_file(char *filepath)
{
  uo_file_mmap *file_mmap = uo_file_mmap_open_read(filepath);



  uo_file_mmap_close(file_mmap);
}

void uo_nn_example()
{
  uo_nn nn;
  uo_nn_init(&nn, 3, 1, (uo_nn_layer_param[]) {
    { 832 },
    { 64, "relu" },
    { 32, "relu" },
    { 1 }
  }, loss_mse);
}

void uo_nn_select_batch_test(uo_nn *nn, size_t iteration, float *X, float *y_true)
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

void uo_nn_report_test(uo_nn *nn, size_t iteration, float error)
{
  printf("iteration %zu rmse: %f\n", iteration, sqrtf(error));

  if (iteration % 10000 == 0)
  {
    uo_print_nn(stdout, nn);
  }
}

bool uo_test_nn_train(char *test_data_dir)
{
  if (!test_data_dir) return false;

  size_t test_count = 0;

  char *filepath = buf;

  strcpy(filepath, test_data_dir);
  strcpy(filepath + strlen(test_data_dir), "/nn-test-xor.nnuo");

  srand(time(NULL));

  size_t batch_size = 256;
  uo_nn nn;
  uo_nn_init(&nn, 2, batch_size, (uo_nn_layer_param[]) {
    { 2 },
    { 2, "swish" },
    { 1 }
  }, loss_mse);

  bool passed = uo_nn_train(&nn, uo_nn_select_batch_test, pow(1e-3, 2), 100, 100000, uo_nn_report_test, 1000);

  if (!passed)
  {
    uo_print_nn(stdout, &nn);
    return false;
  }

  float *y_true = uo_alloca(batch_size * sizeof(float));

  for (size_t i = 0; i < 1000; ++i)
  {
    uo_nn_select_batch_test(&nn, i, nn.X, y_true);
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
