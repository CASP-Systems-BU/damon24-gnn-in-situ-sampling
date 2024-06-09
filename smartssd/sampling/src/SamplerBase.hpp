/**
 * This file defines the method that a sampler will expose to the framework
 */
#ifndef SamplerBase_HPP
#define SamplerBase_HPP
#include <sys/types.h>
#include <vector>

class SamplerBase {
private:
  std::vector<uint> fanouts;

public:
  SamplerBase() {}

  /**
   * Constructor for the SamplerBase class with sample size for each layer
   * @param fanouts: The number of neightbors to sample for each layer
   */
  SamplerBase(std::vector<uint> fanouts);

  /**
   * Set the number of neightbors to sample. The size of fanouts represent the
   * number of layers of our sample.
   */
  void setFanouts(std::vector<uint> fanouts);

  /**
   * Get the number of neightbors to sample
   */
  std::vector<uint> getFanouts();

  /**
   * Get the sample for the given frontier.
   * @param frontier: The frontier that we want to sample
   */
  virtual std::vector<std::vector<uint>>
  getSample(std::vector<uint> frontier) = 0;
};

#endif // SamplerBase_HPP