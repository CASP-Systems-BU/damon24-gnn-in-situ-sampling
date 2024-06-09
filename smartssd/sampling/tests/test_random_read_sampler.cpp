#include "RandomReadSampler.hpp"
#include "utils/timer.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>

std::vector<uint32_t> loadTargetNodes(std::string target_node_file_path) {
  std::vector<uint32_t> target_nodes;
  std::ifstream target_node_file(target_node_file_path);
  int32_t buffer;
  while (target_node_file.read(reinterpret_cast<char *>(&buffer),
                               sizeof(buffer))) {
    target_nodes.push_back(buffer);
  }
  target_node_file.close();
  sort(target_nodes.begin(), target_nodes.end());
  return target_nodes;
}

int main() {

  RandomReadSampler sampler(
      {1}, "simple_random_read_sampler.xclbin", "simple_random_read_sampler",
      "/mnt/nvme2/data/papers100M/preprocessed2/random_read_edges.bin",
      "/mnt/nvme2/data/papers100M/preprocessed2/offsets.bin", {25, 10});
  // RandomReadSampler sampler(
  //     {1}, "simple_random_read_sampler.xclbin", "simple_random_read_sampler",
  //     "/mnt/nvme2/data/yahoo/preprocessed/random_read_edges.bin",
  //     "/mnt/nvme2/data/yahoo/offset.bin", {25, 10});

  std::cout << "Xrt Device Id: " << sampler.getXrtDevice()[0] << std::endl;

  // auto result = sampler.getSample({229273});
  // for (auto &r : result) {
  //   for (auto &rr : r) {
  //     std::cout << rr << " ";
  //   }
  //   std::cout << std::endl;
  // }

  // auto result = sampler.getSample({105218019});
  // for (auto &r : result) {
  //   for (auto &rr : r) {
  //     std::cout << rr << " ";
  //   }
  //   std::cout << std::endl;
  // }

  // auto result = sampler.getSample({41945091});
  // for (auto &r : result) {
  //   for (auto &rr : r) {
  //     std::cout << rr << " ";
  //   }
  //   std::cout << std::endl;
  // }

  // auto result = sampler.getSample({684});
  // for (auto &r : result) {
  //   for (auto &rr : r) {
  //     std::cout << rr << " ";
  //   }
  //   std::cout << std::endl;
  // }

  // result = sampler.getSample({1384});
  // for (auto &r : result) {
  //   for (auto &rr : r) {
  //     std::cout << rr << " ";
  //   }
  //   std::cout << std::endl;
  // }

  // result = sampler.getSample({1525, 2127, 2184, 2205, 3965, 4200, 5031});
  // for (auto &r : result) {
  //   for (auto &rr : r) {
  //     std::cout << rr << " ";
  //   }
  //   std::cout << std::endl << std::endl;
  // }

  // std::vector<uint> training_nodes = sampler.getTargetNodes();

  // auto training_nodes =
  //     loadTargetNodes("/mnt/nvme2/data/papers100M/train_nodes.bin");

  auto training_nodes =
      loadTargetNodes("/mnt/nvme2/data/yahoo/preprocessed/train.bin");

  std::cout << "Training nodes size: " << training_nodes.size() << std::endl;

  std::shuffle(training_nodes.begin(), training_nodes.end(),
               std::default_random_engine(0));

  {
    EasyTimer timer("Queries for whole epoch.");
    for (size_t i = 0; i < training_nodes.size(); i += 1000) {
      // EasyTimer timer2("Queries for 1000 nodes.");
      // std::cout << "\r Progress " << i << " / " << training_nodes.size()
      //           << std::endl;
      // std::cout.flush();

      auto frontier = std::vector<uint>(
          training_nodes.begin() + i,
          min(training_nodes.begin() + i + 1000, training_nodes.end()));

      auto result = sampler.getSample(frontier);
      // break;
    }
  }

  std::cout << "FPGA time" << sampler.getFpgaTime() << std::endl;
  std::cout << "Transfer time" << sampler.getTransferTime() << std::endl;
}