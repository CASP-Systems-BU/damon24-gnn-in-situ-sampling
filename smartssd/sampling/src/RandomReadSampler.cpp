#include "RandomReadSampler.hpp"
#include "utils/timer.hpp"
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <omp.h>
#include <random>
#include <sys/stat.h>

int RandomReadSampler::openEdgeFile(std::string edge_file_path) {
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

int RandomReadSampler::loadOffsets(std::string offsets_file_path) {
  std::ifstream offsets_file(offsets_file_path);
  // off64_t buffer;  // for yahoo
  uint32_t buffer; // for papers
  while (offsets_file.read(reinterpret_cast<char *>(&buffer), sizeof(buffer))) {
    this->offsets.push_back(buffer);
  }
  offsets_file.close();
  return 0;
}

inline uint32_t RandomReadSampler::get_degree(uint32_t node_id) {
  return this->offsets[node_id + 1] - this->offsets[node_id];
}

RandomReadSampler::RandomReadSampler(std::vector<uint> xrt_device_id,
                                     std::string xclbin_file,
                                     std::string kernel_name,
                                     std::string edge_file_path,
                                     std::string offsets_file_path,
                                     std::vector<uint> fanouts)
    : SmartSSDBase(xrt_device_id, xclbin_file, kernel_name),
      SamplerBase(fanouts) {
  EasyTimer timer("RandomReadSampler Constructor");
  openEdgeFile(edge_file_path);
  loadOffsets(offsets_file_path);
  this->max_batch_sample_size = 1024 * 20 * 15 * 10 + 10;
  allocateBufferObject();
}

float RandomReadSampler::getTransferTime() { return this->transfer_time; }

float RandomReadSampler::getFpgaTime() { return this->fpga_time; }

size_t RandomReadSampler::getMaxBatchSampleSize() {
  return this->max_batch_sample_size;
}

void RandomReadSampler::setMaxBatchSampleSize(size_t max_batch_sample_size) {
  this->max_batch_sample_size = max_batch_sample_size;
}

void RandomReadSampler::allocateBufferObject() {
  // calculate the size of the input and output buffer
  size_t raw_sample_size_byte = this->max_batch_sample_size * 512;
  size_t offsets_size_byte = this->max_batch_sample_size * sizeof(int);
  size_t buffer_offsets_size_byte = this->max_batch_sample_size * sizeof(int);
  size_t sample_result_size_byte = this->max_batch_sample_size * sizeof(int);

  // Allocate Memory for Each SmartSSD device
  xrt::bo::flags flags = xrt::bo::flags::p2p;
  for (size_t i = 0; i < this->getXrtDevice().size(); i++) {
    auto device = this->getXrtDevice()[i];
    auto krnl = this->getXrtKernel()[i];

    // std::cout << "Allocate buffer object for device " << device << std::endl;
    // std::cout << "kernel size: " << this->getXrtKernel().size() << std::endl;
    bo_raw_sample.push_back(
        xrt::bo(device, raw_sample_size_byte, flags, krnl.group_id(0)));

    bo_sample_result.push_back(
        xrt::bo(device, sample_result_size_byte, flags, krnl.group_id(1)));

    bo_offsets.push_back(
        xrt::bo(device, offsets_size_byte, flags, krnl.group_id(2)));

    bo_buffer_offsets.push_back(
        xrt::bo(device, buffer_offsets_size_byte, flags, krnl.group_id(3)));
  }

  // Map Global Memory Buffer to Host Pointer
  for (size_t i = 0; i < this->getXrtDevice().size(); i++) {
    bo_raw_sample_map.push_back(bo_raw_sample[i].map<uint *>());
    bo_offsets_map.push_back(bo_offsets[i].map<uint *>());
    bo_buffer_offsets_map.push_back(bo_buffer_offsets[i].map<uint *>());
    bo_sample_result_map.push_back(bo_sample_result[i].map<uint *>());
  }
}

std::vector<uint> RandomReadSampler::sampleOneLayer(std::vector<uint> frontier,
                                                    uint n_neighbors) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::vector<uint> result; //(n_neighbors * frontier.size());
  {
    EasyTimer timer(transfer_time);
    // std::cout << "size of frontier: " << frontier.size() << std::endl;
#pragma omp parallel for
    for (size_t i = 0; i < frontier.size(); i++) {
      // std::cout << "frontier: " << frontier[i] << std::endl;
      size_t degree = get_degree(frontier[i]);
      // std::cout << "degree: " << degree << " n_neighbors: " << n_neighbors
      //           << std::endl;
      if (degree < n_neighbors) {
        // #pragma omp parallel for
        size_t pre_read_buffer_offset = -1;
        uint32_t cur_buffer_offset = 0;
        for (size_t j = 0; j < degree; j++) {
          // 4 byte for each integer
          size_t this_read_offset = (this->offsets[frontier[i]] + j) * 4;

          if (this_read_offset / 512 * 512 == pre_read_buffer_offset) {
            // no need to read again
            bo_offsets_map[0][i * n_neighbors + j] =
                (this_read_offset % 512) / 4;
            bo_buffer_offsets_map[0][i * n_neighbors + j] = ++cur_buffer_offset;
            continue;
          }
          cur_buffer_offset = 0;
          pre_read_buffer_offset = this_read_offset / 512 * 512;
          bo_buffer_offsets_map[0][i * n_neighbors + j] = 0;

          // std::cout << "A this_read_offset: " << (this->offsets[frontier[i]]
          // + j)
          //           << " " << this_read_offset << " "
          //           << this_read_offset / 512 * 512 << " "
          //           << this_read_offset % 512 / 4 << std::endl;
          // bo_raw_sample_map is 4 byte for each integer
          auto re = pread(this->edge_file_handler,
                          (void *)(this->bo_raw_sample_map[0] +
                                   (i * n_neighbors + j) * 128),
                          512, this_read_offset / 512 * 512);
          if (re <= 0) {
            std::cerr << "ERR: pread failed: "
                      << " error: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
          }
          // 4 byte for each integer
          bo_offsets_map[0][i * n_neighbors + j] = (this_read_offset % 512) / 4;
        }
        for (size_t j = degree; j < n_neighbors; j++) {
          bo_offsets_map[0][i * n_neighbors + j] = -1;
        }
      } else {
        std::uniform_int_distribution<> dis(0, get_degree(frontier[i]) - 1);
        size_t pre_read_buffer_offset = -1;
        uint32_t cur_buffer_offset = 0;
        for (size_t j = 0; j < n_neighbors; j++) {
          size_t this_read_offset =
              uint64_t(dis(gen) + this->offsets[frontier[i]]) * 4;

          if (this_read_offset / 512 * 512 == pre_read_buffer_offset) {
            // no need to read again
            bo_offsets_map[0][i * n_neighbors + j] =
                (this_read_offset % 512) / 4;
            bo_buffer_offsets_map[0][i * n_neighbors + j] = ++cur_buffer_offset;
            continue;
          }
          cur_buffer_offset = 0;
          pre_read_buffer_offset = this_read_offset / 512 * 512;
          bo_buffer_offsets_map[0][i * n_neighbors + j] = 0;
          // bo_raw_sample_map is 4 byte for each integer
          // std::cout << "B this_read_offset: " << this_read_offset << " "
          //           << this_read_offset << " " << this_read_offset / 512 *
          //           512
          //           << " " << this_read_offset % 512 / 4 << std::endl;
          auto re = pread(this->edge_file_handler,
                          (void *)(this->bo_raw_sample_map[0] +
                                   (i * n_neighbors + j) * 128),
                          512, this_read_offset / 512 * 512);
          if (re <= 0) {
            std::cerr << "ERR: pread failed: "
                      << " error: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
          }
          // 4 byte for each integer
          bo_offsets_map[0][i * n_neighbors + j] = (this_read_offset % 512) / 4;
        }
      }
    }

    bo_offsets[0].sync(XCL_BO_SYNC_BO_TO_DEVICE);
    bo_buffer_offsets[0].sync(XCL_BO_SYNC_BO_TO_DEVICE);
  }

  // run the kernel
  {
    EasyTimer timer(fpga_time);
    auto run1 = this->getXrtKernel()[0](bo_raw_sample[0], bo_sample_result[0],
                                        bo_offsets[0], bo_buffer_offsets[0],
                                        frontier.size() * n_neighbors);

    run1.wait();
  }

  {
    EasyTimer timer(transfer_time);
    // sync the buffer object back to host
    this->bo_sample_result[0].sync(XCL_BO_SYNC_BO_FROM_DEVICE,
                                   frontier.size() * n_neighbors * sizeof(int),
                                   0);
    // std::copy(bo_sample_result_map[0],
    //           bo_sample_result_map[0] + frontier.size() * n_neighbors,
    //           std::back_inserter(result));
    std::copy_if(bo_sample_result_map[0],
                 bo_sample_result_map[0] + frontier.size() * n_neighbors,
                 std::back_inserter(result),
                 [](uint i) { return i != static_cast<uint>(-1); });
  }

  return result;
}

std::vector<std::vector<uint>>
RandomReadSampler::getSample(std::vector<uint> frontier) {
  std::vector<std::vector<uint>> result;
  // sample for each layer
  for (size_t i = 0; i < this->getFanouts().size(); i++) {
    // std::cout << "Sample for layer " << i << std::endl;
    std::vector<uint> sample_result =
        sampleOneLayer(frontier, this->getFanouts()[i]);

    // deduplicate the sample result
    std::sort(sample_result.begin(), sample_result.end());
    auto last = std::unique(sample_result.begin(), sample_result.end());
    sample_result.erase(last, sample_result.end());

    result.push_back(sample_result);

    // std::cout << "sample_result size: " << sample_result.size() << std::endl;

    // the sampled result is the frontier for the next layer
    if (i < this->getFanouts().size() - 1) {
      // frontier.clear();
      // std::copy_if(sample_result.begin(), sample_result.end(),
      //              std::back_inserter(frontier),
      //              [](uint i) { return i != static_cast<uint>(-1); });
      frontier = sample_result;
      // std::cout << "Next Frontier size: " << frontier.size() << std::endl;
    }
  }

  return result;
}