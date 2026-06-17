#include "aoi.h"

static bool has(const std::vector<nc::AoiOffset> &offsets, int dx, int dz) {
  for (const auto &offset : offsets) {
    if (offset.dx == dx && offset.dz == dz) return true;
  }
  return false;
}

int main() {
  auto offsets = nc::build_aoi_offsets(7);
  if (offsets.size() != 225) return 1;
  if (!has(offsets, -7, -7)) return 1;
  if (!has(offsets, 0, 0)) return 1;
  if (!has(offsets, 7, 7)) return 1;
  if (has(offsets, 8, 0)) return 1;
  if (has(offsets, 0, 8)) return 1;
  return 0;
}
