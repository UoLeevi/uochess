#ifndef UO_NN_H
#define UO_NN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

  typedef struct uo_nn_impl uo_nn_impl;

  typedef struct uo_nn_data
  {
    void *data;
  } uo_nn_data;

  typedef struct uo_nn
  {
    size_t batch_size;
    size_t input_count;
    size_t output_count;
    size_t loss_count;
    uo_nn_data *inputs;
    uo_nn_data *outputs;
    uo_nn_data *true_outputs;
    uo_nn_data *loss;
    uo_nn_impl *impl;
    void *state;
  } uo_nn;

  typedef struct uo_nn_input_half
  {
    union
    {
      bool vector[368 + 2];
      struct
      {
        struct
        {
          bool K[64];
          bool Q[64];
          bool R[64];
          bool B[64];
          bool N[64];
          bool P[48];
        } piece_placement;
        struct
        {
          bool K;
          bool Q;
        } castling;
      } features;
    } mask;

    union
    {
      float vector[5];
      struct
      {
        struct
        {
          float P;
          float N;
          float B;
          float R;
          float Q;
        } material;
      } features;
    } floats;
  } uo_nn_input_half;

  typedef struct uo_nn_input_shared
  {
    union
    {
      bool vector[64 + 8];
      struct
      {
        bool empty_squares[64];
        bool enpassant_file[8];
      } features;
    } mask;
  } uo_nn_input_shared;

  typedef struct uo_nn_position
  {
    uo_nn_input_half halves[2];
    uo_nn_input_shared shared;
  } uo_nn_position;

  uo_nn *uo_nn_create_chess_eval(uo_nn_position *nn_input);

  uo_nn *uo_nn_load_from_file(uo_nn_position *nn_input, char *filepath);

  uo_nn *uo_nn_create_xor(size_t batch_size);

  void uo_nn_save_parameters_to_file(uo_nn *nn, char *filepath);

  void uo_nn_load_parameters_from_file(uo_nn *nn, char *filepath);

  void uo_nn_init_optimizer(uo_nn *nn, ...);

  void uo_nn_set_to_evaluation_mode(uo_nn *nn);

  void uo_nn_forward(uo_nn *nn, ...);

  float uo_nn_compute_loss(uo_nn *nn);

  void uo_nn_backward(uo_nn *nn);

  typedef struct uo_nn uo_nn;

  int16_t uo_nn_evaluate(uo_nn *nn, uint8_t color);

  bool uo_nn_train_eval(char *dataset_filepath, char *nn_init_filepath, char *nn_output_file, float learning_rate, size_t iterations, size_t batch_size);

  bool uo_test_nn_train_xor(char *test_data_dir);

  void uo_nn_generate_dataset(char *dataset_filepath, char *engine_filepath, char *engine_option_commands, size_t position_count);

#ifdef __cplusplus
}
#endif

#endif
