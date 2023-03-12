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
  ChessEvalModel(uo_nn_position *nn_input)
  {
    size_t n_input_half_mask = sizeof(nn_input->halves[0].mask) / sizeof(uint8_t);
    size_t n_input_half_floats = sizeof(nn_input->halves[0].floats) / sizeof(float);
    size_t n_input_shared_mask = sizeof(nn_input->shared.mask) / sizeof(uint8_t);
    size_t n_hidden_1 = 8;
    size_t n_output = 1;

    input_mask_white = torch::from_blob(&nn_input->halves[uo_white].mask.vector, { n_input_half_mask, 1 }, torch::kBool);
    input_floats_white = torch::from_blob(&nn_input->halves[uo_white].floats.vector, { 1, n_input_half_floats }, torch::kFloat32);

    input_mask_black = torch::from_blob(&nn_input->halves[uo_black].mask.vector, { n_input_half_mask, 1 }, torch::kBool);
    input_floats_black = torch::from_blob(&nn_input->halves[uo_black].floats.vector, { 1, n_input_half_floats }, torch::kFloat32);

    input_mask_shared = torch::from_blob(&nn_input->shared.mask.vector, { n_input_shared_mask, 1 }, torch::kBool);

    W1_mask_own = register_parameter("W1_mask_own", torch::randn({ n_input_half_mask, n_hidden_1 }));
    W1_floats_own = register_parameter("W1_floats_own", torch::randn({ n_input_half_floats, n_hidden_1 }));

    W1_mask_enemy = register_parameter("W1_mask_enemy", torch::randn({ n_input_half_mask, n_hidden_1 }));
    W1_floats_enemy = register_parameter("W1_floats_enemy", torch::randn({ n_input_half_floats, n_hidden_1 }));

    W1_mask_shared = register_parameter("W1_mask_shared", torch::randn({ n_input_shared_mask, n_hidden_1 }));

    b1 = register_parameter("b1", torch::randn(n_hidden_1));

    W2 = register_parameter("W2", torch::randn({ n_hidden_1, n_output }));
    b2 = register_parameter("b2", torch::randn(n_output));
  }

  torch::Tensor forward(va_list args) {
    int color = va_arg(args, int);

    torch::Tensor input_mask_own;
    torch::Tensor input_floats_own;
    torch::Tensor input_mask_enemy;
    torch::Tensor input_floats_enemy;

    if (color == uo_white)
    {
      input_mask_own = input_mask_white;
      input_floats_own = input_floats_white;
      input_mask_enemy = input_mask_black;
      input_floats_enemy = input_floats_black;
    }
    else
    {
      input_mask_own = input_mask_black;
      input_floats_own = input_floats_black;
      input_mask_enemy = input_mask_white;
      input_floats_enemy = input_floats_white;
    }

    torch::Tensor zero = torch::zeros(1, torch::kFloat);

    // (n_input_half_mask x n_hidden_1)
    torch::Tensor x_mask_own = torch::where(input_mask_own, W1_mask_own, zero);

    // (1 x n_hidden_1)
    torch::Tensor x_mask_own_sum = torch::sum(x_mask_own, 0, true);

    // (n_input_half_mask x n_hidden_1)
    torch::Tensor x_mask_enemy = torch::where(input_mask_enemy, W1_mask_enemy, zero);
    // (1 x n_hidden_1)
    torch::Tensor x_mask_enemy_sum = torch::sum(x_mask_enemy, 0, true);

    // (n_input_shared_mask x n_hidden_1)
    torch::Tensor x_mask_shared = torch::where(input_mask_shared, W1_mask_shared, zero);
    // (1 x n_hidden_1)
    torch::Tensor x_mask_shared_sum = torch::sum(x_mask_shared, 0, true);

    torch::Tensor x_floats_own = torch::mm(input_floats_own, W1_floats_own);
    // (1 x n_hidden_1)
    torch::Tensor x_floats_enemy = torch::mm(input_floats_enemy, W1_floats_enemy);

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

  torch::Tensor W1_mask_own, W1_floats_own;
  torch::Tensor W1_mask_enemy, W1_floats_enemy;
  torch::Tensor W1_mask_shared;
  torch::Tensor b1;
  torch::Tensor W2, b2;

  torch::Tensor input_mask_white, input_floats_white;
  torch::Tensor input_mask_black, input_floats_black;
  torch::Tensor input_mask_shared;

  torch::Tensor output;
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

