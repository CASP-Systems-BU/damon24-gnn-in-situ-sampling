# In situ neighborhood sampling for large-scale GNN training
This repository includes the implementation of the in situ GNN sampling described in the [DaMoN'24 paper](https://dl.acm.org/doi/abs/10.1145/3662010.3663443) by Yuhang Song, Po Hao Chen, Yuchen Lu, Naima Abrar, Vasiliki Kalavri.

You can cite the paper using the BibTeX below:

```
@inproceedings{10.1145/3662010.3663443,
author = {Song, Yuhang and Chen, Po Hao and Lu, Yuchen and Abrar, Naima and Kalavri, Vasiliki},
title = {In situ neighborhood sampling for large-scale GNN training},
year = {2024},
isbn = {9798400706677},
publisher = {Association for Computing Machinery},
address = {New York, NY, USA},
url = {https://doi.org/10.1145/3662010.3663443},
doi = {10.1145/3662010.3663443},
abstract = {Graph Neural Network (GNN) training algorithms commonly perform neighborhood sampling to construct fixed-size mini-batches for weight aggregation on GPUs. State-of-the-art disk-based GNN frameworks compute sampling on the CPU, transferring edge partitions from disk to memory for every mini-batch. We argue that this design incurs significant waste of PCIe bandwidth, as entire neighborhoods are transferred to main memory only to be discarded after sampling. In this paper, we make the first step towards an inherently different approach that harnesses near-storage compute technology to achieve efficient large-scale GNN training. We target a single machine with one or more SmartSSD devices and develop a high-throughput, epoch-wide sampling FPGA kernel that enables pipelining across epochs. When compared to a baseline random-access sampling kernel, our solution achieves up to 4.26\texttimes{} lower sampling time per epoch.},
booktitle = {Proceedings of the 20th International Workshop on Data Management on New Hardware},
articleno = {11},
numpages = {5},
keywords = {GNN training, SmartSSD, graph neural networks},
location = {Santiago, AA, Chile},
series = {DaMoN '24}
}
```

**NOTICE**: This is an academic proof-of-concept prototype and has not received careful code review. This implementation is NOT ready for production use.

## Building and running the GNN sampling

### Repository organization

All the code is located in the smartssd/sampling folder. The repository is organized as follows:


- The `src` folder contains the core functionality of GNN sampling, including the implementation of various sampling algorithms and its communication with the SmartSSD FPGA program.
- The `scripts` folder contains the code to run the pre-processing, formatting the edge and node files from the raw data to the format favored by our GNN sampling code.
- The `tests` folder contains various unit tests. We import the main library code from src and run the benchmarks with the code here.
- The `kernels` folder contains the implementation of various SmartSSD FPGA kernels. These codes are HLS code that needs to be compiled by Xilinx Vitis.


### Dependencies
To build GNN sampler, you will need to install:
- CMake >= 3.13
- [Xilinx XRT, Deployment Target Platform and Development Target Platform](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/alveo/smartssd.html)
- [Xilinx Vitis](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vitis.html)

    
### Building and running using the `CMakeLists.txt` file:

- Make sure you have Xilinx Vitis installed. cmake will try to automatically determine your cmake installation location. If it's not in the default location, you might need to activate the environment first.
    ```
    source /opt/tools/Xilinx/Vitis/2021.2/settings64.sh 
    source /opt/xilinx/xrt/setup.sh
    ```
    However, the default cmake brought by Xilinx's installation is outdated. If you activate the Vitis environment, you need to use `/usr/bin/cmake` instead of `cmake` in the next instruction.

- First, use cmake to create the build directory and make file:
    ```
    mkdir build
    cd build
    cmake ..
    ```
- To compile the library and tests:
   ```
   make -j
   ```
- To compile the FPGA kernel:
   ```
   make KERNEL_NAME

   # For example
   # make parallel_streaming_sampler
   # make simple_random_read_sampler
   ```
- Now you can run the test:
   ```
   ./test_streaming_sampler
   ```
      
## License
This repository is distributed under the terms of the Apache License (Version 2.0).
