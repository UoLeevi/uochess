#include "include/uo_nn.h"

#include <torch/torch.h>
#include <iostream>
#include <cstdlib>

struct XORModel : torch::nn::Module {
public:
  XORModel()
  {
    W1 = register_parameter("W1", torch::randn({ 2, 8 }));
    b1 = register_parameter("b1", torch::randn(2));

    W2 = register_parameter("W2", torch::randn({ 8, 1 }));
    b2 = register_parameter("b2", torch::randn(1));
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
  torch::optim::SGD *optimizer;
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

