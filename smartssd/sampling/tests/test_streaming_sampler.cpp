#include "StreamingSampler.hpp"
#include "utils/timer.hpp"
#include <iostream>
#include <random>

int main() {
  // StreamingSampler sampler(
  // {1}, "sample_target_nodes.xclbin", "sample_target_nodes",
  // "/mnt/nvme2/data/papers100M/preprocessed/edges_padded.bin",
  // "/mnt/nvme2/data/papers100M/preprocessed/chunks.txt",
  // "/mnt/nvme2/data/papers100M/train_nodes.bin", {20, 15, 10},
  // (size_t)128 * 1024 * 1024);

  // StreamingSampler sampler(
  // {0}, "sample_target_nodes.xclbin", "sample_target_nodes",
  // "/mnt/nvme2/data/papers100M/preprocessed2/streaming_edges.bin",
  // "/mnt/nvme2/data/papers100M/preprocessed2/chunks.txt",
  // "/mnt/nvme2/data/papers100M/train_nodes.bin", {20, 15, 10},
  // (size_t)128 * 1024 * 1024);

  StreamingSampler sampler(
      {0}, "parallel_streaming_sampler.xclbin", "parallel_streaming_sampler",
      "/mnt/nvme2/data/yahoo/preprocessed/streaming_edges.bin",
      "/mnt/nvme2/data/yahoo/preprocessed/chunk_info.bin",
      "/mnt/nvme2/data/yahoo/preprocessed/train.bin", {20, 15, 10},
      (size_t)128 * 1024 * 1024);

  // StreamingSampler sampler(
  // {0}, "seqkernel.xclbin", "seqkernel",
  // "/mnt/nvme2/data/papers100M/preprocessed2/sequential_read_edges.bin",
  // "/mnt/nvme2/data/papers100M/preprocessed2/sequential_read_chunks.txt",
  // "/mnt/nvme2/data/papers100M/train_nodes.bin", {25, 10},
  // (size_t)128 * 1024 * 1024);

  // StreamingSampler sampler(
  // {0}, "seqkernel.xclbin", "seqkernel",
  // "/mnt/nvme2/data/yahoo/preprocessed/sequential_read.bin",
  // "/mnt/nvme2/data/yahoo/preprocessed/chunks.txt",
  // "/mnt/nvme2/data/yahoo/preprocessed/train.bin", {20, 15, 10},
  // (size_t)128 * 1024 * 1024);

  std::cout << "Edge file handler: " << sampler.getEdgeFileHandler()
            << std::endl;
  std::cout << "Chunk offsets size: " << sampler.getChunkOffsets().size()
            << std::endl;
  std::cout << "Target nodes size: " << sampler.getTargetNodes().size()
            << std::endl;
  std::cout << "Edge chunk size: " << sampler.getEdgeChunkSize() << std::endl;
  std::cout << "Edge file size: " << sampler.getEdgeFileSizeByte() << std::endl;

  std::cout << "Xrt Device Id: " << sampler.getXrtDevice()[0] << std::endl;

  {
    EasyTimer timer("Sampling the whole epoch. ");
    sampler.newEpochStart();
    sampler.newEpochStart();
    sampler.newEpochStart();
    sampler.newEpochStart();
    sampler.newEpochStart();
  }

  // auto result = sampler.getSample({602});
  // for (auto &r : result) {
  //   for (auto &rr : r) {
  //     std::cout << rr << " ";
  //   }
  //   std::cout << std::endl << std::endl;
  // }

  // result = sampler.getSample({684});
  // for (auto &r : result) {
  //   for (auto &rr : r) {
  //     std::cout << rr << " ";
  //   }
  //   std::cout << std::endl << std::endl;
  // }

  // result = sampler.getSample({1384});
  // for (auto &r : result) {
  //   for (auto &rr : r) {
  //     std::cout << rr << " ";
  //   }
  //   std::cout << std::endl << std::endl;
  // }

  // result = sampler.getSample({1525, 2127, 2184, 2205, 3965, 4200, 5031});
  // for (auto &r : result) {
  //   for (auto &rr : r) {
  //     std::cout << rr << " ";
  //   }
  //   std::cout << std::endl << std::endl;
  // }

  // std::vector<uint> training_nodes = sampler.getTargetNodes();

  // std::shuffle(training_nodes.begin(), training_nodes.end(),
  //              std::default_random_engine(0));

  // {
  //   EasyTimer timer("Queries for whole epoch.");
  //   for (size_t i = 0; i < training_nodes.size(); i += 1000) {

  //     auto frontier = std::vector<uint>(
  //         training_nodes.begin() + i,
  //         min(training_nodes.begin() + i + 1000, training_nodes.end()));

  //     auto result = sampler.getSample(frontier);
  //   }
  // }
  return 0;
}