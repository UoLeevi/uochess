#include "uo_nn.h"
#include "uo_math.h"
#include "uo_misc.h"
#include "uo_global.h"
#include "uo_util.h"
#include "uo_engine.h"

#include <tensorflow/c/c_api.h>

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

typedef struct uo_nn
{
  uo_nn_node **graph;
  size_t graph_size;
  uo_nn_node **inputs;
  size_t input_count;
  uo_nn_node **outputs;
  size_t output_count;
  uo_nn_adam_params **initializers;
  size_t initializer_count;
  void *state;

  struct
  {
    TF_Session *session;
    TF_Graph *graph;
    TF_Status *status;

    TF_Output input;
    TF_Output target;
    TF_Output output;

    //TF_Operation *init_op;
    //TF_Operation *train_op;
    //TF_Operation *save_op;
    //TF_Operation *restore_op;

    //TF_Output checkpoint_file;
  } tf;
} uo_nn;

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

//void uo_nn_load_position(uo_nn *nn, const uo_position *position, size_t index)
//{
//  size_t n_X = nn->n_X;
//  float *input = nn->X + n_X * index;
//
//  uo_nn_position *position_input = (uo_nn_position *)(void *)input;
//  memset(position_input, 0, sizeof(uo_nn_position));
//
//  // Step 1. Piece configuration
//
//  for (uo_square square = 0; square < 64; ++square)
//  {
//    uo_piece piece = position->board[square];
//
//    if (piece <= 1)
//    {
//      // Empty square
//      position_input->data.empty[square] = 1.0f;
//      continue;
//    }
//
//    switch (piece)
//    {
//      case uo_piece__P:
//        position_input->data.piece_placement.own.P[square] = 1.0f;
//        position_input->data.material.own.P += 1.0f;
//        break;
//
//      case uo_piece__N:
//        position_input->data.piece_placement.own.N[square] = 1.0f;
//        position_input->data.material.own.N += 1.0f;
//        break;
//
//      case uo_piece__B:
//        position_input->data.piece_placement.own.B[square] = 1.0f;
//        position_input->data.material.own.B += 1.0f;
//        break;
//
//      case uo_piece__R:
//        position_input->data.piece_placement.own.R[square] = 1.0f;
//        position_input->data.material.own.R += 1.0f;
//        break;
//
//      case uo_piece__Q:
//        position_input->data.piece_placement.own.Q[square] = 1.0f;
//        position_input->data.material.own.Q += 1.0f;
//        break;
//
//      case uo_piece__K:
//        position_input->data.piece_placement.own.K[square] = 1.0f;
//        break;
//
//      case uo_piece__p:
//        position_input->data.piece_placement.enemy.P[square] = 1.0f;
//        position_input->data.material.enemy.P += 1.0f;
//        break;
//
//      case uo_piece__n:
//        position_input->data.piece_placement.enemy.N[square] = 1.0f;
//        position_input->data.material.enemy.N += 1.0f;
//        break;
//
//      case uo_piece__b:
//        position_input->data.piece_placement.enemy.B[square] = 1.0f;
//        position_input->data.material.enemy.B += 1.0f;
//        break;
//
//      case uo_piece__r:
//        position_input->data.piece_placement.enemy.R[square] = 1.0f;
//        position_input->data.material.enemy.R += 1.0f;
//        break;
//
//      case uo_piece__q:
//        position_input->data.piece_placement.enemy.Q[square] = 1.0f;
//        position_input->data.material.enemy.Q += 1.0f;
//        break;
//
//      case uo_piece__k:
//        position_input->data.piece_placement.enemy.K[square] = 1.0f;
//        break;
//    }
//  }
//
//  // Step 2. Castling
//  position_input->data.castling.own_K = uo_position_flags_castling_OO(position->flags);
//  position_input->data.castling.own_Q = uo_position_flags_castling_OOO(position->flags);
//  position_input->data.castling.enemy_K = uo_position_flags_castling_enemy_OO(position->flags);
//  position_input->data.castling.enemy_Q = uo_position_flags_castling_enemy_OOO(position->flags);
//
//  // Step 3. Enpassant
//  size_t enpassant_file = uo_position_flags_enpassant_file(position->flags);
//
//  if (enpassant_file)
//  {
//    position_input->data.enpassant[enpassant_file - 1] = 1.0f;
//  }
//}

//int8_t uo_nn_load_fen(uo_nn *nn, const char *fen, size_t index)
//{
//  char piece_placement[72];
//  char active_color;
//  char castling[5];
//  char enpassant[3];
//  int rule50;
//  int fullmove;
//
//  int count = sscanf(fen, "%71s %c %4s %2s %d %d",
//    piece_placement, &active_color,
//    castling, enpassant,
//    &rule50, &fullmove);
//
//  if (count != 6)
//  {
//    return -1;
//  }
//
//  size_t n_X = nn->n_X;
//  float *input = nn->X + n_X * index;
//
//  uo_nn_position *position_input = (uo_nn_position *)(void *)input;
//  memset(position_input, 0, sizeof(uo_nn_position));
//
//  uint8_t color = active_color == 'b' ? uo_black : uo_white;
//  size_t flip_if_black = color == uo_black ? 56 : 0;
//
//  // Step 1. Piece placement
//
//  char *ptr = piece_placement;
//
//  char c;
//
//  // loop by rank
//  for (int i = 7; i >= 0; --i)
//  {
//    // loop by file
//    for (int j = 0; j < 8; ++j)
//    {
//      uo_square square = (i * 8 + j) ^ flip_if_black;
//
//      c = *ptr++;
//
//      // empty squares
//      if (c > '0' && c <= ('8' - j))
//      {
//        size_t empty_count = c - '0';
//        j += empty_count - 1;
//
//        while (empty_count--)
//        {
//          position_input->data.empty[square + empty_count] = 1.0f;
//        }
//
//        continue;
//      }
//
//      uo_piece piece = uo_piece_from_char(c);
//
//      if (!piece)
//      {
//        return -1;
//      }
//
//      piece ^= color;
//
//      switch (piece)
//      {
//        case uo_piece__P:
//          position_input->data.piece_placement.own.P[square] = 1.0f;
//          position_input->data.material.own.P += 1.0f;
//          break;
//
//        case uo_piece__N:
//          position_input->data.piece_placement.own.N[square] = 1.0f;
//          position_input->data.material.own.N += 1.0f;
//          break;
//
//        case uo_piece__B:
//          position_input->data.piece_placement.own.B[square] = 1.0f;
//          position_input->data.material.own.B += 1.0f;
//          break;
//
//        case uo_piece__R:
//          position_input->data.piece_placement.own.R[square] = 1.0f;
//          position_input->data.material.own.R += 1.0f;
//          break;
//
//        case uo_piece__Q:
//          position_input->data.piece_placement.own.Q[square] = 1.0f;
//          position_input->data.material.own.Q += 1.0f;
//          break;
//
//        case uo_piece__K:
//          position_input->data.piece_placement.own.K[square] = 1.0f;
//          break;
//
//        case uo_piece__p:
//          position_input->data.piece_placement.enemy.P[square] = 1.0f;
//          position_input->data.material.enemy.P += 1.0f;
//          break;
//
//        case uo_piece__n:
//          position_input->data.piece_placement.enemy.N[square] = 1.0f;
//          position_input->data.material.enemy.N += 1.0f;
//          break;
//
//        case uo_piece__b:
//          position_input->data.piece_placement.enemy.B[square] = 1.0f;
//          position_input->data.material.enemy.B += 1.0f;
//          break;
//
//        case uo_piece__r:
//          position_input->data.piece_placement.enemy.R[square] = 1.0f;
//          position_input->data.material.enemy.R += 1.0f;
//          break;
//
//        case uo_piece__q:
//          position_input->data.piece_placement.enemy.Q[square] = 1.0f;
//          position_input->data.material.enemy.Q += 1.0f;
//          break;
//
//        case uo_piece__k:
//          position_input->data.piece_placement.enemy.K[square] = 1.0f;
//          break;
//      }
//    }
//
//    c = *ptr++;
//
//    if (i != 0 && c != '/')
//    {
//      return -1;
//    }
//  }
//
//  // Step 2. Castling rights
//
//  ptr = castling;
//  c = *ptr++;
//
//  if (c == '-')
//  {
//    c = *ptr++;
//  }
//  else
//  {
//    if (color == uo_white)
//    {
//      if (c == 'K')
//      {
//        position_input->data.castling.own_K = 1.0f;
//        c = *ptr++;
//      }
//
//      if (c == 'Q')
//      {
//        position_input->data.castling.own_Q = 1.0f;
//        c = *ptr++;
//      }
//
//      if (c == 'k')
//      {
//        position_input->data.castling.enemy_K = 1.0f;
//        c = *ptr++;
//      }
//
//      if (c == 'q')
//      {
//        position_input->data.castling.enemy_Q = 1.0f;
//        c = *ptr++;
//      }
//    }
//    else
//    {
//      if (c == 'K')
//      {
//        position_input->data.castling.enemy_K = 1.0f;
//        c = *ptr++;
//      }
//
//      if (c == 'Q')
//      {
//        position_input->data.castling.enemy_Q = 1.0f;
//        c = *ptr++;
//      }
//
//      if (c == 'k')
//      {
//        position_input->data.castling.own_K = 1.0f;
//        c = *ptr++;
//      }
//
//      if (c == 'q')
//      {
//        position_input->data.castling.own_Q = 1.0f;
//        c = *ptr++;
//      }
//    }
//  }
//
//  // Step 3. Enpassant
//
//  ptr = enpassant;
//  c = *ptr++;
//
//  if (c != '-')
//  {
//    uint8_t file = c - 'a';
//    if (file < 0 || file >= 8)
//    {
//      return -1;
//    }
//
//    c = *ptr++;
//    uint8_t rank = c - '1';
//    if (rank < 0 || rank >= 8)
//    {
//      return -1;
//    }
//
//    position_input->data.enpassant[file] = 1.0f;
//  }
//
//  return color;
//}

//// see: http://ufldl.stanford.edu/tutorial/supervised/DebuggingGradientChecking/
//bool uo_nn_check_gradients(uo_nn *nn, uo_nn_layer *layer, float *d, float *y_true)
//{
//  size_t m;
//  size_t n;
//  float *val;
//  size_t feed_forward_layer_index = layer - nn->layers;
//
//  bool is_output_layer = layer->A == nn->y;
//  // For hidden layers, ignore bias terms when computing derivatives
//  int bias_offset = is_output_layer ? 0 : 1;
//
//  if (d == layer->dW_t)
//  {
//    val = layer->W_t;
//    m = layer->n_W;
//    n = layer->m_W;
//    bias_offset = 0;
//  }
//  else if (d == layer->dA)
//  {
//    val = layer->A;
//    m = nn->batch_size;
//    n = layer->n_W;
//    ++feed_forward_layer_index;
//  }
//  else if (d == layer->dZ)
//  {
//    val = layer->Z;
//    m = nn->batch_size;
//    n = layer->n_W;
//    ++feed_forward_layer_index;
//  }
//  else
//  {
//    return false;
//  }
//
//  bool passed = true;
//  float epsilon = 1e-4f;
//
//  for (size_t i = 0; i < m; ++i)
//  {
//    for (size_t j = 0; j < n; ++j)
//    {
//      size_t index = i * (n + bias_offset) + j;
//      float val_ij = val[index];
//
//      // Step 1. Add epsilon and feed forward
//
//      val[index] = val_ij + epsilon;
//
//      if (val == layer->Z && layer->func.activation.f)
//      {
//        bool is_output_layer = layer - nn->layers == nn->layer_count;
//        // For hidden layers, let's leave room for bias terms when computing output matrix
//        int bias_offset = is_output_layer ? 0 : 1;
//        size_t n_W = layer->n_W;
//        size_t n_A = n_W + bias_offset;
//        float *Z = layer->Z;
//        float *A = layer->A;
//        uo_vec_mapfunc_ps(Z, A, nn->batch_size * n_A, layer->func.activation.f);
//      }
//
//      for (size_t layer_index = feed_forward_layer_index; layer_index <= nn->layer_count; ++layer_index)
//      {
//        uo_nn_layer_feed_forward(nn, nn->layers + layer_index);
//      }
//
//      float loss_plus = uo_nn_calculate_loss(nn, y_true);
//
//      // Step 2. Subtract epsilon and feed forward
//
//      val[index] = val_ij - epsilon;
//
//      if (val == layer->Z && layer->func.activation.f)
//      {
//        bool is_output_layer = layer - nn->layers == nn->layer_count;
//        // For hidden layers, let's leave room for bias terms when computing output matrix
//        int bias_offset = is_output_layer ? 0 : 1;
//        size_t n_W = layer->n_W;
//        size_t n_A = n_W + bias_offset;
//        float *Z = layer->Z;
//        float *A = layer->A;
//        uo_vec_mapfunc_ps(Z, A, nn->batch_size * n_A, layer->func.activation.f);
//      }
//
//      for (size_t layer_index = feed_forward_layer_index; layer_index <= nn->layer_count; ++layer_index)
//      {
//        uo_nn_layer_feed_forward(nn, nn->layers + layer_index);
//      }
//
//      float loss_minus = uo_nn_calculate_loss(nn, y_true);
//
//      // Step 3. Restore previous value
//
//      val[index] = val_ij;
//
//      // Step 4. Compute numeric gradient and compare to calculated gradient
//
//      float grad_num = (loss_plus - loss_minus) / (2.0f * epsilon);
//      float grad_calc = d[index];
//
//      float diff = grad_num - grad_calc;
//      if (diff < 0.0f) diff = -diff;
//
//      passed &= diff < 1e-2;
//    }
//  }
//
//  return passed;
//}

//bool uo_nn_train(uo_nn *nn, uo_nn_select_batch *select_batch, float error_threshold, size_t error_sample_size, size_t max_iterations, uo_nn_report *report, size_t report_interval, float learning_rate, size_t batch_size)
//{
//  if (batch_size && nn->batch_size != batch_size)
//  {
//    uo_nn_change_batch_size(nn, batch_size);
//  }
//
//  char *nn_filepath = NULL;
//
//  size_t output_size = nn->batch_size * nn->n_y;
//  float avg_error, prev_avg_err, min_avg_err;
//  const size_t adam_reset_count_threshold = 6;
//  size_t adam_reset_counter = 0;
//
//  float *y_true = malloc(output_size * sizeof(float));
//
//  select_batch(nn, 0, nn->X, y_true);
//  uo_nn_feed_forward(nn);
//
//  float loss = uo_nn_calculate_loss(nn, y_true);
//
//  if (isnan(loss))
//  {
//    free(y_true);
//    return false;
//  }
//
//  float lr_multiplier = learning_rate > 0.0f ? learning_rate / uo_nn_adam_learning_rate : 1.0f;
//  prev_avg_err = min_avg_err = avg_error = loss;
//
//  for (size_t i = 1; i < max_iterations; ++i)
//  {
//    select_batch(nn, i, nn->X, y_true);
//    uo_nn_feed_forward(nn);
//
//    float loss = uo_nn_calculate_loss(nn, y_true);
//
//    if (isnan(loss))
//    {
//      free(y_true);
//      return false;
//    }
//
//    avg_error -= avg_error / error_sample_size;
//    avg_error += loss / error_sample_size;
//
//    if (i > error_sample_size && avg_error < error_threshold)
//    {
//      report(nn, i, avg_error, lr_multiplier * uo_nn_adam_learning_rate);
//      free(y_true);
//      return true;
//    }
//
//    if (i % report_interval == 0)
//    {
//      report(nn, i, avg_error, lr_multiplier * uo_nn_adam_learning_rate);
//
//      if (avg_error > prev_avg_err)
//      {
//        ++adam_reset_counter;
//
//        if (adam_reset_counter == adam_reset_count_threshold)
//        {
//          // Reset adam weights if loss keeps increasing
//          prev_avg_err = avg_error = loss;
//          adam_reset_counter = 0;
//          nn->adam.t = 1;
//          for (size_t layer_index = 1; layer_index < nn->layer_count; ++layer_index)
//          {
//            uo_nn_layer *layer = nn->layers + layer_index;
//            memset(layer->adam.m, 0, layer->m_W * layer->n_W * sizeof(float));
//          }
//
//          lr_multiplier *= 0.75f;
//          lr_multiplier = uo_max(lr_multiplier, 1e-10);
//        }
//      }
//      else
//      {
//        adam_reset_counter = 0;
//
//        if (avg_error < min_avg_err)
//        {
//          lr_multiplier *= 1.025f;
//          min_avg_err = avg_error;
//
//          if (*engine_options.nn_dir)
//          {
//            free(nn_filepath);
//            char timestamp[13];
//            uo_timestr(timestamp);
//            nn_filepath = uo_aprintf("%s/nn-eval-%s.nnuo", engine_options.nn_dir, timestamp);
//            uo_nn_save_to_file(nn, nn_filepath);
//          }
//        }
//      }
//
//      prev_avg_err = avg_error;
//    }
//
//    uo_nn_backprop(nn, y_true, lr_multiplier);
//  }
//
//  if (nn_filepath)
//  {
//    // Load best version from file
//    uo_nn_free(nn);
//    uo_nn_read_from_file(nn, nn_filepath, batch_size);
//    free(nn_filepath);
//  }
//
//  free(y_true);
//  return false;
//}

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
//  //uo_nn nn2;
//  //uo_nn_read_from_file(&nn2, tempnnfile, batch_size);
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
//    //uo_nn_load_position(&nn2, &position, j);
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
//  //assert(memcmp(nn->X, nn2.X, nn2.batch_size * nn2.n_X * sizeof(float)) == 0);
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

  //uo_rand_init(time(NULL));

  //if (!batch_size) batch_size = 0x100;

  //uo_nn_eval_state state = {
  //  .file_mmap = file_mmap,
  //  .buf_size = batch_size * 100,
  //  .buf = malloc(batch_size * 100),
  //};

  //allocated_mem[allocated_mem_count++] = state.buf;

  //uo_nn nn;

  //if (nn_init_filepath)
  //{
  //  //uo_nn_read_from_file(&nn, nn_init_filepath, batch_size);
  //}
  //else
  //{
  //  uo_rand_init(time(NULL));

  //  nn.input_count = 5;
  //  nn.inputs = uo_alloca(nn.input_count * sizeof(uo_tensor *));

  //  nn.output_count = 1;
  //  nn.outputs = uo_alloca(nn.output_count * sizeof(uo_nn_node *));

  //  nn.initializer_count = 4;
  //  nn.initializers = uo_alloca(nn.initializer_count * sizeof(uo_nn_node *));

  //  // Inputs
  //  uo_tensor *X_own_material = nn.inputs[0] = uo_tensor_create('s', 2, (size_t[]) { batch_size, 5 });
  //  uo_nn_node *x_own_material = uo_nn_node_create(X_own_material, NULL, 0, 0);

  //  uo_tensor *X_own_mask = nn.inputs[1] = uo_tensor_create('u', 2, (size_t[]) { batch_size, 370 });
  //  uo_nn_node *x_own_mask = uo_nn_node_create(X_own_mask, NULL, 0, 0);

  //  uo_tensor *X_enemy_material = nn.inputs[2] = uo_tensor_create('s', 2, (size_t[]) { batch_size, 5 });
  //  uo_nn_node *x_enemy_material = uo_nn_node_create(X_enemy_material, NULL, 0, 0);

  //  uo_tensor *X_enemy_mask = nn.inputs[3] = uo_tensor_create('u', 2, (size_t[]) { batch_size, 370 });
  //  uo_nn_node *x_enemy_mask = uo_nn_node_create(X_enemy_mask, NULL, 0, 0);

  //  uo_tensor *X_shared_mask = nn.inputs[4] = uo_tensor_create('u', 2, (size_t[]) { batch_size, 72 });
  //  uo_nn_node *x_shared_mask = uo_nn_node_create(X_shared_mask, NULL, 0, 0);

  //  // Layer 1
  //  uo_tensor *W1_material = uo_tensor_create('s', 2, (size_t[]) { X_own_material->dim_sizes[1], 4 });
  //  uo_tensor_set_rand_s(W1_material, 0, 0, W1_material->element_count, -0.5, 0.5);
  //  uo_nn_node *w1_material = uo_nn_node_create(W1_material, NULL, 0, 0);
  //  uo_nn_adam_params *w1_material_adam = nn.initializers[0] = uo_nn_node_adam_params_create(w1_material);

  //  uo_tensor *W1_mask = uo_tensor_create('s', 2, (size_t[]) { X_own_mask->dim_sizes[1], 32 });
  //  uo_tensor_set_rand_s(W1_mask, 0, 0, W1_mask->element_count, -0.5, 0.5);
  //  uo_nn_node *w1_mask = uo_nn_node_create(W1_mask, NULL, 0, 0);
  //  uo_nn_adam_params *w1_mask_adam = nn.initializers[0] = uo_nn_node_adam_params_create(w1_mask);

  //  uo_tensor *W1_shared_mask = uo_tensor_create('s', 2, (size_t[]) { X_shared_mask->dim_sizes[1], 4 });
  //  uo_tensor_set_rand_s(W1_shared_mask, 0, 0, W1_shared_mask->element_count, -0.5, 0.5);
  //  uo_nn_node *w1_shared_mask = uo_nn_node_create(W1_shared_mask, NULL, 0, 0);
  //  uo_nn_adam_params *w1_shared_mask_adam = nn.initializers[0] = uo_nn_node_adam_params_create(w1_shared_mask);

  //  uo_nn_node *xw1_own_material = uo_nn_node_op_gemm(x_own_material, w1_material, 1.0, 1.0, false, false);
  //  uo_nn_node *xw1_enemy_material = uo_nn_node_op_gemm(x_enemy_material, w1_material, -1.0, 1.0, false, false);

  //  uo_nn_node *xw1_own_piece_placement = uo_nn_node_op_gemm_a_mask(x_own_mask, w1_mask, 1.0, 1.0, false, false);
  //  uo_nn_node *xw1_enemy_piece_placement = uo_nn_node_op_gemm_a_mask(x_enemy_mask, w1_mask, -1.0, 1.0, false, false);

  //  uo_nn_node *xw1_shared = uo_nn_node_op_gemm_a_mask(x_shared_mask, w1_shared_mask, 1.0, 1.0, false, false);

  //  uo_nn_node *xw1 = uo_nn_node_op_concat(1, 5, (uo_nn_node * []) {
  //    xw1_own_material,
  //      xw1_enemy_material,
  //      xw1_own_piece_placement,
  //      xw1_enemy_piece_placement,
  //      xw1_shared
  //  });

  //  uo_tensor *B1 = uo_tensor_create('s', 2, (size_t[]) { 1, xw1->tensor->dim_sizes[1] });
  //  uo_tensor_set_rand_s(B1, 0, 0, B1->element_count, -0.5, 0.5);
  //  uo_nn_node *b1 = uo_nn_node_create(B1, NULL, 0, 0);
  //  uo_nn_adam_params *b1_adam = nn.initializers[1] = uo_nn_node_adam_params_create(b1);

  //  uo_nn_node *z1 = uo_nn_node_op_add(xw1, b1);
  //  uo_nn_node *a1 = uo_nn_node_op_tanh(z1);

  //  // Layer 2
  //  uo_tensor *W2 = uo_tensor_create('s', 2, (size_t[]) { a1->tensor->dim_sizes[1], 1 });
  //  uo_tensor_set_rand_s(W2, 0, 0, W2->element_count, -0.5, 0.5);
  //  uo_nn_node *w2 = uo_nn_node_create(W2, NULL, 0, 0);
  //  uo_nn_adam_params *w2_adam = nn.initializers[2] = uo_nn_node_adam_params_create(w2);

  //  uo_nn_node *xw2 = uo_nn_node_op_matmul(a1, w2);

  //  uo_tensor *B2 = uo_tensor_create('s', 2, (size_t[]) { 1, xw2->tensor->dim_sizes[1] });
  //  uo_tensor_set_rand_s(B2, 0, 0, B2->element_count, -0.5, 0.5);
  //  uo_nn_node *b2 = uo_nn_node_create(B2, NULL, 0, 0);
  //  uo_nn_adam_params *b2_adam = nn.initializers[3] = uo_nn_node_adam_params_create(b2);

  //  uo_nn_node *z2 = uo_nn_node_op_add(xw2, b2);
  //  uo_nn_node *a2 = uo_nn_node_op_tanh(z2);

  //  uo_nn_node *y_pred = nn.outputs[0] = a2;

  //  nn.graph_size = 100; // max size
  //  nn.graph = uo_nn_node_create_graph(y_pred, &nn.graph_size);
  //}

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

//void uo_nn_select_batch_test_xor(uo_nn *nn, size_t iteration, uo_tensor *y_true)
//{
//  uo_tensor *X = *nn->inputs;
//
//  for (size_t j = 0; j < X->dim_sizes[0]; ++j)
//  {
//    float x0 = uo_rand_percent() > 0.5f ? 1.0 : 0.0;
//    float x1 = uo_rand_percent() > 0.5f ? 1.0 : 0.0;
//    float y = (int)x0 ^ (int)x1;
//    y_true->data.s[j] = y;
//    X->data.s[j * X->dim_sizes[1]] = x0;
//    X->data.s[j * X->dim_sizes[1] + 1] = x1;
//  }
//}

void uo_nn_report_test_xor(uo_nn *nn, size_t iteration, float error, float learning_rate)
{
  printf("iteration: %zu, error: %g, learning_rate: %g\n", iteration, error, learning_rate);
}

bool uo_test_nn_train_xor(char *test_data_dir)
{
  if (!test_data_dir) return false;

  void *allocated_mem[1];
  size_t allocated_mem_count = 0;

  char *filepath = uo_aprintf("%s/model_xor.json", engine_options.nn_dir);
  allocated_mem[allocated_mem_count++] = filepath;

  uo_rand_init(time(NULL));

  size_t batch_size = 256;

  uo_nn nn;

  // see: https://gist.github.com/asimshankar/7c9f8a9b04323e93bb217109da8c7ad2

  {
    // Initialize a new TensorFlow session
    TF_Graph *graph = nn.tf.graph = TF_NewGraph();
    TF_SessionOptions *options = TF_NewSessionOptions();
    nn.tf.status = TF_NewStatus();
    TF_Session *session = nn.tf.session = TF_NewSession(graph, options, nn.tf.status);
    TF_DeleteSessionOptions(options);
    if (TF_GetCode(nn.tf.status) != TF_OK) return false;
  }

  {
    // Import graph
    uo_file_mmap *file_mmap = uo_file_mmap_open_read(filepath);
    if (!file_mmap) return false;
    TF_Buffer *graph_def = TF_NewBufferFromString(file_mmap->ptr, file_mmap->size);
    uo_file_mmap_close(file_mmap);
    if (graph_def == NULL) return 0;
    TF_ImportGraphDefOptions *options = TF_NewImportGraphDefOptions();
    TF_GraphImportGraphDef(nn.tf.graph, graph_def, options, nn.tf.status);
    TF_DeleteImportGraphDefOptions(options);
    TF_DeleteBuffer(graph_def);
    if (TF_GetCode(nn.tf.status) != TF_OK) return false;
  }

  {
    // Handles to the interesting operations in the graph.
    nn.tf.input.oper = TF_GraphOperationByName(nn.tf.graph, "input");
    nn.tf.input.index = 0;
    nn.tf.target.oper = TF_GraphOperationByName(nn.tf.graph, "target");
    nn.tf.target.index = 0;
    nn.tf.output.oper = TF_GraphOperationByName(nn.tf.graph, "output");
    nn.tf.output.index = 0;

    //nn.tf.init_op = TF_GraphOperationByName(nn.tf.graph, "init");
    //nn.tf.train_op = TF_GraphOperationByName(nn.tf.graph, "train");
    //nn.tf.save_op = TF_GraphOperationByName(nn.tf.graph, "save/control_dependency");
    //nn.tf.restore_op = TF_GraphOperationByName(nn.tf.graph, "save/restore_all");

    //nn.tf.checkpoint_file.oper = TF_GraphOperationByName(nn.tf.graph, "save/Const");
    //nn.tf.checkpoint_file.index = 0;
  }

  {
    //// Initialize model
    //TF_SessionRun(nn.tf.session, NULL,
    //  /* No inputs */
    //  NULL, NULL, 0,
    //  /* No outputs */
    //  NULL, NULL, 0,
    //  /* Just the init operation */
    //  nn.tf.init_op, 1,
    //  /* No metadata */
    //  NULL, nn.tf.status);

    //if (TF_GetCode(nn.tf.status) != TF_OK) return false;
  }

  //uo_nn_reset(&nn);
  //uo_nn_select_batch_test_xor(&nn, 0, y_true);
  //uo_nn_forward(&nn);
  //float loss_avg = uo_nn_loss_mse(y_pred, y_true->data.s);

  //for (size_t i = 1; i < 1000000; ++i)
  //{
  //  uo_nn_reset(&nn);
  //  uo_nn_select_batch_test_xor(&nn, i, y_true);
  //  uo_nn_forward(&nn);

  //  float loss = uo_nn_loss_mse(y_pred, y_true->data.s);

  //  loss_avg = loss_avg * 0.95 + loss * 0.05;

  //  if (loss_avg < 0.0001) break;

  //  uo_nn_loss_grad_mse(y_pred, y_true->data.s);
  //  uo_nn_backward(&nn);
  //  uo_nn_update_initializers(&nn);
  //}

  //bool passed = loss_avg < 0.0001;

  //if (!passed)
  //{
  //  //uo_print_nn(stdout, &nn);
  //  return false;
  //}

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

