#include "guardian.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/resource.h>
#include <unistd.h>
#include <unordered_map>

namespace nc {
namespace {

bool wallet_is_empty(const std::array<uint8_t, 32> &wallet) {
  return std::all_of(wallet.begin(), wallet.end(), [](uint8_t value) { return value == 0; });
}

uint32_t hash_wallet(const std::array<uint8_t, 32> &wallet) {
  uint32_t hash = 2166136261u;
  for (uint8_t value : wallet) {
    hash ^= value;
    hash *= 16777619u;
  }
  return hash ? hash : 1u;
}

uint64_t fingerprint_wallet(const std::array<uint8_t, 32> &wallet) {
  uint64_t hash = 14695981039346656037ull;
  for (uint8_t value : wallet) {
    hash ^= value;
    hash *= 1099511628211ull;
  }
  return hash ? hash : 1ull;
}

} // namespace

GuardianState::GuardianState(Config cfg) : cfg_(std::move(cfg)), aoi_offsets_(build_aoi_offsets(cfg_.player_aoi_radius_chunks)) {
  uint32_t diameter = (uint32_t)cfg_.service_radius_chunks * 2u + 1u;
  topics_.reserve((size_t)diameter * diameter);
  for (uint32_t i = 0; i < diameter * diameter; ++i) {
    topics_.push_back("c/" + std::to_string(i));
  }
}

bool GuardianState::can_accept_player() const {
  return players_.size() < cfg_.max_players;
}

uint16_t GuardianState::allocate_player_id() {
  for (uint32_t i = 0; i < 65535; ++i) {
    uint16_t id = next_local_player_id_++;
    if (next_local_player_id_ == 0) next_local_player_id_ = 1;
    if (players_.find(id) == players_.end()) return id;
  }
  return 0;
}

Player *GuardianState::create_player(const Hello &hello, uint64_t t_ms) {
  if (!can_accept_player() || !contains_chunk(cfg_, hello.start_chunk_x, hello.start_chunk_z)) return nullptr;
  uint16_t id = allocate_player_id();
  if (id == 0) return nullptr;

  auto player = std::make_unique<Player>();
  player->local_player_id = id;
  player->wallet_pubkey = hello.wallet_pubkey;
  player->owner_hash = hash_wallet(hello.wallet_pubkey);
  player->owner_fingerprint = fingerprint_wallet(hello.wallet_pubkey);
  player->current_chunk_x = hello.start_chunk_x;
  player->current_chunk_z = hello.start_chunk_z;
  player->local_chunk_x = to_local_chunk_x(cfg_, hello.start_chunk_x);
  player->local_chunk_z = to_local_chunk_z(cfg_, hello.start_chunk_z);
  player->connected_at_ms = t_ms;
  player->last_seen_ms = t_ms;
  player->move_window_ms = t_ms;
  player->dig_window_ms = t_ms;

  Player *raw = player.get();
  players_.emplace(id, std::move(player));
  add_player_to_room(raw, get_or_create_room(hello.start_chunk_x, hello.start_chunk_z));
  metrics_.players = players_.size();
  return raw;
}

void GuardianState::remove_player(Player *player) {
  if (!player) return;
  remove_player_from_room(player);
  players_.erase(player->local_player_id);
  metrics_.players = players_.size();
}

bool GuardianState::is_active_player(const Player *player) const {
  if (!player) return false;
  auto it = players_.find(player->local_player_id);
  return it != players_.end() && it->second.get() == player;
}

void GuardianState::release_retired_player(Player *player) {
  if (!player) return;
  auto it = std::find_if(retired_players_.begin(), retired_players_.end(), [&](const auto &entry) {
    return entry.get() == player;
  });
  if (it != retired_players_.end()) retired_players_.erase(it);
}

std::vector<RetiredPlayerNotice> GuardianState::retire_players_with_wallet(const std::array<uint8_t, 32> &wallet) {
  std::vector<RetiredPlayerNotice> notices;
  if (wallet_is_empty(wallet)) return notices;

  for (auto it = players_.begin(); it != players_.end();) {
    Player *player = it->second.get();
    if (player->wallet_pubkey != wallet) {
      ++it;
      continue;
    }

    notices.push_back(RetiredPlayerNotice{
      player->current_chunk_x,
      player->current_chunk_z,
      make_leave(*player, 3),
    });
    remove_player_from_room(player);
    player->connected = false;
    retired_players_.push_back(std::move(it->second));
    it = players_.erase(it);
  }

  metrics_.players = players_.size();
  return notices;
}

ChunkRoom *GuardianState::get_or_create_room(int32_t chunk_x, int32_t chunk_z) {
  uint64_t key = chunk_key(chunk_x, chunk_z);
  auto it = rooms_.find(key);
  if (it != rooms_.end()) return it->second.get();

  auto room = std::make_unique<ChunkRoom>();
  room->chunk_x = chunk_x;
  room->chunk_z = chunk_z;
  room->local_chunk_index = local_chunk_index(cfg_, chunk_x, chunk_z);
  room->topic = topics_[room->local_chunk_index];
  ChunkRoom *raw = room.get();
  rooms_.emplace(key, std::move(room));
  metrics_.active_chunks = rooms_.size();
  return raw;
}

ChunkRoom *GuardianState::find_room(int32_t chunk_x, int32_t chunk_z) {
  auto it = rooms_.find(chunk_key(chunk_x, chunk_z));
  return it == rooms_.end() ? nullptr : it->second.get();
}

void GuardianState::add_player_to_room(Player *player, ChunkRoom *room) {
  player->current_chunk_room = room;
  player->index_in_chunk_vector = room->players.size();
  room->players.push_back(player);
}

void GuardianState::remove_player_from_room(Player *player) {
  ChunkRoom *room = player->current_chunk_room;
  if (!room) return;
  size_t idx = player->index_in_chunk_vector;
  if (idx < room->players.size()) {
    Player *last = room->players.back();
    room->players[idx] = last;
    last->index_in_chunk_vector = idx;
    room->players.pop_back();
  }
  player->current_chunk_room = nullptr;
  if (room->players.empty()) {
    rooms_.erase(chunk_key(room->chunk_x, room->chunk_z));
    metrics_.active_chunks = rooms_.size();
  }
}

bool GuardianState::move_player_chunk(Player *player, int32_t chunk_x, int32_t chunk_z) {
  if (!contains_chunk(cfg_, chunk_x, chunk_z)) return false;
  if (player->current_chunk_x == chunk_x && player->current_chunk_z == chunk_z) return true;
  remove_player_from_room(player);
  player->current_chunk_x = chunk_x;
  player->current_chunk_z = chunk_z;
  player->local_chunk_x = to_local_chunk_x(cfg_, chunk_x);
  player->local_chunk_z = to_local_chunk_z(cfg_, chunk_z);
  add_player_to_room(player, get_or_create_room(chunk_x, chunk_z));
  return true;
}

void GuardianState::update_player_move(Player *player, const Move &move, uint64_t t_ms) {
  player->pos_x = move.pos_x;
  player->pos_y = move.pos_y;
  player->pos_z = move.pos_z;
  player->yaw = move.yaw;
  player->pitch = move.pitch;
  player->has_pose = true;
  player->last_seen_ms = t_ms;
  player->last_move_ms = t_ms;
  player->pending_move = make_move_item(*player);
  player->has_pending_move = true;
  if (!player->queued_pending_move) {
    player->queued_pending_move = true;
    pending_move_players_.push_back(player);
  } else {
    metrics_.dropped_move_packets += 1;
  }
}

bool GuardianState::accept_dig_seq(Player *player, uint16_t seq) {
  if (player->has_dig_seq && seq == player->last_dig_seq) return false;
  player->last_dig_seq = seq;
  player->has_dig_seq = true;
  return true;
}

PlayerJoin GuardianState::make_join(const Player &player) const {
  return PlayerJoin{
    player.local_player_id,
    player.owner_hash,
    player.owner_fingerprint,
    player.local_chunk_x,
    player.local_chunk_z,
    player.pos_x,
    player.pos_y,
    player.pos_z,
    player.yaw,
    player.pitch,
  };
}

PlayerLeave GuardianState::make_leave(const Player &player, uint8_t reason) const {
  return PlayerLeave{player.local_player_id, reason, player.owner_hash, player.owner_fingerprint};
}

MoveItem GuardianState::make_move_item(const Player &player) const {
  return MoveItem{
    player.local_player_id,
    player.local_chunk_x,
    player.local_chunk_z,
    player.pos_x,
    player.pos_y,
    player.pos_z,
    player.yaw,
    player.pitch,
  };
}

DigEvent GuardianState::make_dig_event(const Player &player, const Dig &dig) const {
  return DigEvent{
    player.local_player_id,
    dig.seq,
    dig.local_chunk_x,
    dig.local_chunk_z,
    dig.block_x,
    dig.block_y,
    dig.block_z,
    dig.action,
    server_tick_,
  };
}

void GuardianState::publish_to_aoi(int32_t center_chunk_x, int32_t center_chunk_z, std::string_view payload, const PublishFn &publish) {
  for (const auto &offset : aoi_offsets_) {
    int32_t x = center_chunk_x + offset.dx;
    int32_t z = center_chunk_z + offset.dz;
    if (!contains_chunk(cfg_, x, z)) continue;
    publish(topic_for_chunk(x, z), payload);
    metrics_.messages_out += 1;
    metrics_.bytes_out += payload.size();
  }
}

void GuardianState::flush_pending_moves(const PublishFn &publish) {
  server_tick_++;
  std::unordered_map<uint32_t, std::vector<MoveItem>> batches;
  batches.reserve(pending_move_players_.size() * aoi_offsets_.size() / 8 + 8);

  for (Player *player : pending_move_players_) {
    if (!player || !player->connected || !player->has_pending_move) continue;
    for (const auto &offset : aoi_offsets_) {
      int32_t x = player->current_chunk_x + offset.dx;
      int32_t z = player->current_chunk_z + offset.dz;
      if (!contains_chunk(cfg_, x, z)) continue;
      batches[local_chunk_index(cfg_, x, z)].push_back(player->pending_move);
    }
    player->has_pending_move = false;
    player->queued_pending_move = false;
  }
  pending_move_players_.clear();

  for (auto &[topic_index, items] : batches) {
    size_t offset = 0;
    while (offset < items.size()) {
      size_t count = std::min<size_t>(255, items.size() - offset);
      std::string payload = encode_move_batch(server_tick_, items.data() + offset, count);
      publish(topics_[topic_index], payload);
      metrics_.messages_out += 1;
      metrics_.bytes_out += payload.size();
      metrics_.move_batches += 1;
      metrics_.move_items += count;
      offset += count;
    }
  }
}

void GuardianState::collect_snapshot(Player *viewer, std::vector<PlayerJoin> &out) const {
  for (const auto &offset : aoi_offsets_) {
    int32_t x = viewer->current_chunk_x + offset.dx;
    int32_t z = viewer->current_chunk_z + offset.dz;
    if (!contains_chunk(cfg_, x, z)) continue;
    auto it = rooms_.find(chunk_key(x, z));
    if (it == rooms_.end()) continue;
    for (Player *other : it->second->players) {
      if (!other->has_pose) continue;
      if (cfg_.echo_self || other != viewer) out.push_back(make_join(*other));
    }
  }
}

const std::string &GuardianState::topic_for_chunk(int32_t chunk_x, int32_t chunk_z) const {
  return topics_[local_chunk_index(cfg_, chunk_x, chunk_z)];
}

bool GuardianState::local_to_global(uint8_t local_x, uint8_t local_z, int32_t &chunk_x, int32_t &chunk_z) const {
  chunk_x = from_local_chunk_x(cfg_, local_x);
  chunk_z = from_local_chunk_z(cfg_, local_z);
  return contains_chunk(cfg_, chunk_x, chunk_z);
}

void GuardianState::print_stats(uint64_t t_ms) {
  StatsRates rates = update_rates(t_ms);

  std::cout << "stats connections=" << metrics_.connections
            << " players=" << metrics_.players
            << " active_chunks=" << metrics_.active_chunks
            << " move_in_per_sec=" << rates.move_in_per_sec
            << " dig_in_per_sec=" << rates.dig_in_per_sec
            << " messages_out_per_sec=" << rates.messages_out_per_sec
            << " bytes_in_per_sec=" << rates.bytes_in_per_sec
            << " bytes_out_per_sec=" << rates.bytes_out_per_sec
            << " move_batches_per_sec=" << rates.move_batches_per_sec
            << " avg_move_batch_count=" << std::fixed << std::setprecision(2) << rates.avg_move_batch_count
            << " dropped_move_packets=" << metrics_.dropped_move_packets
            << " dropped_slow_clients=" << metrics_.dropped_slow_clients
            << " backpressure_connections=" << metrics_.backpressure_connections
            << " memory_usage_mb=" << rates.memory_usage_mb
            << std::endl;
}

void GuardianState::add_log(std::string message) {
  tui_logs_.push_back(TuiLogEntry{0, std::move(message)});
  while (tui_logs_.size() > cfg_.tui_log_capacity) tui_logs_.pop_front();
}

void GuardianState::apply_tui_command(TuiCommand command) {
  switch (command) {
    case TuiCommand::NextSection:
      tui_section_ = (uint8_t)((tui_section_ + 1) % 5);
      tui_row_ = 0;
      break;
    case TuiCommand::PreviousSection:
      tui_section_ = (uint8_t)((tui_section_ + 4) % 5);
      tui_row_ = 0;
      break;
    case TuiCommand::RowDown:
      ++tui_row_;
      break;
    case TuiCommand::RowUp:
      if (tui_row_ > 0) --tui_row_;
      break;
    case TuiCommand::Section1:
    case TuiCommand::Section2:
    case TuiCommand::Section3:
    case TuiCommand::Section4:
    case TuiCommand::Section5:
      tui_section_ = (uint8_t)command - (uint8_t)TuiCommand::Section1;
      tui_row_ = 0;
      break;
  }
}

StatsRates GuardianState::update_rates(uint64_t t_ms) {
  StatsRates rates;
  if (metrics_.last_print_ms == 0) {
    metrics_.last_print_ms = t_ms;
    struct rusage usage {};
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
      last_cpu_seconds_ = (double)usage.ru_utime.tv_sec + (double)usage.ru_utime.tv_usec / 1000000.0 +
                          (double)usage.ru_stime.tv_sec + (double)usage.ru_stime.tv_usec / 1000000.0;
    }
    return rates;
  }

  uint64_t elapsed = t_ms - metrics_.last_print_ms;
  if (elapsed == 0) return rates;
  double sec = (double)elapsed / 1000.0;
  uint64_t move_delta = metrics_.move_in - metrics_.last_move_in;
  uint64_t dig_delta = metrics_.dig_in - metrics_.last_dig_in;
  uint64_t out_delta = metrics_.messages_out - metrics_.last_messages_out;
  uint64_t bin_delta = metrics_.bytes_in - metrics_.last_bytes_in;
  uint64_t bout_delta = metrics_.bytes_out - metrics_.last_bytes_out;
  uint64_t batch_delta = metrics_.move_batches - metrics_.last_move_batches;

  rates.move_in_per_sec = (uint64_t)(move_delta / sec);
  rates.dig_in_per_sec = (uint64_t)(dig_delta / sec);
  rates.messages_out_per_sec = (uint64_t)(out_delta / sec);
  rates.bytes_in_per_sec = (uint64_t)(bin_delta / sec);
  rates.bytes_out_per_sec = (uint64_t)(bout_delta / sec);
  rates.move_batches_per_sec = (uint64_t)(batch_delta / sec);
  rates.avg_move_batch_count = metrics_.move_batches ? (double)metrics_.move_items / (double)metrics_.move_batches : 0.0;

  struct rusage usage {};
  if (getrusage(RUSAGE_SELF, &usage) == 0) {
    double cpu_seconds = (double)usage.ru_utime.tv_sec + (double)usage.ru_utime.tv_usec / 1000000.0 +
                         (double)usage.ru_stime.tv_sec + (double)usage.ru_stime.tv_usec / 1000000.0;
    rates.cpu_percent = ((cpu_seconds - last_cpu_seconds_) / sec) * 100.0;
    last_cpu_seconds_ = cpu_seconds;
  }

  long rss_pages = 0;
  FILE *fp = fopen("/proc/self/statm", "r");
  if (fp) {
    unsigned long ignored = 0;
    if (fscanf(fp, "%lu %ld", &ignored, &rss_pages) == 2 && rss_pages > 0) {
      long page_size = sysconf(_SC_PAGESIZE);
      rates.memory_usage_mb = (uint64_t)((rss_pages * page_size) / (1024 * 1024));
    }
    fclose(fp);
  }

  metrics_.last_print_ms = t_ms;
  metrics_.last_move_in = metrics_.move_in;
  metrics_.last_dig_in = metrics_.dig_in;
  metrics_.last_messages_out = metrics_.messages_out;
  metrics_.last_bytes_in = metrics_.bytes_in;
  metrics_.last_bytes_out = metrics_.bytes_out;
  metrics_.last_move_batches = metrics_.move_batches;
  return rates;
}

} // namespace nc
