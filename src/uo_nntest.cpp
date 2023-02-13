#include "uo_nn/uo_nntest.h"

#include <torch/torch.h>
#include <iostream>
#include <cstdlib>


typedef struct uo_nn2
{
  void *model;
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

uo_nn2 *uo_nn2_create_xor()
{
}

void *uo_nn2_input_data_ptr(uo_nn2 *nn, int input_index);

void *uo_nn2_output_data_ptr(uo_nn2 *nn, int output_index);

void uo_nn2_zero_grad(uo_nn2 *nn);

void uo_nn2_forward(uo_nn2 *nn);

void uo_nn2_backward(uo_nn2 *nn);

void uo_nn2_update_parameters(uo_nn2 *nn);
