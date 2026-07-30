#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace nc {

struct Config {
  std::string guardian_id = "guardian-001";
  std::string listen_host = "0.0.0.0";
  uint16_t listen_port = 8080;
  uint16_t public_port = 8080;
  std::string public_url = "ws://127.0.0.1:8080/ws";
  std::string path = "/ws";

  bool tls = false;
  std::string cert_file;
  std::string key_file;

  int32_t guardian_center_chunk_x = 0;
  int32_t guardian_center_chunk_z = 0;
  uint16_t service_radius_chunks = 100;

  uint16_t player_aoi_chunks = 15;
  uint8_t player_aoi_radius_chunks = 7;

  uint16_t chunk_size_blocks = 16;
  uint16_t position_precision = 64;

  uint32_t max_connections = 50000;
  uint32_t max_players = 60000;
  uint32_t max_players_per_chunk = 1024;

  uint16_t movement_broadcast_hz = 20;
  uint16_t client_move_rate_limit_per_sec = 30;
  uint16_t client_dig_rate_limit_per_sec = 10;
  uint16_t client_equipment_rate_limit_per_sec = 10;
  uint16_t client_building_announce_rate_limit_per_sec = 4;
  uint32_t max_building_records = 16384;
  std::string building_manifest_file = "guardian-buildings.bin";

  uint32_t max_payload_length = 4096;
  uint32_t max_backpressure = 262144;
  bool close_on_backpressure_limit = false;
  bool echo_self = true;

  bool enable_text_frame = false;
  bool enable_ws_compression = false;
  bool enable_tui = true;
  uint16_t tui_refresh_hz = 2;
  uint16_t tui_log_capacity = 200;
  uint16_t idle_timeout_sec = 15;
  uint16_t heartbeat_interval_sec = 5;
  uint16_t hello_timeout_sec = 5;
  uint16_t stats_interval_sec = 10;

  std::string log_level = "info";
};

bool load_config_file(Config &cfg, const std::string &path);
bool apply_cli_args(Config &cfg, int argc, char **argv);
bool validate_config(const Config &cfg, std::string &error);
void print_startup_guide(const char *program_name);
bool run_startup_wizard(Config &cfg, const char *program_name);
void print_startup_config(const Config &cfg);

} // namespace nc
