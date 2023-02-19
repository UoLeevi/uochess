#include "uo_nn.h"

#include <torch/torch.h>
#include <iostream>
#include <cstdlib>

struct ChessEvalModel : torch::nn::Module {
public:
  ChessEvalModel(uo_nn_position *nn_input)
  {
    size_t n_input_half_mask = sizeof(nn_input->halves[0].mask) / sizeof(uint8_t);
    size_t n_input_half_floats = sizeof(nn_input->halves[0].floats) / sizeof(float);
    size_t n_input_shared_mask = sizeof(nn_input->shared.mask) / sizeof(uint8_t);
    size_t n_hidden_1 = 8;
    size_t n_output = 1;

    auto options_mask = torch::TensorOptions().dtype(torch::kBool).device(torch::kCPU);
    auto options_floats = torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU);

    input_mask_white = torch::from_blob(nn_input->halves[uo_white].mask.vector, { n_input_half_mask }, options_mask);
    input_floats_white = torch::from_blob(nn_input->halves[uo_white].floats.vector, { 1, n_input_half_floats }, options_floats);

    input_mask_black = torch::from_blob(nn_input->halves[uo_black].mask.vector, { n_input_half_mask }, options_mask);
    input_floats_black = torch::from_blob(nn_input->halves[uo_black].floats.vector, { 1, n_input_half_floats }, options_floats);

    input_mask_shared = torch::from_blob(nn_input->shared.mask.vector, { n_input_shared_mask }, options_mask);

    W1_mask_own = register_parameter("W1_mask_own", torch::randn({ n_input_half_mask, n_hidden_1 }));
    W1_floats_own = register_parameter("W1_floats_own", torch::randn({ n_input_half_floats, n_hidden_1 }));

    W1_mask_enemy = register_parameter("W1_mask_enemy", torch::randn({ n_input_half_mask, n_hidden_1 }));
    W1_floats_enemy = register_parameter("W1_floats_enemy", torch::randn({ n_input_half_floats, n_hidden_1 }));

    W1_mask_shared = register_parameter("W1_mask_shared", torch::randn({ n_input_shared_mask, n_hidden_1 }));

    b1 = register_parameter("b1", torch::randn(n_hidden_1));

    W2 = register_parameter("W2", torch::randn({ n_hidden_1, n_output }));
    b2 = register_parameter("b2", torch::randn(n_output));

    output = torch::empty({ 1, n_output });
  }

  torch::Tensor forward(uint8_t color) {
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

    torch::Tensor zeros_mask_own = torch::zeros_like(W1_mask_own);
    torch::Tensor x_mask_own = torch::where(input_mask_own, W1_mask_own, zeros_mask_own);

    torch::Tensor zeros_mask_enemy = torch::zeros_like(W1_mask_enemy);
    torch::Tensor x_mask_enemy = torch::where(input_mask_enemy, W1_mask_enemy, zeros_mask_enemy);
    
    torch::Tensor zeros_mask_shared = torch::zeros_like(W1_mask_shared);
    torch::Tensor x_mask_shared = torch::where(input_mask_shared, W1_mask_shared, zeros_mask_shared);

    torch::Tensor zeros_mask_enemy = torch::zeros_like(W1_mask_enemy);
    torch::Tensor x_mask_enemy = torch::where(input_mask_enemy, W1_mask_enemy, zeros_mask_enemy);

    torch::Tensor x_floats_own = torch::mm(input_floats_own, W1_floats_own);
    torch::Tensor x_floats_enemy = torch::mm(input_floats_enemy, W1_floats_enemy);

    torch::Tensor x = torch::concat({ x_mask_own, x_mask_enemy, x_mask_shared, x_mask_enemy, x_floats_enemy }, -1);


    x = torch::addmm(b1, x, W1);
    x = torch::relu(x);
    x = torch::addmm(b2, x, W2);
    x = torch::sigmoid(x);
    return x;
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

struct XORModel : torch::nn::Module {
public:
  XORModel()
  {
    size_t n_input = 2;
    size_t n_hidden_1 = 8;
    size_t n_output = 1;

    W1 = register_parameter("W1", torch::randn({ n_input, n_hidden_1 }));
    b1 = register_parameter("b1", torch::randn(n_hidden_1));

    W2 = register_parameter("W2", torch::randn({ n_hidden_1, n_output }));
    b2 = register_parameter("b2", torch::randn(n_output));
  }

  torch::Tensor forward(torch::Tensor x) {
    x = torch::addmm(b1, x, W1);
    x = torch::relu(x);
    x = torch::addmm(b2, x, W2);
    x = torch::sigmoid(x);
    return x;
  }

  torch::Tensor W1, b1;
  torch::Tensor W2, b2;
};

typedef struct uo_nn_impl
{
  torch::Tensor *input_tensors;
  torch::Tensor *output_tensors;
  torch::Tensor *true_output_tensors;
  torch::Tensor *loss_tensors;
  XORModel *model;
  torch::optim::Optimizer *optimizer;
} uo_nn_impl;

uo_nn *uo_nn_create_xor(size_t batch_size)
{
  uo_nn *nn = new uo_nn();
  nn->batch_size = batch_size;

  uo_nn_impl *impl = nn->impl = new uo_nn_impl();

  auto options = torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU);

  nn->input_count = 1;
  impl->input_tensors = new torch::Tensor[nn->input_count];
  nn->inputs = new uo_nn_data[nn->input_count];
  new (&impl->input_tensors[0]) torch::Tensor(torch::zeros({ batch_size, 2 }, options));
  nn->inputs[0].data = impl->input_tensors[0].data_ptr();

  nn->output_count = 1;
  impl->output_tensors = new torch::Tensor[nn->output_count];
  impl->true_output_tensors = new torch::Tensor[nn->output_count];
  nn->outputs = new uo_nn_data[nn->output_count];
  nn->true_outputs = new uo_nn_data[nn->output_count];
  new (&impl->true_output_tensors[0]) torch::Tensor(torch::zeros({ batch_size, 1 }, options));
  nn->true_outputs[0].data = impl->true_output_tensors[0].data_ptr();

  nn->loss_count = 1;
  impl->loss_tensors = new torch::Tensor[nn->loss_count];
  nn->loss = new uo_nn_data[nn->loss_count];

  impl->model = new XORModel();

  return nn;
}

void uo_nn_init_optimizer(uo_nn *nn)
{
  // Define the loss function and optimizer
  //nn->loss_fn = torch::nn::BCELoss;
  nn->impl->optimizer = new torch::optim::SGD(nn->impl->model->parameters(), torch::optim::SGDOptions(0.1));
}

void uo_nn_forward(uo_nn *nn)
{
  nn->impl->output_tensors[0] = nn->impl->model->forward(nn->impl->input_tensors[0]);
  nn->outputs[0].data = nn->impl->output_tensors[0].data_ptr();
}

float uo_nn_compute_loss(uo_nn *nn)
{
  torch::nn::BCELoss loss_fn;
  torch::Tensor loss = nn->impl->loss_tensors[0] = loss_fn(nn->impl->output_tensors[0], nn->impl->true_output_tensors[0]);
  nn->loss[0].data = loss.data_ptr();
  return loss.item<float>();
}

void uo_nn_backward(uo_nn *nn)
{
  nn->impl->optimizer->zero_grad();
  nn->impl->loss_tensors[0].backward();
  nn->impl->optimizer->step();
}

