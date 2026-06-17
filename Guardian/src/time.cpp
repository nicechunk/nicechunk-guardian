#include "nc_time.h"

#include <chrono>

namespace nc {

uint64_t now_ms() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

} // namespace nc
