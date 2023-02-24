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
      uint8_t vector[368 + 2];
      struct
      {
        struct
        {
          uint8_t K[64];
          uint8_t Q[64];
          uint8_t R[64];
          uint8_t B[64];
          uint8_t N[64];
          uint8_t P[48];
        } piece_placement;
        struct
        {
          uint8_t K;
          uint8_t Q;
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
      uint8_t vector[64 + 8];
      struct
      {
        uint8_t empty_squares[64];
        uint8_t enpassant_file[8];
      } features;
    } mask;
  } uo_nn_input_shared;

  typedef struct uo_nn_position
  {
    uo_nn_input_half halves[2];
    uo_nn_input_shared shared;
  } uo_nn_position;

  uo_nn *uo_nn_create_xor(size_t batch_size);

  void uo_nn_init_optimizer(uo_nn *nn);

  void uo_nn_forward(uo_nn *nn, ...);

  float uo_nn_compute_loss(uo_nn *nn);

  void uo_nn_backward(uo_nn *nn);

  typedef struct uo_nn uo_nn;

  //typedef void uo_nn_select_batch(uo_nn *nn, size_t iteration, uo_tensor *y_true);
  //typedef void uo_nn_report(uo_nn *nn, size_t iteration, float error, float learning_rate);

  //void uo_nn_load_position(uo_nn *nn, const uo_position *position, size_t index);

  //int16_t uo_nn_evaluate(uo_nn *nn, const uo_position *position);

  uo_nn *uo_nn_read_from_file(uo_nn *nn, char *filepath, size_t batch_size);

  bool uo_nn_train_eval(char *dataset_filepath, char *nn_init_filepath, char *nn_output_file, float learning_rate, size_t iterations, size_t batch_size);

  bool uo_test_nn_train_xor(char *test_data_dir);

  void uo_nn_generate_dataset(char *dataset_filepath, char *engine_filepath, char *engine_option_commands, size_t position_count);

#ifdef __cplusplus
}
#endif

#endif
