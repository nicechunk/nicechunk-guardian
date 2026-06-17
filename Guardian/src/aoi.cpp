#include "aoi.h"

namespace nc {

std::vector<AoiOffset> build_aoi_offsets(uint8_t radius) {
  std::vector<AoiOffset> offsets;
  offsets.reserve((size_t)(radius * 2 + 1) * (size_t)(radius * 2 + 1));
  for (int dz = -(int)radius; dz <= (int)radius; ++dz) {
    for (int dx = -(int)radius; dx <= (int)radius; ++dx) {
      offsets.push_back(AoiOffset{(int8_t)dx, (int8_t)dz});
    }
  }
  return offsets;
}

} // namespace nc
