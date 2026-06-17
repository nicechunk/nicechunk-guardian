#include "chunk.h"

namespace nc {

uint64_t chunk_key(int32_t chunk_x, int32_t chunk_z) {
  return ((uint64_t)(uint32_t)chunk_x << 32) | (uint32_t)chunk_z;
}

bool contains_chunk(const Config &cfg, int32_t chunk_x, int32_t chunk_z) {
  int64_t cx = cfg.guardian_center_chunk_x;
  int64_t cz = cfg.guardian_center_chunk_z;
  int64_t r = cfg.service_radius_chunks;
  return (int64_t)chunk_x >= cx - r &&
         (int64_t)chunk_x <= cx + r &&
         (int64_t)chunk_z >= cz - r &&
         (int64_t)chunk_z <= cz + r;
}

uint8_t to_local_chunk_x(const Config &cfg, int32_t chunk_x) {
  return (uint8_t)((int64_t)chunk_x - ((int64_t)cfg.guardian_center_chunk_x - cfg.service_radius_chunks));
}

uint8_t to_local_chunk_z(const Config &cfg, int32_t chunk_z) {
  return (uint8_t)((int64_t)chunk_z - ((int64_t)cfg.guardian_center_chunk_z - cfg.service_radius_chunks));
}

int32_t from_local_chunk_x(const Config &cfg, uint8_t local_chunk_x) {
  return (int32_t)((int64_t)cfg.guardian_center_chunk_x - cfg.service_radius_chunks + local_chunk_x);
}

int32_t from_local_chunk_z(const Config &cfg, uint8_t local_chunk_z) {
  return (int32_t)((int64_t)cfg.guardian_center_chunk_z - cfg.service_radius_chunks + local_chunk_z);
}

uint32_t local_chunk_index(const Config &cfg, int32_t chunk_x, int32_t chunk_z) {
  uint32_t diameter = (uint32_t)cfg.service_radius_chunks * 2u + 1u;
  return (uint32_t)to_local_chunk_z(cfg, chunk_z) * diameter + (uint32_t)to_local_chunk_x(cfg, chunk_x);
}

} // namespace nc
