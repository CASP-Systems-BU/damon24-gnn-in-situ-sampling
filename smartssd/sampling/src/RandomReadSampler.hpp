#ifndef RANDOM_READ_SAMPLER_HPP
#define RANDOM_READ_SAMPLER_HPP
#include "SamplerBase.hpp"
#include "SmartSSDBase.hpp"
#include <vector>

class RandomReadSampler : public SmartSSDBase, public SamplerBase {
private:
  int edge_file_handler;
  off_t edge_file_size_byte;
  std::vector<off64_t> offsets;
  std::vector<uint> degrees;
  size_t max_batch_sample_size;
  std::vector<xrt::bo> bo_raw_sample;     // ~1.5GB
  std::vector<xrt::bo> bo_offsets;        // ~12MB
  std::vector<xrt::bo> bo_buffer_offsets; // ~12MB
  std::vector<xrt::bo> bo_sample_result;  // ~12MB

  std::vector<uint *> bo_raw_sample_map;
  std::vector<uint *> bo_offsets_map;
  std::vector<uint *> bo_buffer_offsets_map;
  std::vector<uint *> bo_sample_result_map;

  float transfer_time = 0;
  float fpga_time = 0;

  /**
   * Open the edge file to get the file handler
   */
  int openEdgeFile(std::string edge_file_path);

  /**
   * Load the offsets information of all the nodes in the edge file
   */
  int loadOffsets(std::string offsets_file_path);

  /**
   * Allocate the Buffer Object for FPGA and create its mapping
   */
  void allocateBufferObject();

  /**
   * Helper funtion to smaple one layer
   */
  std::vector<uint> sampleOneLayer(std::vector<uint> frontier,
                                   uint n_neighbors);

  /**
   * Get the degree of a node by its two offsets
   */
  inline uint32_t get_degree(uint32_t node_id);

public:
  /**
   * @brief Construct a new Streaming Sampler object
   * this will open the device and load xclbin file, also load the edge_file,
   * chunk_info_file, and frontier_file
   * @param xrt_device_id: The vector of int represent ids of the XRT device
   * @param xclbin_file: The xclbin file path
   * @param kernel_name: The name of the kernel
   * @param edge_file_path: The edge file path
   * @param offsets_file_path: The offsets information of edge file
   * @param fanouts: The number of neighbors of each sample layer
   */
  RandomReadSampler(std::vector<uint> xrt_device_id, std::string xclbin_file,
                    std::string kernel_name, std::string edge_file_path,
                    std::string offsets_file_path, std::vector<uint> fanouts);

  /**
   * Get the maximum batch sample size
   */
  size_t getMaxBatchSampleSize();

  /**
   * Set the maximum batch sample size. This affect the buffer object size we
   * allocate.
   */
  void setMaxBatchSampleSize(size_t max_batch_sample_size);

  /**
   * Overwrite the abstract function getSample. This is called by DGL.
   */
  std::vector<std::vector<uint>> getSample(std::vector<uint> fontier) override;

  /**
   * get fpga time
   */
  float getFpgaTime();

  /**
   * get transfer time
   */
  float getTransferTime();
};

#endif // RANDOM_READ_SAMPLER_HPP