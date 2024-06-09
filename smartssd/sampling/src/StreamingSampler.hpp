#ifndef STREAMING_SAMPLER_HPP
#define STREAMING_SAMPLER_HPP
#include "SamplerBase.hpp"
#include "SmartSSDBase.hpp"
#include <vector>
#include <random>

class StreamingSampler : public SmartSSDBase, public SamplerBase {
private:
  int edge_file_handler;
  off_t edge_file_size_byte;
  std::vector<uint> chunk_offsets;
  std::vector<uint> target_nodes;
  size_t edge_chunk_size;
  size_t max_sample_size_per_chunk;
  size_t max_target_size;
  size_t input_size_byte;
  std::vector<std::vector<xrt::bo>> bo_edge;
  std::vector<xrt::bo> bo_sample_result;
  std::vector<xrt::bo> bo_target_nodes;
  std::vector<std::vector<uint *>> bo_edge_map;
  // std::vector<uint *> bo_edge_B_map;
  std::vector<uint *> bo_sample_result_map;
  std::vector<uint *> bo_target_nodes_map;
  int current_bo_index;
  std::vector<std::vector<std::vector<uint>>> sample_result;
  std::vector<std::vector<std::vector<uint>>> sample_result_size;

  /**
   * Open the edge file to get the file handler
   */
  int openEdgeFile(std::string edge_file_path);

  /**
   * Open the chunk file and load the chunk info
   */
  int loadChunkInfo(std::string chunk_info_file_path);

  /**
   * Open the training nodes file and load the training nodes
   */
  int loadTargetNodes(std::string target_nodes_file_path);

  /**
   * Allocate the Buffer Object for FPGA and create its mapping
   */
  void allocateBufferObject();

  /**
   * Sample one layer of the neighbors of the frontier
   */
  std::vector<uint> sampleOneLayer(std::vector<std::vector<uint>> frontier,
                                   int n_neighbors);

  /**
   * Clean up the sample result, remove -1 from the result, and also sort the
   * result to prepare for the next layer sampling
   */
  std::vector<uint> cleanSampleResult(std::vector<uint> &result);

  /**
   * Internal call for this sampler to sample all the result of next layer
   */
  void sampleNextEpoch(int bo_index);

  /**
   * Analyze the frontier infomation and split them according to chunk info
   */
  std::vector<std::vector<uint>> splitFrontier(std::vector<uint> &frontier);

public:
  /**
   * @brief Construct a new Streaming Sampler object
   * this will open the device and load xclbin file, also load the edge_file,
   * chunk_info_file, and frontier_file
   * @param xrt_device_id: The vector of int represent ids of the XRT device
   * @param xclbin_file: The xclbin file path
   * @param kernel_name: The name of the kernel
   * @param edge_file_path: The edge file path
   * @param chunk_info_file_path: The chunk info file path
   * @param target_node_file_path: The frontier file path
   * @param fanouts: The number of neighbors of each sample layer
   * @param edge_chunk_size: The size of the chunk (number of integers)
   */
  StreamingSampler(std::vector<uint> xrt_device_id, std::string xclbin_file,
                   std::string kernel_name, std::string edge_file_path,
                   std::string chunk_info_file_path,
                   std::string target_node_file_path, std::vector<uint> fanouts,
                   size_t edge_chunk_size);

  /**
   * Get the edge file handler that is opened
   */
  int getEdgeFileHandler();

  /**
   * Get the edge file size in bytes
   */
  off_t getEdgeFileSizeByte();

  /**
   * Get the chunk offsets
   */
  std::vector<uint> getChunkOffsets();

  /**
   * Get target nodes
   */
  std::vector<uint> getTargetNodes();

  /**
   * Set the edge chunk size
   */
  void setEdgeChunkSize(size_t edge_chunk_size);

  /**
   * Get the edge chunk size
   */
  size_t getEdgeChunkSize();

  /**
   * Set the maxmium sample size per chunk
   */
  void setMaxSampleSizePerChunk(size_t max_sample_size_per_chunk);

  /**
   * Get the maxmium sample size per chunk
   */
  size_t getMaxSampleSizePerChunk();

  /**
   * Set the maxmium target size
   */
  void setMaxTargetSize(size_t max_target_size);

  /**
   * Get the maxmium target size
   */
  size_t getMaxTargetSize();

  std::vector<std::vector<uint>> getSample(std::vector<uint> fontier) override;

  /**
   * An new epoch is started
   */
  void newEpochStart();

  std::vector<uint> flipBackToOriginalORder(std::vector<uint> &result, std::vector<uint> &idx, uint fanout);

  std::vector<uint> deduplicateResult(std::vector<uint> &result, std::vector<std::vector<uint> > &sample_result_size, uint fanout);
};

#endif // STREAMING_SAMPLER_HPP