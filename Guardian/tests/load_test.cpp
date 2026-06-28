#include "aoi.h"
#include "chunk.h"
#include "config.h"
#include "player.h"
#include "protocol.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <unordered_set>
#include <vector>

namespace {

constexpr size_t kVirtualPlayers = 4096;
constexpr size_t kTicks = 240;
constexpr size_t kMoveBatchLimit = 255;
constexpr uint8_t kAoiRadius = 7;
constexpr int64_t kMaxElapsedMs = 10000;

struct Counters {
  uint64_t aoi_fanout_checks = 0;
  uint64_t in_range_checks = 0;
  uint64_t local_roundtrip_checks = 0;
  uint64_t encoded_batches = 0;
  uint64_t encoded_bytes = 0;
};

bool fail(const char *message) {
  std::cerr << "load_test: " << message << "\n";
  return false;
}

bool verify_rate_limit() {
  uint64_t window_ms = 0;
  uint16_t count = 0;
  for (uint16_t i = 0; i < 30; ++i) {
    if (!nc::rate_limit_allow(10, window_ms, count, 30)) return false;
  }
  if (nc::rate_limit_allow(10, window_ms, count, 30)) return false;
  if (!nc::rate_limit_allow(1010, window_ms, count, 30)) return false;
  return window_ms == 1010 && count == 1;
}

} // namespace

int main() {
  const auto started = std::chrono::steady_clock::now();

  nc::Config cfg;
  cfg.guardian_center_chunk_x = 1200;
  cfg.guardian_center_chunk_z = -700;
  cfg.service_radius_chunks = 100;
  cfg.player_aoi_radius_chunks = kAoiRadius;

  const auto offsets = nc::build_aoi_offsets(cfg.player_aoi_radius_chunks);
  if (offsets.size() != 225) return fail("unexpected AOI offset count");

  std::vector<nc::Player> players(kVirtualPlayers);
  std::unordered_set<uint64_t> observed_chunks;
  observed_chunks.reserve(kVirtualPlayers);
  Counters counters;

  const int32_t min_x = cfg.guardian_center_chunk_x - cfg.service_radius_chunks;
  const int32_t min_z = cfg.guardian_center_chunk_z - cfg.service_radius_chunks;
  const int32_t diameter = (int32_t)cfg.service_radius_chunks * 2 + 1;

  for (size_t i = 0; i < players.size(); ++i) {
    const int32_t chunk_x = min_x + (int32_t)(i % (size_t)diameter);
    const int32_t chunk_z = min_z + (int32_t)((i / (size_t)diameter) % (size_t)diameter);
    auto &player = players[i];
    player.local_player_id = (uint16_t)(i + 1);
    player.current_chunk_x = chunk_x;
    player.current_chunk_z = chunk_z;
    player.local_chunk_x = nc::to_local_chunk_x(cfg, chunk_x);
    player.local_chunk_z = nc::to_local_chunk_z(cfg, chunk_z);
    player.pos_x = (uint16_t)(i % 32768);
    player.pos_y = 64;
    player.pos_z = (uint16_t)((i * 3) % 32768);
    player.yaw = (uint8_t)(i % 256);
    player.pitch = (int8_t)((int)(i % 61) - 30);

    if (!nc::contains_chunk(cfg, player.current_chunk_x, player.current_chunk_z)) {
      return fail("player placed outside service range");
    }
    ++counters.in_range_checks;

    if (nc::from_local_chunk_x(cfg, player.local_chunk_x) != player.current_chunk_x) {
      return fail("local chunk x roundtrip failed");
    }
    if (nc::from_local_chunk_z(cfg, player.local_chunk_z) != player.current_chunk_z) {
      return fail("local chunk z roundtrip failed");
    }
    ++counters.local_roundtrip_checks;

    observed_chunks.insert(nc::chunk_key(player.current_chunk_x, player.current_chunk_z));
  }

  if (nc::contains_chunk(cfg, min_x - 1, cfg.guardian_center_chunk_z)) {
    return fail("accepted chunk left of service range");
  }
  if (nc::contains_chunk(cfg, cfg.guardian_center_chunk_x, min_z - 1)) {
    return fail("accepted chunk above service range");
  }

  std::vector<nc::MoveItem> batch;
  batch.reserve(kMoveBatchLimit);

  for (size_t tick = 0; tick < kTicks; ++tick) {
    batch.clear();
    for (size_t i = 0; i < players.size(); ++i) {
      auto &player = players[i];
      player.pos_x = (uint16_t)((player.pos_x + 3 + tick) % 32768);
      player.pos_z = (uint16_t)((player.pos_z + 5 + i) % 32768);
      player.yaw = (uint8_t)(player.yaw + 1);

      for (const auto &offset : offsets) {
        const int32_t nearby_x = player.current_chunk_x + offset.dx;
        const int32_t nearby_z = player.current_chunk_z + offset.dz;
        if (nc::contains_chunk(cfg, nearby_x, nearby_z)) {
          (void)nc::chunk_key(nearby_x, nearby_z);
        }
        ++counters.aoi_fanout_checks;
      }

      batch.push_back(nc::MoveItem{
        player.local_player_id,
        player.local_chunk_x,
        player.local_chunk_z,
        player.pos_x,
        player.pos_y,
        player.pos_z,
        player.yaw,
        player.pitch,
      });

      if (batch.size() == kMoveBatchLimit) {
        const auto encoded = nc::encode_move_batch((uint16_t)tick, batch.data(), batch.size());
        if (encoded.size() != nc::kMoveBatchHeaderSize + batch.size() * nc::kMoveItemSize) {
          return fail("full MOVE_BATCH encoded size mismatch");
        }
        ++counters.encoded_batches;
        counters.encoded_bytes += encoded.size();
        batch.clear();
      }
    }

    if (!batch.empty()) {
      const auto encoded = nc::encode_move_batch((uint16_t)tick, batch.data(), batch.size());
      if (encoded.size() != nc::kMoveBatchHeaderSize + batch.size() * nc::kMoveItemSize) {
        return fail("partial MOVE_BATCH encoded size mismatch");
      }
      ++counters.encoded_batches;
      counters.encoded_bytes += encoded.size();
    }
  }

  if (!verify_rate_limit()) return fail("rate limit window behavior changed");

  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now() - started
  ).count();
  if (elapsed > kMaxElapsedMs) return fail("load test exceeded elapsed-time budget");

  std::cout
    << "virtual_players=" << kVirtualPlayers
    << " ticks=" << kTicks
    << " observed_chunks=" << observed_chunks.size()
    << " aoi_fanout_checks=" << counters.aoi_fanout_checks
    << " in_range_checks=" << counters.in_range_checks
    << " local_roundtrip_checks=" << counters.local_roundtrip_checks
    << " encoded_batches=" << counters.encoded_batches
    << " encoded_bytes=" << counters.encoded_bytes
    << " elapsed_ms=" << elapsed
    << "\n";

  return 0;
}
