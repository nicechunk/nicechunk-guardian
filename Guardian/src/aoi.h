#pragma once

#include <cstdint>
#include <vector>

namespace nc {

struct AoiOffset {
  int8_t dx = 0;
  int8_t dz = 0;
};

std::vector<AoiOffset> build_aoi_offsets(uint8_t radius);

} // namespace nc
