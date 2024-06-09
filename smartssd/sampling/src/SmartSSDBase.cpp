#include "SmartSSDBase.hpp"

SmartSSDBase::SmartSSDBase() {}

SmartSSDBase::SmartSSDBase(std::vector<uint> xrt_device_id,
                           std::string xclbin_file, std::string kernel_name) {
  // Initialize the XRT device
  for (auto id : xrt_device_id) {

    auto device = xrt::device(id);
    auto uuid = device.load_xclbin(xclbin_file);
    auto krnl = xrt::kernel(device, uuid, kernel_name);

    this->xrt_device.push_back(device);
    this->xrt_uuid.push_back(uuid);
    this->xrt_kernel.push_back(krnl);
  }
}

SmartSSDBase::SmartSSDBase(std::vector<uint> xrt_device_id) {
  // Initialize the XRT device
  for (auto id : xrt_device_id) {
    auto device = xrt::device(id);
    this->xrt_device.push_back(device);
  }
}

void SmartSSDBase::loadXrtDevice(std::vector<uint> xrt_device_id) {
  for (auto id : xrt_device_id) {
    auto device = xrt::device(id);
    this->xrt_device.push_back(device);
  }
}

std::vector<xrt::device> SmartSSDBase::getXrtDevice() {
  return this->xrt_device;
}

void SmartSSDBase::loadXrtUUID(std::string xclbin_file) {
  for (auto device : this->xrt_device) {
    auto uuid = device.load_xclbin(xclbin_file);
    this->xrt_uuid.push_back(uuid);
  }
}

std::vector<xrt::uuid> SmartSSDBase::getXrtUUID() { return this->xrt_uuid; }

void SmartSSDBase::loadXrtKernel(std::string kernel_name) {
  for (size_t i = 0; i < this->xrt_device.size(); i++) {
    auto krnl =
        xrt::kernel(this->xrt_device[i], this->xrt_uuid[i], kernel_name);
    this->xrt_kernel.push_back(krnl);
  }
}

std::vector<xrt::kernel> SmartSSDBase::getXrtKernel() {
  return this->xrt_kernel;
}
