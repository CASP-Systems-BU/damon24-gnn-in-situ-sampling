#include "SamplerBase.hpp"
#include <cassert>
#include <iostream>
#include <vector>

class TestSamplerBase : public SamplerBase {
public:
  TestSamplerBase(std::vector<uint> fanouts) : SamplerBase(fanouts) {}
  std::vector<std::vector<uint>>
  getSample(std::vector<uint> frontier) override {
    std::vector<std::vector<uint>> result = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    return result;
  }
};

int main() {
  std::vector<uint> fanouts = {20, 15, 10};
  TestSamplerBase testSamplerBase(fanouts);
  std::vector<uint> frontier = {1, 2, 3};
  // std::vector<std::vector<int>> result = testSamplerBase.getSample(frontier);
  // std::cout << "number of layers: " << testSamplerBase.getFanouts().size()
  //           << std::endl;
  assert(testSamplerBase.getFanouts().size() == 3 &&
         "number of layers is not correct");

  testSamplerBase.setFanouts({20, 20, 20, 20});
  assert(testSamplerBase.getFanouts().size() == 4 &&
         "number of layers is not correct");
  return 0;
}
