#include <iostream>

#include "SmartSSDBase.hpp"

int main() {
  SmartSSDBase smartSSDBase;
  std::vector<uint> xrt_device_id = {0, 1};
  std::string xclbin_file = "sample_target_nodes.xclbin";
  std::string kernel_name = "kernel_name";
  smartSSDBase.loadXrtDevice(xrt_device_id);
  smartSSDBase.loadXrtUUID(xclbin_file);
  smartSSDBase.loadXrtKernel("sample_target_nodes");

  auto devices = smartSSDBase.getXrtDevice();
  for (auto device : devices) {
    std::cout << "Device: " << device.get_info<xrt::info::device::name>()
              << "   BDF: " << device.get_info<xrt::info::device::bdf>()
              << std::endl;
  }

  auto uuids = smartSSDBase.getXrtUUID();
  for (auto uuid : uuids) {
    std::cout << "UUID: " << uuid << std::endl;
  }

  auto kernels = smartSSDBase.getXrtKernel();
  std::cout << "number of kernels: " << kernels.size() << std::endl;

  return 0;
}