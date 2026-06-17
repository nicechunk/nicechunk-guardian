#pragma once

#include <cstdint>

namespace nc {

struct Metrics {
  uint64_t connections = 0;
  uint64_t players = 0;
  uint64_t active_chunks = 0;

  uint64_t move_in = 0;
  uint64_t dig_in = 0;
  uint64_t messages_out = 0;
  uint64_t bytes_in = 0;
  uint64_t bytes_out = 0;
  uint64_t move_batches = 0;
  uint64_t move_items = 0;
  uint64_t dropped_move_packets = 0;
  uint64_t dropped_slow_clients = 0;
  uint64_t backpressure_connections = 0;

  uint64_t last_print_ms = 0;
  uint64_t last_move_in = 0;
  uint64_t last_dig_in = 0;
  uint64_t last_messages_out = 0;
  uint64_t last_bytes_in = 0;
  uint64_t last_bytes_out = 0;
  uint64_t last_move_batches = 0;
};

} // namespace nc
