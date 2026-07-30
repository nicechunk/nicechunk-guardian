#pragma once

#include "aoi.h"
#include "chunk.h"
#include "config.h"
#include "metrics.h"
#include "player.h"
#include "protocol.h"

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace nc {

enum class TuiCommand {
  NextSection,
  PreviousSection,
  RowDown,
  RowUp,
  Section1,
  Section2,
  Section3,
  Section4,
  Section5,
};

struct StatsRates {
  uint64_t move_in_per_sec = 0;
  uint64_t dig_in_per_sec = 0;
  uint64_t messages_out_per_sec = 0;
  uint64_t bytes_in_per_sec = 0;
  uint64_t bytes_out_per_sec = 0;
  uint64_t move_batches_per_sec = 0;
  double avg_move_batch_count = 0.0;
  double cpu_percent = 0.0;
  uint64_t memory_usage_mb = 0;
};

struct TuiLogEntry {
  uint64_t timestamp_ms = 0;
  std::string message;
};

struct RetiredPlayerNotice {
  int32_t chunk_x = 0;
  int32_t chunk_z = 0;
  PlayerLeave leave{};
};

class GuardianState {
public:
  explicit GuardianState(Config cfg);

  const Config &config() const { return cfg_; }
  Metrics &metrics() { return metrics_; }
  const Metrics &metrics() const { return metrics_; }
  uint16_t server_tick() const { return server_tick_; }

  bool can_accept_player() const;
  Player *create_player(const Hello &hello, uint64_t now_ms);
  void remove_player(Player *player);
  bool is_active_player(const Player *player) const;
  void release_retired_player(Player *player);
  std::vector<RetiredPlayerNotice> retire_players_with_wallet(const std::array<uint8_t, 32> &wallet);

  ChunkRoom *get_or_create_room(int32_t chunk_x, int32_t chunk_z);
  ChunkRoom *find_room(int32_t chunk_x, int32_t chunk_z);
  void add_player_to_room(Player *player, ChunkRoom *room);
  void remove_player_from_room(Player *player);

  bool move_player_chunk(Player *player, int32_t chunk_x, int32_t chunk_z);
  void update_player_move(Player *player, const Move &move, uint64_t now_ms);
  void update_player_equipment(Player *player, const Equipment &equipment, uint64_t now_ms);
  void update_player_identity(Player *player, const PlayerIdentity &identity, uint64_t now_ms);
  bool accept_dig_seq(Player *player, uint16_t seq);

  PlayerJoin make_join(const Player &player) const;
  PlayerLeave make_leave(const Player &player, uint8_t reason) const;
  MoveItem make_move_item(const Player &player) const;
  DigEvent make_dig_event(const Player &player, const Dig &dig) const;
  EquipmentEvent make_equipment_event(const Player &player) const;
  PlayerIdentity make_identity(const Player &player) const;

  using PublishFn = std::function<void(std::string_view, std::string_view)>;
  void publish_to_aoi(int32_t center_chunk_x, int32_t center_chunk_z, std::string_view payload, const PublishFn &publish);
  void flush_pending_moves(const PublishFn &publish);
  void collect_snapshot(Player *viewer, std::vector<PlayerJoin> &out) const;
  void collect_equipment_snapshot(Player *viewer, std::vector<EquipmentEvent> &out) const;
  void collect_identity_snapshot(Player *viewer, std::vector<PlayerIdentity> &out) const;

  BuildingRegionDigest building_digest() const;
  std::vector<std::string> building_manifest_pages() const;
  std::string building_manifest_binary() const;
  std::string building_manifest_etag() const;
  bool upsert_building(const BuildingRecord &record);

  const std::string &topic_for_chunk(int32_t chunk_x, int32_t chunk_z) const;
  bool local_to_global(uint8_t local_x, uint8_t local_z, int32_t &chunk_x, int32_t &chunk_z) const;

  void print_stats(uint64_t now_ms);
  void add_log(std::string message);
  void apply_tui_command(TuiCommand command);
  std::string render_tui(uint64_t now_ms);

private:
  Config cfg_;
  Metrics metrics_{};
  uint16_t next_local_player_id_ = 1;
  uint16_t server_tick_ = 0;
  std::unordered_map<uint16_t, std::unique_ptr<Player>> players_;
  std::deque<std::unique_ptr<Player>> retired_players_;
  std::unordered_map<uint64_t, std::unique_ptr<ChunkRoom>> rooms_;
  std::vector<std::string> topics_;
  std::vector<AoiOffset> aoi_offsets_;
  std::vector<Player *> pending_move_players_;
  std::deque<TuiLogEntry> tui_logs_;
  uint8_t tui_section_ = 0;
  uint32_t tui_row_ = 0;
  double last_cpu_seconds_ = 0.0;
  int32_t building_region_x_ = 0;
  int32_t building_region_z_ = 0;
  uint64_t building_revision_ = 0;
  std::array<uint8_t, 16> building_hash_{};
  std::map<uint64_t, BuildingRecord> building_records_;

  uint16_t allocate_player_id();
  StatsRates update_rates(uint64_t now_ms);
  bool valid_building_record(const BuildingRecord &record) const;
  void recompute_building_hash();
  void load_building_manifest();
  void save_building_manifest() const;
};

} // namespace nc
