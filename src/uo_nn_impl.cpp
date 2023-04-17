#include "uo_nn.h"
#include "uo_def.h"

#include <torch/torch.h>
#include <iostream>
#include <cstdlib>
#include <cstdarg>

struct Model : torch::nn::Module
{
  virtual torch::Tensor forward(va_list args) = 0;
  virtual torch::Tensor loss(torch::Tensor y_pred, torch::Tensor y_true) = 0;
  virtual void init_optimizer(va_list args) = 0;
  torch::optim::Optimizer *optimizer;
};

struct ChessEvalModel : Model {
public:
  ChessEvalModel(uo_nn_position *nn_input) :
    conv1(torch::nn::Conv2dOptions(num_channels, 64, 3).padding(1)),
    conv2(torch::nn::Conv2dOptions(64, 128, 3).padding(1)),
    conv3(torch::nn::Conv2dOptions(128, 256, 3).padding(1)),
    fc1(256 * 8 * 8, 512),
    fc2(512, 1)
  {
    // TODO: somehow make input tensors with different shapes compatible. Maybe pad with zeros


    register_module("conv1", conv1);
    register_module("conv2", conv2);
    register_module("conv3", conv3);
    register_module("fc1", fc1);
    register_module("fc2", fc2);



    size_t n_input_shared_mask = sizeof(nn_input->shared.mask) / sizeof(bool);
    size_t n_hidden_1 = 8;
    size_t n_output = 1;

    size_t n_input_pawn_placement_mask_white = sizeof(nn_input->halves[0].mask.features.piece_placement.P) / sizeof(bool);
    assert(n_input_pawn_placement_mask_white == 6 * 8);
    input_pawn_placement_mask_white = torch::from_blob(&nn_input->halves[uo_white].mask.features.piece_placement.P, { 1, 6, 8 }, torch::kBool);

    size_t n_input_piece_placement_mask_white = sizeof(nn_input->halves[0].mask.features.piece_placement) / sizeof(bool) - n_input_pawn_placement_mask_white;
    assert(n_input_pawn_placement_mask_white == 5 * 8 * 8);
    input_piece_placement_mask_white = torch::from_blob(&nn_input->halves[uo_white].mask.features.piece_placement, { 1, 5, 8, 8 }, torch::kBool);

    size_t n_input_castling_mask_white = sizeof(nn_input->halves[0].mask.features.castling) / sizeof(bool);
    input_castling_mask_white = torch::from_blob(&nn_input->halves[uo_white].mask.features.castling, { 1, n_input_castling_mask_white }, torch::kBool);

    size_t n_input_material_floats_white = sizeof(nn_input->halves[0].floats.features.material) / sizeof(float);
    input_material_floats_white = torch::from_blob(&nn_input->halves[uo_white].floats.vector, { 1, n_input_material_floats_white }, torch::kFloat32);

    size_t n_input_pawn_placement_mask_black = sizeof(nn_input->halves[0].mask.features.piece_placement.P) / sizeof(bool);
    assert(n_input_pawn_placement_mask_black == 6 * 8);
    input_pawn_placement_mask_black = torch::from_blob(&nn_input->halves[uo_black].mask.features.piece_placement.P, { 1, 6, 8 }, torch::kBool);

    size_t n_input_piece_placement_mask_black = sizeof(nn_input->halves[0].mask.features.piece_placement) / sizeof(bool) - n_input_pawn_placement_mask_black;
    assert(n_input_piece_placement_mask_black == 5 * 8 * 8);
    input_piece_placement_mask_black = torch::from_blob(&nn_input->halves[uo_black].mask.features.piece_placement, { 1, 5, 8, 8 }, torch::kBool);

    size_t n_input_castling_mask_black = sizeof(nn_input->halves[0].mask.features.castling) / sizeof(bool);
    input_castling_mask_black = torch::from_blob(&nn_input->halves[uo_black].mask.features.castling, { 1, n_input_castling_mask_black }, torch::kBool);

    size_t n_input_material_floats_black = sizeof(nn_input->halves[0].floats.features.material) / sizeof(float);
    input_material_floats_black = torch::from_blob(&nn_input->halves[uo_black].floats.vector, { 1, n_input_material_floats_black }, torch::kFloat32);

    size_t n_input_empty_squares_mask = sizeof(nn_input->shared.mask.features.empty_squares) / sizeof(bool);
    assert(n_input_empty_squares_mask == 8 * 8);
    input_empty_squares_mask = torch::from_blob(&nn_input->shared.mask.features.empty_squares, { 1, 8, 8 }, torch::kBool);

    size_t n_input_enpassant_file_mask = sizeof(nn_input->shared.mask.features.enpassant_file) / sizeof(bool);
    input_enpassant_file_mask = torch::from_blob(&nn_input->shared.mask.features.enpassant_file, { 1, n_input_enpassant_file_mask }, torch::kBool);

    W_piece_placement_mask_own = register_parameter("W_piece_placement_mask_own", torch::randn_like(input_piece_placement_mask_white));
    W_pawn_placement_mask_own = register_parameter("W_pawn_placement_mask_own", torch::randn_like(input_pawn_placement_mask_white));
    W_castling_mask_own = register_parameter("W_castling_mask_own", torch::randn_like(input_castling_mask_white));

    W_piece_placement_mask_enemy = register_parameter("W_piece_placement_mask_enemy", torch::randn_like(input_piece_placement_mask_black));
    W_pawn_placement_mask_enemy = register_parameter("W_pawn_placement_mask_enemy", torch::randn_like(input_pawn_placement_mask_black));
    W_castling_mask_enemy = register_parameter("W_castling_mask_enemy", torch::randn_like(input_castling_mask_black));

    W_empty_squares_mask = register_parameter("W_empty_squares_mask", torch::randn_like(input_empty_squares_mask));
    W_enpassant_file_mask = register_parameter("W_enpassant_file_mask", torch::randn_like(input_enpassant_file_mask));;

    W_material_floats_own = register_parameter("W_material_floats_own", torch::randn({ n_input_material_floats_white, n_hidden_1 }));
    W_material_floats_enemy = register_parameter("W_material_floats_enemy", torch::randn({ n_input_material_floats_black, n_hidden_1 }));

    b1 = register_parameter("b1", torch::randn(n_hidden_1));

    W2 = register_parameter("W2", torch::randn({ n_hidden_1, n_output }));
    b2 = register_parameter("b2", torch::randn(n_output));
  }

  torch::Tensor forward(va_list args) {
    int color = va_arg(args, int);

    torch::Tensor input_piece_placement_mask_own, input_pawn_placement_mask_own, input_castling_mask_own, input_material_floats_own;
    torch::Tensor input_piece_placement_mask_enemy, input_pawn_placement_mask_enemy, input_castling_mask_enemy, input_material_floats_enemy;

    if (color == uo_white)
    {
      input_piece_placement_mask_own = input_piece_placement_mask_white;
      input_pawn_placement_mask_own = input_pawn_placement_mask_white;
      input_castling_mask_own = input_castling_mask_white;
      input_material_floats_own = input_material_floats_white;
      input_piece_placement_mask_enemy = input_piece_placement_mask_black;
      input_castling_mask_enemy = input_castling_mask_black;
      input_material_floats_enemy = input_material_floats_black;
    }
    else
    {
      input_piece_placement_mask_own = input_piece_placement_mask_black;
      input_pawn_placement_mask_own = input_pawn_placement_mask_black;
      input_castling_mask_own = input_castling_mask_black;
      input_material_floats_own = input_material_floats_black;
      input_piece_placement_mask_enemy = input_piece_placement_mask_white;
      input_castling_mask_enemy = input_castling_mask_white;
      input_material_floats_enemy = input_material_floats_white;
    }

    torch::Tensor x_piece_placement_mask_own = torch::where(input_piece_placement_mask_own, W_piece_placement_mask_own, 0.0f);
    torch::Tensor x_pawn_placement_mask_own = torch::where(input_pawn_placement_mask_own, W_pawn_placement_mask_own, 0.0f);
    torch::Tensor x_castling_mask_own = torch::where(input_castling_mask_own, W_castling_mask_own, 0.0f);
    torch::Tensor x_piece_placement_mask_enemy = torch::where(input_piece_placement_mask_enemy, W_piece_placement_mask_enemy, 0.0f);
    torch::Tensor x_pawn_placement_mask_enemy = torch::where(input_pawn_placement_mask_enemy, W_pawn_placement_mask_enemy, 0.0f);
    torch::Tensor x_castling_mask_enemy = torch::where(input_castling_mask_enemy, W_castling_mask_enemy, 0.0f);


    // (1 x n_hidden_1)
    torch::Tensor x_mask_own_sum = torch::sum(x_mask_own, 0, true);

    // (n_input_half_mask x n_hidden_1)
    torch::Tensor x_mask_enemy = torch::where(input_mask_enemy, W1_mask_enemy, 0.0f);
    // (1 x n_hidden_1)
    torch::Tensor x_mask_enemy_sum = torch::sum(x_mask_enemy, 0, true);

    // (n_input_shared_mask x n_hidden_1)
    torch::Tensor x_mask_shared = torch::where(input_mask_shared, W1_mask_shared, 0.0f);
    // (1 x n_hidden_1)
    torch::Tensor x_mask_shared_sum = torch::sum(x_mask_shared, 0, true);

    torch::Tensor x_material_floats_own = torch::mm(input_floats_own, W1_floats_own);
    // (1 x n_hidden_1)
    torch::Tensor x_material_floats_enemy = torch::mm(input_floats_enemy, W1_floats_enemy);

    torch::Tensor zero = torch::zeros(1, torch::kFloat);
    torch::Tensor x = zero;
    x = torch::add(x, x_mask_own_sum);
    x = torch::add(x, x_mask_enemy_sum);
    x = torch::add(x, x_mask_shared_sum);
    x = torch::add(x, x_floats_own);
    x = torch::add(x, x_floats_enemy);
    x = torch::add(x, b1);
    x = torch::tanh(x);

    x = torch::addmm(b2, x, W2);

    output = torch::tanh(x);

    return output;


            x = torch::relu(conv1(x));
        x = torch::relu(conv2(x));
        x = torch::relu(conv3(x));
        x = x.view({-1, 256 * board_size * board_size});
        x = torch::relu(fc1(x));
        x = fc2(x);
        return x;
  }

  torch::Tensor loss(torch::Tensor y_pred, torch::Tensor y_true)
  {
    torch::nn::MSELoss loss_fn;
    return loss_fn(y_pred, y_true);
  }

  void init_optimizer(va_list args)
  {
    double learning_rate = va_arg(args, double);
    optimizer = new torch::optim::SGD(parameters(), torch::optim::SGDOptions(learning_rate));
  }

  //torch::Tensor W1_mask_own, W1_floats_own;
  //torch::Tensor W1_mask_enemy, W1_floats_enemy;
  //torch::Tensor W1_mask_shared;
  torch::Tensor b1;
  torch::Tensor W2, b2;

  torch::Tensor input_piece_placement_mask_white, input_pawn_placement_mask_white, input_castling_mask_white, input_material_floats_white;
  torch::Tensor input_piece_placement_mask_black, input_pawn_placement_mask_black, input_castling_mask_black, input_material_floats_black;
  torch::Tensor input_empty_squares_mask;
  torch::Tensor input_enpassant_file_mask;

  torch::Tensor W_piece_placement_mask_own, W_pawn_placement_mask_own, W_castling_mask_own, W_material_floats_own;
  torch::Tensor W_piece_placement_mask_enemy, W_pawn_placement_mask_enemy, W_castling_mask_enemy, W_material_floats_enemy;
  torch::Tensor W_empty_squares_mask;
  torch::Tensor W_enpassant_file_mask;

  //torch::Tensor input_mask_white, input_floats_white;
  //torch::Tensor input_mask_black, input_floats_black;
  //torch::Tensor input_mask_shared;

  torch::Tensor output;


  torch::nn::Conv2d conv1;
  torch::nn::Conv2d conv2;
  torch::nn::Conv2d conv3;
  torch::nn::Linear fc1;
  torch::nn::Linear fc2;
};

struct XORModel : Model {
public:
  XORModel(size_t batch_size)
  {
    size_t n_input = 2;
    size_t n_hidden_1 = 8;
    size_t n_output = 1;

    auto options = torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU);

    input = torch::zeros({ batch_size, n_input }, options);
    output = torch::empty({ batch_size, n_output }, options);

    W1 = register_parameter("W1", torch::randn({ n_input, n_hidden_1 }));
    b1 = register_parameter("b1", torch::randn(n_hidden_1));

    W2 = register_parameter("W2", torch::randn({ n_hidden_1, n_output }));
    b2 = register_parameter("b2", torch::randn(n_output));
  }

  torch::Tensor forward(va_list args) {
    torch::Tensor x = torch::addmm(b1, input, W1);
    x = torch::relu(x);
    x = torch::addmm(b2, x, W2);

    torch::sigmoid_out(output, x);
    return output;
  }

  torch::Tensor loss(torch::Tensor y_pred, torch::Tensor y_true)
  {
    torch::nn::BCELoss loss_fn;
    return loss_fn(y_pred, y_true);
  }

  void init_optimizer(va_list args)
  {
    double learning_rate = va_arg(args, double);
    optimizer = new torch::optim::SGD(parameters(), torch::optim::SGDOptions(learning_rate));
  }

  torch::Tensor W1, b1;
  torch::Tensor W2, b2;

  torch::Tensor input;
  torch::Tensor output;
};

typedef struct uo_nn_impl
{
  torch::Tensor *input_tensors;
  torch::Tensor *output_tensors;
  torch::Tensor *true_output_tensors;
  torch::Tensor *loss_tensors;
  Model *model;
} uo_nn_impl;

void uo_nn_set_to_evaluation_mode(uo_nn *nn)
{
  nn->impl->model->eval();
}

void uo_nn_save_parameters_to_file(uo_nn *nn, char *filepath)
{
  // Save model state
  Model *model = nn->impl->model;
  torch::serialize::OutputArchive output_model_archive;
  model->to(torch::kCPU);
  model->save(output_model_archive);
  output_model_archive.save_to(filepath);

  //// Save optimizer state
  //torch::optim::Optimizer *optimizer = model->optimizer;
  //torch::serialize::OutputArchive optimizer_output_archive;
  //optimizer->save(optimizer_output_archive);
  //optimizer_output_archive.save_to(optim_state_dict_path);
}

void uo_nn_load_parameters_from_file(uo_nn *nn, char *filepath)
{
  // Load model state
  Model *model = nn->impl->model;
  torch::serialize::InputArchive model_input_archive;
  model_input_archive.load_from(filepath);
  model->load(model_input_archive);

  //// Load optimizer state
  //torch::optim::Optimizer *optimizer = model->optimizer;
  //torch::serialize::InputArchive optimizer_input_archive;
  //optimizer_input_archive.load_from(optimizer_filepath);
  //optimizer->load(optimizer_input_archive);
}

uo_nn *uo_nn_create_chess_eval(uo_nn_position *nn_input)
{
  uo_nn *nn = new uo_nn();
  nn->batch_size = 1;

  uo_nn_impl *impl = nn->impl = new uo_nn_impl();
  impl->model = new ChessEvalModel(nn_input);

  auto options = torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU);

  nn->input_count = 0;

  nn->output_count = 1;
  impl->output_tensors = new torch::Tensor[nn->output_count];
  impl->true_output_tensors = new torch::Tensor[nn->output_count];
  nn->outputs = new uo_nn_data[nn->output_count];
  nn->true_outputs = new uo_nn_data[nn->output_count];
  new (&impl->true_output_tensors[0]) torch::Tensor(torch::zeros({ 1, 1 }, options));
  nn->true_outputs[0].data = impl->true_output_tensors[0].data_ptr();

  nn->loss_count = 1;
  impl->loss_tensors = new torch::Tensor[nn->loss_count];
  nn->loss = new uo_nn_data[nn->loss_count];

  return nn;
}

uo_nn *uo_nn_create_xor(size_t batch_size)
{
  uo_nn *nn = new uo_nn();
  nn->batch_size = batch_size;

  uo_nn_impl *impl = nn->impl = new uo_nn_impl();
  XORModel *model = new XORModel(batch_size);
  impl->model = model;

  auto options = torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU);

  nn->input_count = 1;
  impl->input_tensors = new torch::Tensor[nn->input_count];
  impl->input_tensors[0] = model->input;
  nn->inputs = new uo_nn_data[nn->input_count];
  nn->inputs[0].data = impl->input_tensors[0].data_ptr();

  nn->output_count = 1;
  impl->output_tensors = new torch::Tensor[nn->output_count];
  impl->output_tensors[0] = model->output;

  impl->true_output_tensors = new torch::Tensor[nn->output_count];
  nn->outputs = new uo_nn_data[nn->output_count];
  nn->true_outputs = new uo_nn_data[nn->output_count];
  new (&impl->true_output_tensors[0]) torch::Tensor(torch::zeros({ batch_size, 1 }, options));
  nn->true_outputs[0].data = impl->true_output_tensors[0].data_ptr();

  nn->loss_count = 1;
  impl->loss_tensors = new torch::Tensor[nn->loss_count];
  nn->loss = new uo_nn_data[nn->loss_count];

  return nn;
}

void uo_nn_init_optimizer(uo_nn *nn, ...)
{
  va_list args;
  va_start(args, nn);
  nn->impl->model->init_optimizer(args);
  va_end(args);
}

void uo_nn_forward(uo_nn *nn, ...)
{
  va_list args;
  va_start(args, nn);

  nn->impl->output_tensors[0] = nn->impl->model->forward(args);
  nn->outputs[0].data = nn->impl->output_tensors[0].data_ptr();

  va_end(args);
}

float uo_nn_compute_loss(uo_nn *nn)
{
  torch::Tensor loss = nn->impl->loss_tensors[0] = nn->impl->model->loss(nn->impl->output_tensors[0], nn->impl->true_output_tensors[0]);
  nn->loss[0].data = loss.data_ptr();
  return loss.item<float>();
}

void uo_nn_backward(uo_nn *nn)
{
  nn->impl->model->optimizer->zero_grad();
  nn->impl->loss_tensors[0].backward();
  nn->impl->model->optimizer->step();
}

