#pragma once

#include "protocol.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace nc {

struct ChunkRoom;

struct Player {
  uint16_t local_player_id = 0;
  std::array<uint8_t, 32> wallet_pubkey{};
  uint32_t owner_hash = 0;
  uint64_t owner_fingerprint = 0;

  int32_t current_chunk_x = 0;
  int32_t current_chunk_z = 0;
  uint8_t local_chunk_x = 0;
  uint8_t local_chunk_z = 0;

  uint16_t pos_x = 0;
  uint16_t pos_y = 0;
  uint16_t pos_z = 0;
  uint8_t yaw = 0;
  int8_t pitch = 0;
  bool has_pose = false;

  uint64_t connected_at_ms = 0;
  uint64_t last_seen_ms = 0;
  uint64_t last_move_ms = 0;
  uint16_t last_dig_seq = 0;
  bool has_dig_seq = false;
  bool has_equipment = false;
  bool has_identity = false;
  std::string display_name;
  uint16_t equipment_seq = 0;
  uint8_t equipment_right_hand_kind = 0;
  uint8_t equipment_right_hand_variant = 0;
  uint8_t equipment_flags = 0;
  uint32_t equipment_design_hash = 0;
  std::string equipment_payload;

  uint64_t move_window_ms = 0;
  uint16_t move_count = 0;
  uint64_t dig_window_ms = 0;
  uint16_t dig_count = 0;
  uint64_t equipment_window_ms = 0;
  uint16_t equipment_count = 0;
  uint64_t building_window_ms = 0;
  uint16_t building_count = 0;

  ChunkRoom *current_chunk_room = nullptr;
  size_t index_in_chunk_vector = 0;

  bool has_pending_move = false;
  bool queued_pending_move = false;
  MoveItem pending_move{};
  bool connected = true;
};

bool rate_limit_allow(uint64_t now_ms, uint64_t &window_ms, uint16_t &count, uint16_t limit_per_sec);

} // namespace nc
