#include "./include/uo_nn/uo_nntest.h"

#include <torch/torch.h>
#include <iostream>
#include <cstdlib>

typedef struct uo_nn2
{
  torch::Tensor *input_tensors;
  torch::Tensor *output_tensors;
  torch::Tensor *output_true_tensors;
  torch::nn::Module *model;
  torch::optim::Optimizer *optimizer;
  torch::nn::Module *loss_fn;
} uo_nn2;


class XORModel : public torch::nn::Module {
public:
  XORModel() {
    fc1 = register_module("fc1", torch::nn::Linear(2, 2));
    fc2 = register_module("fc2", torch::nn::Linear(2, 1));
  }

  torch::Tensor forward(torch::Tensor x) {
    x = torch::relu(fc1->forward(x));
    x = torch::sigmoid(fc2->forward(x));
    return x;
  }

  torch::nn::Linear fc1, fc2;
};


int main() {
  XORModel model;

  // Generate some data
  torch::Tensor x = torch::tensor({ {0.0, 0.0}, {0.0, 1.0}, {1.0, 0.0}, {1.0, 1.0} }, torch::kFloat32);
  torch::Tensor y = torch::tensor({ {0.0}, {1.0}, {1.0}, {0.0} }, torch::kFloat32);

  // Define the loss function and optimizer
  torch::nn::BCELoss loss_fn;
  torch::optim::SGD optimizer(model.parameters(), torch::optim::SGDOptions(0.1));

  // Train the model
  for (size_t epoch = 0; epoch < 100; ++epoch) {
    // Zero the gradients
    optimizer.zero_grad();

    // Make a forward pass
    auto y_pred = model.forward(x);

    // Compute the loss
    auto loss = loss_fn(y_pred, y);
    std::cout << "Epoch: " << epoch << " | Loss: " << loss.item<float>() << std::endl;

    // Backpropagate the gradients
    loss.backward();

    // Update the parameters
    optimizer.step();
  }

  // Predict using the trained model
  auto y_pred = model.forward(x);
  std::cout << "Final prediction: " << y_pred << std::endl;

  return 0;
}

uo_nn2 *uo_nn2_create_xor(size_t batch_size)
{
  char *mem = (char *)std::malloc(sizeof(uo_nn2) + 3 * sizeof(torch::Tensor));

  uo_nn2 *nn = (uo_nn2 *)mem;
  mem += sizeof(uo_nn2);

  nn->input_tensors = (torch::Tensor *)mem;
  mem += sizeof(torch::Tensor) * 1;

  nn->output_tensors = (torch::Tensor *)mem;
  mem += sizeof(torch::Tensor) * 1;

  nn->output_true_tensors = (torch::Tensor *)mem;
  mem += sizeof(torch::Tensor) * 1;

  nn->model = new XORModel();

  nn->input_tensors[0] = torch::zeros({ batch_size, 2 });
  nn->output_true_tensors[0] = torch::zeros({ batch_size, 1 });

  return nn;
}

void uo_nn2_init_optimizer(uo_nn2 *nn)
{
  // Define the loss function and optimizer
  //nn->loss_fn = torch::nn::BCELoss;
  nn->optimizer = new torch::optim::SGD(nn->model->parameters(), torch::optim::SGDOptions(0.1));
}

void *uo_nn2_input_data_ptr(uo_nn2 *nn, int input_index)
{
  return nn->input_tensors[input_index].data_ptr();
}

void *uo_nn2_output_data_ptr(uo_nn2 *nn, int output_index)
{
  return nn->output_tensors[output_index].data_ptr();
}

void *uo_nn2_output_true_data_ptr(uo_nn2 *nn, int output_index)
{
  return nn->output_true_tensors[output_index].data_ptr();
}

void uo_nn2_zero_grad(uo_nn2 *nn)
{
  nn->optimizer->zero_grad();
}

void uo_nn2_forward(uo_nn2 *nn)
{
  nn->output_tensors[0] = reinterpret_cast<XORModel *>(nn->model)->forward(nn->input_tensors[0]);
}

void uo_nn2_backward(uo_nn2 *nn)
{
  torch::nn::BCELoss loss_fn;
  auto loss = loss_fn(nn->output_tensors[0], nn->output_true_tensors[0]);
  loss.backward();
  nn->optimizer->step();
}

