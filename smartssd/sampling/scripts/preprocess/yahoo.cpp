#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

bool isLittleEndian() {
  int num = 1;
  return *(char *)&num == 1;
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    std::cerr << "Preprocesing Yahoo dataset: " << argv[0]
              << " <input_file> <output_degree_file> <output_edge_file>"
              << std::endl;
    return 1;
  }

  // the system must be little-endian to make sure the binary file is created
  // correctly
  if (!isLittleEndian()) {
    std::cerr << "This program only works on little-endian machines"
              << std::endl;
    return 1;
  }

  FILE *input_file = fopen(argv[1], "r");
  if (!input_file) {
    std::cerr << "Failed to open input file" << std::endl;
    return 1;
  }
  FILE *degree_file = fopen(argv[2], "wb");
  FILE *edge_file = fopen(argv[3], "wb");
  if (!degree_file || !edge_file) {
    std::cerr << "Failed to open output file" << std::endl;
    return 1;
  }

  // node 0-3
  for (size_t i = 0; i < 4; i++) {
    // write 0 to degree file
    uint32_t zero = 0;
    // fprintf(degree_file, "%u\n", zero);
    fwrite(&zero, sizeof(uint32_t), 1, degree_file);
  }

  uint32_t pre_src = -1, src, degree, dst;
  while (fscanf(input_file, "%u %u", &src, &degree) == 2) {
    if (src % 500000 == 0) {
      std::cout << "\rProcessing node " << src << "/1413511393, "
                << src / 1413511393.0 * 100 << "%";
      std::cout.flush();
    }
    // check accending
    if (pre_src != -1 && pre_src + 1 != src) {
      throw std::runtime_error(
          "src not consecutive accending!! pre_src=" + std::to_string(pre_src) +
          " src=" + std::to_string(src));
    }
    pre_src = src;

    // write degree to degree file
    // fprintf(degree_file, "%u\n", degree);
    fwrite(&degree, sizeof(uint32_t), 1, degree_file);

    for (size_t i = 0; i < degree; i++) {
      fscanf(input_file, "%u", &dst);
      // write edge to edge file
      fwrite(&dst, sizeof(uint32_t), 1, edge_file);

      // check min > 4
      // if (dst < 4) {
      //   throw std::runtime_error("dst < 4!! src=" + std::to_string(src) +
      //                            " dst=" + std::to_string(dst));
      // }
    }
  }

  std::cout << std::endl << "Processing done" << std::endl;
}