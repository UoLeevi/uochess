#include <stdio.h>
#include <torch/torch.h>

int main(int argc, char **argv) {
  // Load the network from disk
  torch::nn::Module net;
  torch::load(net, "src/nn/model_xor.pt");

  // Initialize the loss function and optimizer
  torch::nn::BCELoss criterion;
  torch::optim::SGD optimizer(net->parameters(), torch::optim::SGDOptions(0.1));

  // Train the network
  torch::Tensor inputs = torch::tensor({ {0, 0}, {0, 1}, {1, 0}, {1, 1} }, torch::kFloat32);
  torch::Tensor targets = torch::tensor({ {0}, {1}, {1}, {0} }, torch::kFloat32);
  for (int epoch = 0; epoch < 10000; ++epoch) {
    optimizer.zero_grad();
    torch::Tensor outputs = net->forward(inputs);
    torch::Tensor loss = criterion(outputs, targets);
    loss.backward();
    optimizer.step();
  }

  // Test the network
  torch::Tensor test_inputs = torch::tensor({ {0, 0}, {0, 1}, {1, 0}, {1, 1} }, torch::kFloat32);
  torch::Tensor test_outputs = net->forward(test_inputs);
  printf("%.2f, %.2f, %.2f, %.2f\n", test_outputs[0].item<float>(), test_outputs[1].item<float>(),
    test_outputs[2].item<float>(), test_outputs[3].item<float>());

  return 0;
}
