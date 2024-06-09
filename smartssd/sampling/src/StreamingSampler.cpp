#include "StreamingSampler.hpp"
#include "utils/timer.hpp"
#include <fcntl.h>
#include <fstream>
#include <sys/stat.h>

StreamingSampler::StreamingSampler(
    std::vector<uint> xrt_device_id, std::string xclbin_file,
    std::string kernel_name, std::string edge_file_path,
    std::string chunk_info_file_path, std::string target_node_file_path,
    std::vector<uint> fanouts, size_t edge_chunk_size)
    : SmartSSDBase(xrt_device_id, xclbin_file, kernel_name),
      SamplerBase(fanouts) {
  openEdgeFile(edge_file_path);
  loadChunkInfo(chunk_info_file_path);
  loadTargetNodes(target_node_file_path);
  setEdgeChunkSize(edge_chunk_size);
  setMaxSampleSizePerChunk(460000000);
  setMaxTargetSize(46000000);
  allocateBufferObject();
  current_bo_index = 1;
  this->sample_result.push_back(std::vector<std::vector<uint>>());
  this->sample_result.push_back(std::vector<std::vector<uint>>());
  this->sample_result_size.push_back(std::vector<std::vector<uint>>());
  this->sample_result_size.push_back(std::vector<std::vector<uint>>());
}

int StreamingSampler::openEdgeFile(std::string edge_file_path) {
  this->edge_file_handler = -1;
  this->edge_file_handler = open(edge_file_path.c_str(), O_RDWR | O_DIRECT);
  if (this->edge_file_handler < 0) {
    std::cerr << "ERROR: open " << edge_file_path << "failed: " << std::endl;
    return -1;
  }
  struct stat statbuf;
  if (fstat(edge_file_handler, &statbuf) == -1) {
    return -1;
  }
  this->edge_file_size_byte = statbuf.st_size;
  return 0;
}

off_t StreamingSampler::getEdgeFileSizeByte() {
  return this->edge_file_size_byte;
}

int StreamingSampler::loadChunkInfo(std::string chunk_info_file_path) {
  std::ifstream chunk_info_file(chunk_info_file_path);
  int32_t buffer;
  while (chunk_info_file.read(reinterpret_cast<char *>(&buffer),
                               sizeof(buffer))) {
    this->chunk_offsets.push_back(buffer);
  }
  chunk_info_file.close();
  return 0;
}

int StreamingSampler::loadTargetNodes(std::string target_node_file_path) {
  std::ifstream target_node_file(target_node_file_path);
  int32_t buffer;
  while (target_node_file.read(reinterpret_cast<char *>(&buffer),
                               sizeof(buffer))) {
    this->target_nodes.push_back(buffer);
  }
  target_node_file.close();
  sort(target_nodes.begin(), target_nodes.end());
  return 0;
}

int StreamingSampler::getEdgeFileHandler() { return this->edge_file_handler; }

std::vector<uint> StreamingSampler::getChunkOffsets() {
  return this->chunk_offsets;
}

std::vector<uint> StreamingSampler::getTargetNodes() {
  return this->target_nodes;
}

void StreamingSampler::setMaxSampleSizePerChunk(
    size_t max_sample_size_per_chunk) {
  this->max_sample_size_per_chunk = max_sample_size_per_chunk;
}

size_t StreamingSampler::getMaxSampleSizePerChunk() {
  return this->max_sample_size_per_chunk;
}

void StreamingSampler::setMaxTargetSize(size_t max_target_size) {
  this->max_target_size = max_target_size;
}

size_t StreamingSampler::getMaxTargetSize() { return this->max_target_size; }

void StreamingSampler::setEdgeChunkSize(size_t edge_chunk_size) {
  this->edge_chunk_size = edge_chunk_size;
}

size_t StreamingSampler::getEdgeChunkSize() { return this->edge_chunk_size; }

void StreamingSampler::allocateBufferObject() {
  // calculate the size of the input and output buffer
  this->input_size_byte = this->edge_chunk_size * sizeof(int);
  size_t output_size_byte = this->max_sample_size_per_chunk * sizeof(int);
  size_t target_size_byte = this->max_target_size * sizeof(int);

  // Allocate Memory for Each SmartSSD device
  xrt::bo::flags flags = xrt::bo::flags::p2p;
  for (size_t i = 0; i < this->getXrtDevice().size(); i++) {
    auto device = this->getXrtDevice()[i];
    auto krnl = this->getXrtKernel()[i];
    bo_edge.push_back({
        xrt::bo(device, this->input_size_byte, flags, krnl.group_id(0)),
        xrt::bo(device, this->input_size_byte, flags, krnl.group_id(0)),
    });
    bo_sample_result.push_back(
        xrt::bo(device, output_size_byte, krnl.group_id(1)));
    bo_target_nodes.push_back(
        xrt::bo(device, target_size_byte, krnl.group_id(2)));
  }

  // Map Global Memory Buffer to Host Pointer
  for (size_t i = 0; i < this->getXrtDevice().size(); i++) {
    bo_edge_map.push_back(
        {bo_edge[i][0].map<uint *>(), bo_edge[i][1].map<uint *>()});
    bo_sample_result_map.push_back(bo_sample_result[i].map<uint *>());
    bo_target_nodes_map.push_back(bo_target_nodes[i].map<uint *>());
  }
}

std::vector<std::vector<uint>>
StreamingSampler::splitFrontier(std::vector<uint> &frontier) {
  // std::cout << "Splitting frontier..., frontier size " << frontier.size()
  //           << "Last frontier" << frontier[frontier.size() - 1] << std::endl;
  std::vector<std::vector<uint>> result;
  uint cur_chunk = 0;
  // int cur_chunk_start = 0;
  uint cur_chunk_end = this->chunk_offsets[cur_chunk];
  // std::cout << "this->chunk_offsets.size(): " << this->chunk_offsets.size()
  //           << std::endl;
  // std::cout << "cur_chunk_end: " << cur_chunk_end << std::endl;
  std::vector<uint> cur_chunk_nodes;
  // uint pre_node = -1;
  for (size_t i = 0; i < frontier.size(); i++) {
    if (frontier[i] >= cur_chunk_end) {
      result.push_back(cur_chunk_nodes);
      cur_chunk_nodes.clear();
      cur_chunk += 1;
      // cur_chunk_start = cur_chunk_end;
      cur_chunk_end = this->chunk_offsets[cur_chunk];
    }
    // if (frontier[i] == pre_node) {
    //   continue;
    // }
    cur_chunk_nodes.push_back(frontier[i]);
    // pre_node = frontier[i];
  }
  result.push_back(cur_chunk_nodes);
  // std::cout << "Splitting frontier done, number of chunks: " << result.size()
  //           << std::endl;
  return result;
}

std::vector<uint> StreamingSampler::sampleOneLayer(
    std::vector<std::vector<uint>> splitted_frontier, int n_neighbors) {
  EasyTimer timer("Total time for Sample one layer");
  std::cout << "Begin sample one layer, n_neighbors: " << n_neighbors
            << ", number of chunks: " << splitted_frontier.size() << std::endl;
  std::vector<uint> result;
  float fpga_time = 0;
  float data_transfer_time = 0;
  for (size_t i = 0; i < splitted_frontier.size(); i += 1) {
    std::cout << "Processing chunk " << i
              << ", chunk frontier size: " << splitted_frontier[i].size()
              << std::endl;
    size_t this_read_size_byte;
    // the last chunk might be smaller than chunk size
    if (i == splitted_frontier.size() - 1) {
      // only read the remaining size
      this_read_size_byte = edge_file_size_byte - i * this->input_size_byte;
    } else {
      // read whole chunk
      this_read_size_byte = this->input_size_byte;
    }

    //fill sample result with -1
    std::fill(bo_sample_result_map[0],
              bo_sample_result_map[0] + splitted_frontier[i].size() * n_neighbors,
              static_cast<uint32_t>(-1));
    bo_sample_result[0].sync(XCL_BO_SYNC_BO_TO_DEVICE);

    // read the chunk from the edge file
    {
      EasyTimer timer(data_transfer_time);
      auto re = pread(this->edge_file_handler, (void *)bo_edge_map[0][0],
                      this_read_size_byte, i * this->input_size_byte);
      if (re <= 0) {
        std::cerr << "ERR: pread failed: "
                  << " error: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
      }

      // copy target nodes to bo
      std::copy(splitted_frontier[i].begin(), splitted_frontier[i].end(),
                bo_target_nodes_map[0]);

      // std::cout << "frontiers: ";
      // for(int i = 0; i < 5; i++) {
      //   std::cout << bo_target_nodes_map[0][i] << " ";
      // }
      // std::cout << std::endl;

      // sync target nodes to FPGA
      bo_target_nodes[0].sync(XCL_BO_SYNC_BO_TO_DEVICE);
    }

    // TO BE DELETE AFTER FIX KERNEL
    // for (size_t k = 0; k < splitted_frontier[i].size() * n_neighbors; k++) {
    //   bo_sample_result_map[0][k] = -1;
    // }
    // bo_sample_result[0].sync(XCL_BO_SYNC_BO_TO_DEVICE);

    {
      EasyTimer timer(fpga_time);
      // run the kernel
      auto run1 = this->getXrtKernel()[0](
          bo_edge[0][0], bo_sample_result[0], bo_target_nodes[0],
          splitted_frontier[i].size(), n_neighbors, (uint)time(NULL));

      run1.wait();
    }

    {
      EasyTimer timer(data_transfer_time);
      // Get the result from FPGA
      bo_sample_result[0].sync(XCL_BO_SYNC_BO_FROM_DEVICE);

      // Copy the result from bo to result vector
      std::copy(bo_sample_result_map[0],
                bo_sample_result_map[0] +
                    splitted_frontier[i].size() * n_neighbors,
                std::back_inserter(result));
    }

    std::cout << "chunk " << i << ", sample result: " << std::endl;

    for (int k = 0; k < n_neighbors * 5; k++) {
      if (k && k % n_neighbors == 0) {
        std::cout << std::endl;
      }
      if(bo_sample_result_map[0][k] != 4294967295)
        std::cout << bo_sample_result_map[0][k] << " ";
      else 
        std::cout << "N ";
    }
    std::cout << std::endl << std::endl;
  }
  std::cout << "End sample one layer, result size: " << result.size()
            << " fpga time: " << fpga_time
            << " data transfer time: " << data_transfer_time << std::endl;
  return result;
}

std::vector<uint>
StreamingSampler::cleanSampleResult(std::vector<uint> &result) {
  // std::cout << "Begin clean sample result... result size: " << result.size()
  // << std::endl;
  std::vector<uint> filtered;

  // Copy elements that are not -1 (actual value 4,294,967,295 in uint)
  std::copy_if(result.begin(), result.end(), std::back_inserter(filtered),
               [](uint value) {
                 return value != (uint)-1; // 4,294,967,295
               });

  sort(filtered.begin(), filtered.end());
  // std::cout << "filtered size: " << filtered.size() << std::endl;
  return filtered;
}

std::vector<uint> StreamingSampler::flipBackToOriginalORder(std::vector<uint> &result, std::vector<uint> &idx, uint fanout) {
  std::vector<uint> flipped_result(result.size());
  for (size_t j = 0; j < idx.size(); j++) {
    for (size_t k = 0; k < fanout; k++) {
      flipped_result[j * fanout + k] =
          result[idx[j] * fanout + k];
    }
  }
  return flipped_result;
}

std::vector<uint> StreamingSampler::deduplicateResult(std::vector<uint> &result, std::vector<std::vector<uint>> &sample_result_size, uint fanout) {
  // vector to store the size of the result of each batch
  std::vector<uint> cur_result_size = std::vector<uint>();
  std::vector<uint> deduplicated_result;
  size_t pos = 0;
  for(auto source_size: sample_result_size.back()) {
      std::vector<uint> temp;
      // copy all non -1 elements to deduplicated_result
      std::copy_if(result.begin() + pos, result.begin() + pos + source_size * fanout, std::back_inserter(temp), [](uint value) {
        return value != static_cast<uint>(-1); // 4,294,967,295
      });
      // deduplicate
      std::sort(temp.begin(), temp.end());
      auto last = std::unique(temp.begin(), temp.end());
      std::copy(temp.begin(), last, std::back_inserter(deduplicated_result));
      cur_result_size.push_back(last - temp.begin());
      pos += source_size * fanout;
  }
  sample_result_size.push_back(cur_result_size);
  return deduplicated_result;
}

void StreamingSampler::sampleNextEpoch(int bo_index) {
  std::cout << "Begin sample next epoch... bo_index: " << bo_index << std::endl;
  std::vector<std::vector<uint>> *cur_sample_result = &sample_result[bo_index];
  std::vector<std::vector<uint>> splited_frontier;
  std::vector<uint32_t> idx;
  sample_result_size[bo_index].clear();
  uint32_t deduplicate_range = 1000;
  cur_sample_result->clear();
  // Sample layer by layer
  for (size_t i = 0; i < this->getFanouts().size(); i++) {
    std::vector<uint> cur_frontier;
    {
      EasyTimer timer("Prepair frontiers ");
      if (i == 0) {
        cur_frontier.resize(this->target_nodes.size());
        idx.resize(this->target_nodes.size());
        for (size_t i = 0; i < idx.size(); ++i) {
          idx[i] = i;
        }
        // Sorting the index vector based on the original vector
        std::sort(idx.begin(), idx.end(), [&](int i1, int i2) {
          return this->target_nodes[i1] < this->target_nodes[i2];
        });
        // Reordering the elements in the original vector based on the sorted
        // indices
        for (size_t j = 0; j < this->target_nodes.size(); ++j) {
          cur_frontier[j] = this->target_nodes[idx[j]];
        }
      } else {
        
        cur_frontier.resize((*cur_sample_result)[i-1].size());
        idx.resize(cur_frontier.size());
        for (size_t i = 0; i < idx.size(); ++i) {
          idx[i] = i;
        }
        // Sorting the index vector based on the original vector
        std::sort(idx.begin(), idx.end(), [&](int i1, int i2) {
          return (*cur_sample_result)[i-1][i1] < (*cur_sample_result)[i-1][i2];
        });
        // Reordering the elements in the original vector based on the sorted
        // indices
        for (size_t j = 0; j < (*cur_sample_result)[i-1].size(); ++j) {
          cur_frontier[j] = (*cur_sample_result)[i-1][idx[j]];
        }
      }
      std::cout << "cur_frontier size: " << cur_frontier.size() << std::endl;
      splited_frontier = splitFrontier(cur_frontier);
    }
    std::vector<uint> this_layer_result =
        sampleOneLayer(splited_frontier, this->getFanouts()[i]);
    // convert back to original order
    this_layer_result = flipBackToOriginalORder(this_layer_result, idx, this->getFanouts()[i]);

    // deduplicate
    if(i == 0){
      // add sample_result_size for trian nodes
      std::vector<uint> temp;
      for(size_t j = 0; j < cur_frontier.size(); j+= 1000) {
        temp.push_back(std::min(size_t(1000), cur_frontier.size() - j));
      }
      sample_result_size[bo_index].push_back(temp);
    }
    std::vector<uint> deduplicate_result = deduplicateResult(this_layer_result, sample_result_size[bo_index], this->getFanouts()[i]);

    (*cur_sample_result).push_back(deduplicate_result);

    // count non -1 size
    int count = std::count_if(deduplicate_result.begin(), deduplicate_result.end(),
                              [](uint x) { return x != -1; });

    std::cout << "Number of elements not equal to -1: " << count << std::endl;
  }

}

std::vector<std::vector<uint>>
StreamingSampler::getSample(std::vector<uint> target_nodes) {
  std::vector<std::vector<uint>> result;
  // we no longer need to assemble the result, just return the result from the
  // memory.

  // TODO implment this

  return result;
}

void StreamingSampler::newEpochStart() {
  // shuffle for benchmark only
  std::shuffle(target_nodes.begin(), target_nodes.end(),
                std::default_random_engine(time(NULL)));
  // flip the buffer object index
  this->current_bo_index ^= 1;
  // clear the frontier and sample result
  // sample the next epoch
  sampleNextEpoch(this->current_bo_index);
}