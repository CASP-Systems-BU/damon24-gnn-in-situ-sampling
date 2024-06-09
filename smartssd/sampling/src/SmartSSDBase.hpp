/**
 * This file implements the basic methods to open and load kernels for a
 * SmartSSD device
 */

#ifndef SmartSSD_Base_HPP
#define SmartSSD_Base_HPP

#include "experimental/xrt_bo.h"
#include "experimental/xrt_device.h"
#include "experimental/xrt_kernel.h"
#include <sys/types.h>
#include <vector>

class SmartSSDBase {
private:
  std::vector<xrt::device> xrt_device;
  std::vector<xrt::uuid> xrt_uuid;
  std::vector<xrt::kernel> xrt_kernel;

public:
  /**
   * Constructor for the SmartSSDBase class with no argument
   */
  SmartSSDBase();

  /**
   * Constructor for the SmartSSDBase class, this will open the device only.
   * For some programs we do not need to run a FPGA kernel.
   * @param xrt_device_id: The vector of int represent ids of the XRT device
   */
  SmartSSDBase(std::vector<uint> xrt_device_id);

  /**
   * Constructor for the SmartSSDBase class, this will open the device
   * and load xclbin kernel file
   * @param xrt_device_id: The vector of int represent ids of the XRT device
   * @param xclbin_file: The xclbin file path
   * @param kernel_name: The name of the kernel
   */
  SmartSSDBase(std::vector<uint> xrt_device_id, std::string xclbin_file,
               std::string kernel_name);

  /**
   * Load device according to device id
   */
  void loadXrtDevice(std::vector<uint> xrt_device_id);

  /**
   * Get all the XRT device that are loaded
   */
  std::vector<xrt::device> getXrtDevice();

  /**
   * Load the XRT UUID to match an expected xclbin
   */
  void loadXrtUUID(std::string xclbin_file);

  /**
   * Get the XRT UUID vector
   */
  std::vector<xrt::uuid> getXrtUUID();

  /**
   * Load the XRT kernel
   */
  void loadXrtKernel(std::string kernel_name);

  /**
   * Get the kernel name
   */
  std::vector<xrt::kernel> getXrtKernel();
};

#endif // SmartSSD_Base_HPP