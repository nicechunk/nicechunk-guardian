#pragma once

#include "config.h"

#include <cstdint>
#include <string>
#include <vector>

namespace nc {

struct Player;

struct ChunkRoom {
  int32_t chunk_x = 0;
  int32_t chunk_z = 0;
  uint32_t local_chunk_index = 0;
  std::string topic;
  std::vector<Player *> players;
};

uint64_t chunk_key(int32_t chunk_x, int32_t chunk_z);
bool contains_chunk(const Config &cfg, int32_t chunk_x, int32_t chunk_z);
uint8_t to_local_chunk_x(const Config &cfg, int32_t chunk_x);
uint8_t to_local_chunk_z(const Config &cfg, int32_t chunk_z);
int32_t from_local_chunk_x(const Config &cfg, uint8_t local_chunk_x);
int32_t from_local_chunk_z(const Config &cfg, uint8_t local_chunk_z);
uint32_t local_chunk_index(const Config &cfg, int32_t chunk_x, int32_t chunk_z);

} // namespace nc
