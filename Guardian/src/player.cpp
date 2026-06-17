#include "player.h"

namespace nc {

bool rate_limit_allow(uint64_t now_ms, uint64_t &window_ms, uint16_t &count, uint16_t limit_per_sec) {
  if (now_ms - window_ms >= 1000) {
    window_ms = now_ms;
    count = 0;
  }
  if (count >= limit_per_sec) return false;
  ++count;
  return true;
}

} // namespace nc
