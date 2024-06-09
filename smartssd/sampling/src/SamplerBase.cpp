#include "SamplerBase.hpp"

SamplerBase::SamplerBase(std::vector<uint> fanouts) { this->fanouts = fanouts; }

void SamplerBase::setFanouts(std::vector<uint> fanouts) {
  this->fanouts = fanouts;
}

std::vector<uint> SamplerBase::getFanouts() { return this->fanouts; }
