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

typedef struct uo_tensor
{
  /*
    s = single precision float (32 bit)
    d = double precision float (64 bit)
    i = signed integer (32 bit)
    u = unsigned integer (32 bit)
  */
  char type;
  union
  {
    float *s;
    double *d;
    int32_t *i;
    uint32_t *u;
  };
  size_t dim_count;
  size_t *dim_sizes;
} uo_tensor;

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


//void uo_print_nn(FILE *const fp, uo_nn *nn)
//{
//  fprintf(fp, "Layers: %zu\n\n", nn->layer_count);
//
//  for (size_t layer_index = 1; layer_index <= nn->layer_count; ++layer_index)
//  {
//    uo_nn_layer *layer = nn->layers + layer_index;
//    size_t m_W = layer->m_W;
//    size_t n_W = layer->n_W;
//
//    fprintf(fp, "Layer %zu (%zu x %zu)", layer_index, m_W, n_W);
//
//    if (layer->func.name)
//    {
//      fprintf(fp, " - %s", layer->func.name);
//    }
//
//    fprintf(fp, ":");
//    float *W_t = layer->W_t;
//    float *W = nn->temp[0];
//    uo_transpose_ps(W_t, W, n_W, m_W);
//    uo_print_matrix(fp, W, m_W, n_W);
//    fprintf(fp, "\n\n");
//  }
//}
//
//void uo_nn_save_to_file(uo_nn *nn, char *filepath)
//{
//  FILE *fp = fopen(filepath, "w");
//  uo_print_nn(fp, nn);
//  fclose(fp);
//}

void uo_nn_init(uo_nn *nn, size_t batch_size, uo_nn_node **graph)
{
  // Step 1. Assign fields
  nn->graph = graph;
  nn->batch_size = batch_size;

  // Step 2. Allocate graph for operators
  size_t op_count = 0;
  uo_nn_node **iter = graph;

  while (*iter)
  {
    iter += (*iter)->input_arg_count + 1;
    ++op_count;
  }

  nn->op_count = op_count;
  nn->nodes = malloc(op_count * sizeof(uo_nn_node **));

  // Step 3. Initialize nodes and assign operator nodes
  iter = graph;
  size_t op_counter = 0;
  size_t op_index = 0;

  while (*iter)
  {
    uo_nn_node *node = *iter;

    if (op_counter == 0)
    {
      nn->nodes[op_index++] = iter;
      op_counter = node->input_arg_count + 1;
    }

    if (!node->is_init)
    {
      node->init(node, nn);
      node->is_init = true;
    }

    ++iter;
    --op_counter;
  }

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

    const uo_nn_function_param *function_param = uo_nn_get_function_by_name(layer_params[layer_index].function);
    layer->func = *function_param;

    if (layer_index == layer_count && !function_param->loss.f)
    {
      // If no loss function was set for output layer, let's default to mean squared error loss
      function_param = uo_nn_get_function_by_name("loss_mse");
      layer->func.loss.f = function_param->loss.f;
      layer->func.loss.df = function_param->loss.df;
    }

    if (layer->func.activation.df)
    {
      float *mem = calloc(size_output * 4, sizeof(float));
      layer->Z = mem;
      mem += size_output;
      layer->dZ = mem;
      mem += size_output;
      layer->A = mem;
      mem += size_output;
      layer->dA = mem;
    }
    else
    {
      float *mem = calloc(size_output * 2, sizeof(float));
      layer->Z = layer->A = mem;
      mem += size_output;
      layer->dZ = layer->dA = mem;
    }

    float *mem = calloc(size_weights * 6, sizeof(float));
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
      layer->W_t[j] = uo_rand_between(-scale, scale);
    }
  }

  // Step 6. Allocate space for temp data
  float *temp = calloc(size_max * uo_nn_temp_var_count, sizeof(float));
  for (size_t i = 0; i < uo_nn_temp_var_count; ++i)
  {
    nn->temp[i] = temp + size_max * i;
  }
}

void uo_nn_free(uo_nn *nn)
{
  free(nn->nodes);



  //free(nn->temp[0]);
  //free(nn->X);

  //for (size_t i = 1; i <= nn->layer_count; ++i)
  //{
  //  free(nn->layers[i - 1].Z);
  //  free(nn->layers[i - 1].W_t);
  //}

  //free(nn->layers);
}

void uo_nn_change_batch_size(uo_nn *nn, size_t batch_size)
{
  nn->batch_size = batch_size;

  // Step 1. Reallocate input layer
  uo_nn_layer *input_layer = nn->layers;
  size_t size_input = batch_size * nn->n_X;
  size_t size_max = size_input;
  free(nn->X);
  nn->X = input_layer->A = calloc(size_input, sizeof(float));

  // Step 2. Reallocate hidden layers and output layer
  uo_nn_layer *layer = input_layer;

  for (size_t layer_index = 1; layer_index <= nn->layer_count; ++layer_index)
  {
    ++layer;
    bool is_output_layer = layer_index == nn->layer_count;
    // For hidden layers, let's leave room for bias terms when computing output matrix
    int bias_offset = is_output_layer ? 0 : 1;

    size_t m_W = layer->m_W;
    size_t n_W = layer->n_W;
    size_t size_weights = m_W * n_W;
    size_max = uo_max(size_max, size_weights);
    size_t n_A = n_W + bias_offset;
    size_t size_output = batch_size * n_A;
    size_max = uo_max(size_max, size_output);

    if (layer->func.activation.df)
    {
      free(layer->Z);
      float *mem = calloc(size_output * 4, sizeof(float));
      layer->Z = mem;
      mem += size_output;
      layer->dZ = mem;
      mem += size_output;
      layer->A = mem;
      mem += size_output;
      layer->dA = mem;
    }
    else
    {
      free(layer->Z);
      float *mem = calloc(size_output * 2, sizeof(float));
      layer->Z = layer->A = mem;
      mem += size_output;
      layer->dZ = layer->dA = mem;
    }
  }

  // Step 3. Set output layer and loss function
  uo_nn_layer *output_layer = nn->layers + nn->layer_count;
  nn->n_y = output_layer->n_W;
  nn->y = output_layer->A;

  // Step 4. Allocate space for temp data
  free(nn->temp[0]);
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

  uo_nn_position *position_input = (uo_nn_position *)(void *)input;
  memset(position_input, 0, sizeof(uo_nn_position));

  // Step 1. Piece configuration

  for (uo_square square = 0; square < 64; ++square)
  {
    uo_piece piece = position->board[square];

    if (piece <= 1)
    {
      // Empty square
      position_input->data.empty[square] = 1.0f;
      continue;
    }

    switch (piece)
    {
      case uo_piece__P:
        position_input->data.piece_placement.own.P[square] = 1.0f;
        position_input->data.material.own.P += 1.0f;
        break;

      case uo_piece__N:
        position_input->data.piece_placement.own.N[square] = 1.0f;
        position_input->data.material.own.N += 1.0f;
        break;

      case uo_piece__B:
        position_input->data.piece_placement.own.B[square] = 1.0f;
        position_input->data.material.own.B += 1.0f;
        break;

      case uo_piece__R:
        position_input->data.piece_placement.own.R[square] = 1.0f;
        position_input->data.material.own.R += 1.0f;
        break;

      case uo_piece__Q:
        position_input->data.piece_placement.own.Q[square] = 1.0f;
        position_input->data.material.own.Q += 1.0f;
        break;

      case uo_piece__K:
        position_input->data.piece_placement.own.K[square] = 1.0f;
        break;

      case uo_piece__p:
        position_input->data.piece_placement.enemy.P[square] = 1.0f;
        position_input->data.material.enemy.P += 1.0f;
        break;

      case uo_piece__n:
        position_input->data.piece_placement.enemy.N[square] = 1.0f;
        position_input->data.material.enemy.N += 1.0f;
        break;

      case uo_piece__b:
        position_input->data.piece_placement.enemy.B[square] = 1.0f;
        position_input->data.material.enemy.B += 1.0f;
        break;

      case uo_piece__r:
        position_input->data.piece_placement.enemy.R[square] = 1.0f;
        position_input->data.material.enemy.R += 1.0f;
        break;

      case uo_piece__q:
        position_input->data.piece_placement.enemy.Q[square] = 1.0f;
        position_input->data.material.enemy.Q += 1.0f;
        break;

      case uo_piece__k:
        position_input->data.piece_placement.enemy.K[square] = 1.0f;
        break;
    }
  }

  // Step 2. Castling
  position_input->data.castling.own_K = uo_position_flags_castling_OO(position->flags);
  position_input->data.castling.own_Q = uo_position_flags_castling_OOO(position->flags);
  position_input->data.castling.enemy_K = uo_position_flags_castling_enemy_OO(position->flags);
  position_input->data.castling.enemy_Q = uo_position_flags_castling_enemy_OOO(position->flags);

  // Step 3. Enpassant
  size_t enpassant_file = uo_position_flags_enpassant_file(position->flags);

  if (enpassant_file)
  {
    position_input->data.enpassant[enpassant_file - 1] = 1.0f;
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

  uo_nn_position *position_input = (uo_nn_position *)(void *)input;
  memset(position_input, 0, sizeof(uo_nn_position));

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
      uo_square square = (i * 8 + j) ^ flip_if_black;

      c = *ptr++;

      // empty squares
      if (c > '0' && c <= ('8' - j))
      {
        size_t empty_count = c - '0';
        j += empty_count - 1;

        while (empty_count--)
        {
          position_input->data.empty[square + empty_count] = 1.0f;
        }

        continue;
      }

      uo_piece piece = uo_piece_from_char(c);

      if (!piece)
      {
        return -1;
      }

      piece ^= color;

      switch (piece)
      {
        case uo_piece__P:
          position_input->data.piece_placement.own.P[square] = 1.0f;
          position_input->data.material.own.P += 1.0f;
          break;

        case uo_piece__N:
          position_input->data.piece_placement.own.N[square] = 1.0f;
          position_input->data.material.own.N += 1.0f;
          break;

        case uo_piece__B:
          position_input->data.piece_placement.own.B[square] = 1.0f;
          position_input->data.material.own.B += 1.0f;
          break;

        case uo_piece__R:
          position_input->data.piece_placement.own.R[square] = 1.0f;
          position_input->data.material.own.R += 1.0f;
          break;

        case uo_piece__Q:
          position_input->data.piece_placement.own.Q[square] = 1.0f;
          position_input->data.material.own.Q += 1.0f;
          break;

        case uo_piece__K:
          position_input->data.piece_placement.own.K[square] = 1.0f;
          break;

        case uo_piece__p:
          position_input->data.piece_placement.enemy.P[square] = 1.0f;
          position_input->data.material.enemy.P += 1.0f;
          break;

        case uo_piece__n:
          position_input->data.piece_placement.enemy.N[square] = 1.0f;
          position_input->data.material.enemy.N += 1.0f;
          break;

        case uo_piece__b:
          position_input->data.piece_placement.enemy.B[square] = 1.0f;
          position_input->data.material.enemy.B += 1.0f;
          break;

        case uo_piece__r:
          position_input->data.piece_placement.enemy.R[square] = 1.0f;
          position_input->data.material.enemy.R += 1.0f;
          break;

        case uo_piece__q:
          position_input->data.piece_placement.enemy.Q[square] = 1.0f;
          position_input->data.material.enemy.Q += 1.0f;
          break;

        case uo_piece__k:
          position_input->data.piece_placement.enemy.K[square] = 1.0f;
          break;
      }
    }

    c = *ptr++;

    if (i != 0 && c != '/')
    {
      return -1;
    }
  }

  // Step 2. Castling rights

  ptr = castling;
  c = *ptr++;

  if (c == '-')
  {
    c = *ptr++;
  }
  else
  {
    if (color == uo_white)
    {
      if (c == 'K')
      {
        position_input->data.castling.own_K = 1.0f;
        c = *ptr++;
      }

      if (c == 'Q')
      {
        position_input->data.castling.own_Q = 1.0f;
        c = *ptr++;
      }

      if (c == 'k')
      {
        position_input->data.castling.enemy_K = 1.0f;
        c = *ptr++;
      }

      if (c == 'q')
      {
        position_input->data.castling.enemy_Q = 1.0f;
        c = *ptr++;
      }
    }
    else
    {
      if (c == 'K')
      {
        position_input->data.castling.enemy_K = 1.0f;
        c = *ptr++;
      }

      if (c == 'Q')
      {
        position_input->data.castling.enemy_Q = 1.0f;
        c = *ptr++;
      }

      if (c == 'k')
      {
        position_input->data.castling.own_K = 1.0f;
        c = *ptr++;
      }

      if (c == 'q')
      {
        position_input->data.castling.own_Q = 1.0f;
        c = *ptr++;
      }
    }
  }

  // Step 3. Enpassant

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

    position_input->data.enpassant[file] = 1.0f;
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

static void uo_nn_layer_feed_forward(uo_nn *nn, uo_nn_layer *layer)
{
  bool is_output_layer = layer - nn->layers == nn->layer_count;
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

void uo_nn_feed_forward(uo_nn *nn)
{
  for (size_t i = 0; i < nn->op_count; ++i)
  {
    uo_nn_node **graph = nn->nodes[i];
    uo_nn_node *node = *graph;
    node->forward(graph);
  }

  // Step 1. Feed forward input layer
  uo_nn_layer *input_layer = nn->layers + 1;

  // Step 1.1. Matrix multiplication Z = XW

  bool is_output_layer = 1 == nn->layer_count;
  // For non-output layers, let's leave room for bias terms when computing output matrix
  int bias_offset = is_output_layer ? 0 : 1;

  size_t m_W = input_layer->m_W;
  size_t n_W = input_layer->n_W;
  float *W_t = input_layer->W_t;
  float *Z = input_layer->Z;

  uint8_t color = uo_color(position->flags);
  uo_nn_input_half *halves = position->nn_input.halves;
  uo_nn_input_shared *shared = &position->nn_input.shared;

  uo_matmul_position_ps(
    halves[color].mask.vector, halves[!color].mask.vector, sizeof(halves[color].mask.vector) / sizeof(uint32_t),
    halves[color].floats.vector, halves[!color].floats.vector, sizeof(halves[color].floats.vector) / sizeof(float),
    shared->mask.vector, sizeof(shared->mask.vector) / sizeof(uint32_t),
    shared->floats.vector, sizeof(shared->floats.vector) / sizeof(float),
    W_t, Z, nn->batch_size, n_W, bias_offset, 0);

  // Step 1.2. Apply activation function A = f(Z)
  if (input_layer->func.activation.f)
  {
    size_t n_A = n_W + bias_offset;
    float *A = input_layer->A;
    uo_vec_mapfunc_ps(Z, A, nn->batch_size * n_A, input_layer->func.activation.f);
  }

  for (size_t layer_index = 2; layer_index <= nn->layer_count; ++layer_index)
  {
    uo_nn_layer *layer = nn->layers + layer_index;
    uo_nn_layer_feed_forward(nn, layer);
  }
}

// see: http://ufldl.stanford.edu/tutorial/supervised/DebuggingGradientChecking/
bool uo_nn_check_gradients(uo_nn *nn, uo_nn_layer *layer, float *d, float *y_true)
{
  size_t m;
  size_t n;
  float *val;
  size_t feed_forward_layer_index = layer - nn->layers;

  bool is_output_layer = layer->A == nn->y;
  // For hidden layers, ignore bias terms when computing derivatives
  int bias_offset = is_output_layer ? 0 : 1;

  if (d == layer->dW_t)
  {
    val = layer->W_t;
    m = layer->n_W;
    n = layer->m_W;
    bias_offset = 0;
  }
  else if (d == layer->dA)
  {
    val = layer->A;
    m = nn->batch_size;
    n = layer->n_W;
    ++feed_forward_layer_index;
  }
  else if (d == layer->dZ)
  {
    val = layer->Z;
    m = nn->batch_size;
    n = layer->n_W;
    ++feed_forward_layer_index;
  }
  else
  {
    return false;
  }

  bool passed = true;
  float epsilon = 1e-4f;

  for (size_t i = 0; i < m; ++i)
  {
    for (size_t j = 0; j < n; ++j)
    {
      size_t index = i * (n + bias_offset) + j;
      float val_ij = val[index];

      // Step 1. Add epsilon and feed forward

      val[index] = val_ij + epsilon;

      if (val == layer->Z && layer->func.activation.f)
      {
        bool is_output_layer = layer - nn->layers == nn->layer_count;
        // For hidden layers, let's leave room for bias terms when computing output matrix
        int bias_offset = is_output_layer ? 0 : 1;
        size_t n_W = layer->n_W;
        size_t n_A = n_W + bias_offset;
        float *Z = layer->Z;
        float *A = layer->A;
        uo_vec_mapfunc_ps(Z, A, nn->batch_size * n_A, layer->func.activation.f);
      }

      for (size_t layer_index = feed_forward_layer_index; layer_index <= nn->layer_count; ++layer_index)
      {
        uo_nn_layer_feed_forward(nn, nn->layers + layer_index);
      }

      float loss_plus = uo_nn_calculate_loss(nn, y_true);

      // Step 2. Subtract epsilon and feed forward

      val[index] = val_ij - epsilon;

      if (val == layer->Z && layer->func.activation.f)
      {
        bool is_output_layer = layer - nn->layers == nn->layer_count;
        // For hidden layers, let's leave room for bias terms when computing output matrix
        int bias_offset = is_output_layer ? 0 : 1;
        size_t n_W = layer->n_W;
        size_t n_A = n_W + bias_offset;
        float *Z = layer->Z;
        float *A = layer->A;
        uo_vec_mapfunc_ps(Z, A, nn->batch_size * n_A, layer->func.activation.f);
      }

      for (size_t layer_index = feed_forward_layer_index; layer_index <= nn->layer_count; ++layer_index)
      {
        uo_nn_layer_feed_forward(nn, nn->layers + layer_index);
      }

      float loss_minus = uo_nn_calculate_loss(nn, y_true);

      // Step 3. Restore previous value

      val[index] = val_ij;

      // Step 4. Compute numeric gradient and compare to calculated gradient

      float grad_num = (loss_plus - loss_minus) / (2.0f * epsilon);
      float grad_calc = d[index];

      float diff = grad_num - grad_calc;
      if (diff < 0.0f) diff = -diff;

      passed &= diff < 1e-2;
    }
  }

  return passed;
}

void uo_nn_backprop(uo_nn *nn, float *y_true, float lr_multiplier)
{
  for (size_t i = nn->op_count; i > 0; --i)
  {
    uo_nn_node **graph = nn->nodes[i - 1];
    uo_nn_node *node = *graph;
    node->backward(graph);
  }








  // Step 1. Derivative of loss wrt output
  float *dA = nn->layers[nn->layer_count].dA;
  uo_vec_map2func_ps(y_true, nn->y, dA, nn->batch_size * nn->n_y, nn->loss_func_d);
  uo_vec_mul1_ps(dA, 1.0f / (float)nn->batch_size, dA, nn->batch_size * nn->n_y);

  assert(uo_nn_check_gradients(nn, nn->layers + nn->layer_count, dA, y_true));

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
      assert(uo_nn_check_gradients(nn, layer, dZ, y_true));
    }

    // Step 3. Derivative of loss wrt weights
    size_t m_W = layer->m_W;
    size_t n_W = layer->n_W;
    float *dW_t = layer->dW_t;
    float *X = layer[-1].A;

    // X: m_X x m_W
    // dZ: m_X x n_W
    // dW: m_W x n_W

    // dW_t: n_W x m_W
    // X_t: m_W x m_X
    // dZ_t: n_W x m_X

    // dW = X_t . dZ

    uo_matmul_t_ps(X, dZ, dW_t, m_W, n_W, nn->batch_size, 0, 0, bias_offset);
    assert(uo_nn_check_gradients(nn, layer, dW_t, y_true));

    if (layer_index > 1)
    {
      // Step 4. Derivative of loss wrt input
      float *dX = layer[-1].dA;
      float *W_t = layer->W_t;
      float *W = nn->temp[0];
      uo_transpose_ps(W_t, W, n_W, m_W);
      uo_matmul_ps(dZ, W, dX, nn->batch_size, m_W, n_W, 0, bias_offset, 0);
      assert(uo_nn_check_gradients(nn, layer - 1, dX, y_true));
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
      W_t[i] -= learning_rate * m_hat[i] / (sqrtf(v_hat[i] + uo_nn_adam_epsilon));

      // weight decay
      W_t[i] -= (i + 1) % m_W == 0 ? 0.0f : (uo_nn_adam_weight_decay * learning_rate * W_t[i]);
    }
  }

  // Step 6. Increment Adam update timestep
  nn->adam.t++;
}

typedef void uo_nn_select_batch(uo_nn *nn, size_t iteration, float *X, float *y_true);
typedef void uo_nn_report(uo_nn *nn, size_t iteration, float error, float learning_rate);

bool uo_nn_train(uo_nn *nn, uo_nn_select_batch *select_batch, float error_threshold, size_t error_sample_size, size_t max_iterations, uo_nn_report *report, size_t report_interval, float learning_rate, size_t batch_size)
{
  if (batch_size && nn->batch_size != batch_size)
  {
    uo_nn_change_batch_size(nn, batch_size);
  }

  char *nn_filepath = NULL;

  size_t output_size = nn->batch_size * nn->n_y;
  float avg_error, prev_avg_err, min_avg_err;
  const size_t adam_reset_count_threshold = 6;
  size_t adam_reset_counter = 0;

  float *y_true = malloc(output_size * sizeof(float));

  select_batch(nn, 0, nn->X, y_true);
  uo_nn_feed_forward(nn);

  float loss = uo_nn_calculate_loss(nn, y_true);

  if (isnan(loss))
  {
    free(y_true);
    return false;
  }

  float lr_multiplier = learning_rate > 0.0f ? learning_rate / uo_nn_adam_learning_rate : 1.0f;
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
        ++adam_reset_counter;

        if (adam_reset_counter == adam_reset_count_threshold)
        {
          // Reset adam weights if loss keeps increasing
          prev_avg_err = avg_error = loss;
          adam_reset_counter = 0;
          nn->adam.t = 1;
          for (size_t layer_index = 1; layer_index < nn->layer_count; ++layer_index)
          {
            uo_nn_layer *layer = nn->layers + layer_index;
            memset(layer->adam.m, 0, layer->m_W * layer->n_W * sizeof(float));
          }

          lr_multiplier *= 0.75f;
          lr_multiplier = uo_max(lr_multiplier, 1e-10);
        }
      }
      else
      {
        adam_reset_counter = 0;

        if (avg_error < min_avg_err)
        {
          lr_multiplier *= 1.025f;
          min_avg_err = avg_error;

          if (*engine_options.nn_dir)
          {
            free(nn_filepath);
            char timestamp[13];
            uo_timestr(timestamp);
            nn_filepath = uo_aprintf("%s/nn-eval-%s.nnuo", engine_options.nn_dir, timestamp);
            uo_nn_save_to_file(nn, nn_filepath);
          }
        }
      }

      prev_avg_err = avg_error;
    }

    uo_nn_backprop(nn, y_true, lr_multiplier);
  }

  if (nn_filepath)
  {
    // Load best version from file
    uo_nn_free(nn);
    uo_nn_read_from_file(nn, nn_filepath, batch_size);
    free(nn_filepath);
  }

  free(y_true);
  return false;
}

int16_t uo_nn_evaluate(uo_nn *nn, const uo_position *position)
{
  uo_nn_load_position(nn, position, 0);
  uo_nn_feed_forward(nn);
  return uo_score_q_score_to_centipawn(*nn->y);
  //return uo_score_win_prob_to_centipawn(*nn->y);
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


void uo_nn_train_eval_select_batch(uo_nn *nn, size_t iteration, float *X, float *y_true)
{
  uo_nn_eval_state *state = nn->state;
  size_t i = (size_t)uo_rand_between(0.0f, (float)state->file_mmap->size);
  i += iteration;
  i %= state->file_mmap->size - nn->batch_size * 160;

  char *ptr = state->buf;
  memcpy(ptr, state->file_mmap->ptr + i, state->buf_size);

  char *fen = strchr(ptr, '\n') + 1;
  char *eval = strchr(fen, ',') + 1;

  //uo_position position;

  //char *tempnnfile = uo_aprintf("%s/nn-eval-temp.nnuo", engine_options.nn_dir);
  //uo_nn_save_to_file(nn, tempnnfile);
  //uo_nn nn2;
  //uo_nn_read_from_file(&nn2, tempnnfile, nn->batch_size);

  for (size_t j = 0; j < nn->batch_size; ++j)
  {
    uint8_t color = uo_nn_load_fen(nn, fen, j);
    assert(color == uo_white || color == uo_black);

    //uo_position_from_fen(&position, fen);
    //uo_nn_load_position(&nn2, &position, j);

    //float win_prob;
    float q_score;

    bool matein = eval[0] == '#';

    if (matein)
    {
      // Let's skip positions which lead to mate

      ////win_prob = (eval[1] == '+' && color == uo_white) || (eval[1] == '-' && color == uo_black) ? 1.0f : 0.0f;
      //q_score = (eval[1] == '+' && color == uo_white) || (eval[1] == '-' && color == uo_black) ? 1.0f : -1.0f;
      fen = strchr(eval, '\n') + 1;
      eval = strchr(fen, ',') + 1;

      --j;
      continue;
    }
    else
    {
      char *end;
      float score = (float)strtol(eval, &end, 10);

      if (color == uo_black) score = -score;

      q_score = uo_score_centipawn_to_q_score(score);
      //win_prob = uo_score_centipawn_to_win_prob(score);

      fen = strchr(end, '\n') + 1;
      eval = strchr(fen, ',') + 1;
    }

    //y_true[j] = win_prob;
    y_true[j] = q_score;
  }

  //assert(memcmp(nn->X, nn2.X, nn2.batch_size * nn2.n_X * sizeof(float)) == 0);
}

void uo_nn_train_eval_report_progress(uo_nn *nn, size_t iteration, float error, float learning_rate)
{
  printf("iteration: %zu, error: %g, learning_rate: %g\n", iteration, error, learning_rate);
}

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

  if (!batch_size) batch_size = 0x100;

  uo_nn_eval_state state = {
    .file_mmap = file_mmap,
    .buf_size = batch_size * 100,
    .buf = malloc(batch_size * 100),
  };

  allocated_mem[allocated_mem_count++] = state.buf;

  uo_nn nn;

  if (nn_init_filepath)
  {
    //uo_nn_read_from_file(&nn, nn_init_filepath, batch_size);
  }
  else
  {
    // Layer 1
    uo_nn_node_position position_node;
    uo_nn_node_weights W1_t = { { "weights", .m = sizeof(uo_nn_position) / sizeof(uint32_t), .n = 127 } };
    uo_nn_node swish1;

    // Layer 2
    uo_nn_node_weights W2_t = { { "weights", .m = W1_t.base.n + 1, .n = 8 } };
    uo_nn_node matmul2;
    uo_nn_node swish2;

    // Layer 3
    uo_nn_node_weights W3_t = { { "weights", .m = W2_t.base.n + 1, .n = 1 } };
    uo_nn_node matmul3;
    uo_nn_node tanh3;

    uo_nn_node *graph[] =
    {
      // Layer 1
      &position_node, &W1_t,
      &swish1, &position_node,

      // Layer 2
      &matmul2, &swish1, &W2_t,
      &swish2, &matmul2,

      // Layer 3
      &matmul3, &swish2, &W3_t,
      &tanh3, &matmul3,

      NULL
    };



    uo_nn_init(&nn, batch_size, graph);
  }

  nn.state = &state;

  if (!iterations) iterations = 1000;

  bool passed = uo_nn_train(&nn, uo_nn_train_eval_select_batch, pow(0.1, 2), 10, iterations, uo_nn_train_eval_report_progress, 100, learning_rate, batch_size);

  if (!passed)
  {
    uo_file_mmap_close(file_mmap);
    if (nn_output_file) uo_nn_save_to_file(&nn, nn_output_file);
    while (allocated_mem_count--) free(allocated_mem[allocated_mem_count]);
    return false;
  }

  float *y_true = uo_alloca(batch_size * sizeof(float));

  for (size_t i = 0; i < 1000; ++i)
  {
    uo_nn_train_eval_select_batch(&nn, i, nn.X, y_true);
    uo_nn_feed_forward(&nn);

    float mse = uo_nn_calculate_loss(&nn, y_true);
    float rmse = sqrt(mse);

    if (rmse > 0.1)
    {
      uo_file_mmap_close(file_mmap);
      if (nn_output_file) uo_nn_save_to_file(&nn, nn_output_file);
      while (allocated_mem_count--) free(allocated_mem[allocated_mem_count]);
      return false;
    }
  }

  uo_file_mmap_close(file_mmap);
  if (nn_output_file) uo_nn_save_to_file(&nn, nn_output_file);
  while (allocated_mem_count--) free(allocated_mem[allocated_mem_count]);
  return true;
}

//void uo_nn_select_batch_test_xor(uo_nn *nn, size_t iteration, float *X, float *y_true)
//{
//  for (size_t j = 0; j < nn->batch_size; ++j)
//  {
//    float x0 = uo_rand_percent() > 0.5f ? 1.0 : 0.0;
//    float x1 = uo_rand_percent() > 0.5f ? 1.0 : 0.0;
//    float y = (int)x0 ^ (int)x1;
//    y_true[j] = y;
//    X[j * nn->n_X] = x0;
//    X[j * nn->n_X + 1] = x1;
//  }
//}
//
//void uo_nn_report_test_xor(uo_nn *nn, size_t iteration, float error, float learning_rate)
//{
//  printf("iteration: %zu, error: %g, learning_rate: %g\n", iteration, error, learning_rate);
//}
//
//bool uo_test_nn_train_xor(char *test_data_dir)
//{
//  if (!test_data_dir) return false;
//
//  char *filepath = buf;
//
//  strcpy(filepath, test_data_dir);
//  strcpy(filepath + strlen(test_data_dir), "/nn-test-xor.nnuo");
//
//  uo_rand_init(time(NULL));
//
//  size_t batch_size = 256;
//  uo_nn nn;
//  uo_nn_init(&nn, 2, batch_size, (uo_nn_layer_param[]) {
//    { 2 },
//    { 2, "swish" },
//    { 1, "loss_mse" }
//  });
//
//  bool passed = uo_nn_train(&nn, uo_nn_select_batch_test_xor, pow(1e-3, 2), 100, 400000, uo_nn_report_test_xor, 1000, 0.0001f, batch_size);
//
//  if (!passed)
//  {
//    uo_print_nn(stdout, &nn);
//    return false;
//  }
//
//  float *y_true = uo_alloca(batch_size * sizeof(float));
//
//  for (size_t i = 0; i < 1000; ++i)
//  {
//    uo_nn_select_batch_test_xor(&nn, i, nn.X, y_true);
//    uo_nn_feed_forward(&nn);
//
//    float mse = uo_nn_calculate_loss(&nn, y_true);
//    float rmse = sqrt(mse);
//
//    if (rmse > 0.001)
//    {
//      uo_print_nn(stdout, &nn);
//      return false;
//    }
//  }
//
//  uo_print_nn(stdout, &nn);
//  uo_nn_save_to_file(&nn, filepath);
//  return true;
//}

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
