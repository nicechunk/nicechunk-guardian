#include "config.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace nc {
namespace {

std::string trim(std::string value) {
  auto not_space = [](unsigned char c) { return !std::isspace(c); };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
  value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
  if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
    value = value.substr(1, value.size() - 2);
  }
  return value;
}

bool parse_bool(const std::string &value) {
  return value == "true" || value == "1" || value == "yes" || value == "on";
}

bool parse_no(const std::string &value) {
  return value == "false" || value == "0" || value == "no" || value == "n" || value == "off";
}

std::string prompt_string(std::string_view label, const std::string &current) {
  std::cout << label << " [" << current << "]: " << std::flush;
  std::string value;
  std::getline(std::cin, value);
  value = trim(value);
  return value.empty() ? current : value;
}

uint16_t prompt_u16(std::string_view label, uint16_t current) {
  while (true) {
    std::string value = prompt_string(label, std::to_string(current));
    try {
      unsigned long parsed = std::stoul(value);
      if (parsed <= 65535) return (uint16_t)parsed;
    } catch (...) {
    }
    std::cout << "Enter a number from 0 to 65535.\n";
  }
}

int32_t prompt_i32(std::string_view label, int32_t current) {
  while (true) {
    std::string value = prompt_string(label, std::to_string(current));
    try {
      return std::stoi(value);
    } catch (...) {
      std::cout << "Enter a signed integer.\n";
    }
  }
}

bool prompt_bool(std::string_view label, bool current) {
  while (true) {
    std::string value = prompt_string(label, current ? "yes" : "no");
    std::string lower;
    lower.reserve(value.size());
    for (char ch : value) lower.push_back((char)std::tolower((unsigned char)ch));
    if (parse_bool(lower)) return true;
    if (parse_no(lower)) return false;
    std::cout << "Enter yes or no.\n";
  }
}

void apply_pair(Config &cfg, const std::string &key, const std::string &value) {
  if (key == "guardian_id") cfg.guardian_id = value;
  else if (key == "listen_host") cfg.listen_host = value;
  else if (key == "listen_port") cfg.listen_port = (uint16_t)std::stoul(value);
  else if (key == "public_port") cfg.public_port = (uint16_t)std::stoul(value);
  else if (key == "public_url") cfg.public_url = value;
  else if (key == "path") cfg.path = value;
  else if (key == "tls") cfg.tls = parse_bool(value);
  else if (key == "cert_file") cfg.cert_file = value;
  else if (key == "key_file") cfg.key_file = value;
  else if (key == "guardian_center_chunk_x") cfg.guardian_center_chunk_x = std::stoi(value);
  else if (key == "guardian_center_chunk_z") cfg.guardian_center_chunk_z = std::stoi(value);
  else if (key == "service_radius_chunks") cfg.service_radius_chunks = (uint16_t)std::stoul(value);
  else if (key == "player_aoi_chunks") cfg.player_aoi_chunks = (uint16_t)std::stoul(value);
  else if (key == "player_aoi_radius_chunks") cfg.player_aoi_radius_chunks = (uint8_t)std::stoul(value);
  else if (key == "chunk_size_blocks") cfg.chunk_size_blocks = (uint16_t)std::stoul(value);
  else if (key == "position_precision") cfg.position_precision = (uint16_t)std::stoul(value);
  else if (key == "max_connections") cfg.max_connections = (uint32_t)std::stoul(value);
  else if (key == "max_players") cfg.max_players = (uint32_t)std::stoul(value);
  else if (key == "max_players_per_chunk") cfg.max_players_per_chunk = (uint32_t)std::stoul(value);
  else if (key == "movement_broadcast_hz") cfg.movement_broadcast_hz = (uint16_t)std::stoul(value);
  else if (key == "client_move_rate_limit_per_sec") cfg.client_move_rate_limit_per_sec = (uint16_t)std::stoul(value);
  else if (key == "client_dig_rate_limit_per_sec") cfg.client_dig_rate_limit_per_sec = (uint16_t)std::stoul(value);
  else if (key == "client_equipment_rate_limit_per_sec") cfg.client_equipment_rate_limit_per_sec = (uint16_t)std::stoul(value);
  else if (key == "client_building_announce_rate_limit_per_sec") cfg.client_building_announce_rate_limit_per_sec = (uint16_t)std::stoul(value);
  else if (key == "max_building_records") cfg.max_building_records = (uint32_t)std::stoul(value);
  else if (key == "building_manifest_file") cfg.building_manifest_file = value;
  else if (key == "max_payload_length") cfg.max_payload_length = (uint32_t)std::stoul(value);
  else if (key == "max_backpressure") cfg.max_backpressure = (uint32_t)std::stoul(value);
  else if (key == "close_on_backpressure_limit") cfg.close_on_backpressure_limit = parse_bool(value);
  else if (key == "echo_self") cfg.echo_self = parse_bool(value);
  else if (key == "enable_text_frame") cfg.enable_text_frame = parse_bool(value);
  else if (key == "enable_ws_compression") cfg.enable_ws_compression = parse_bool(value);
  else if (key == "enable_tui") cfg.enable_tui = parse_bool(value);
  else if (key == "tui_refresh_hz") cfg.tui_refresh_hz = (uint16_t)std::stoul(value);
  else if (key == "tui_log_capacity") cfg.tui_log_capacity = (uint16_t)std::stoul(value);
  else if (key == "idle_timeout_sec") cfg.idle_timeout_sec = (uint16_t)std::stoul(value);
  else if (key == "heartbeat_interval_sec") cfg.heartbeat_interval_sec = (uint16_t)std::stoul(value);
  else if (key == "hello_timeout_sec") cfg.hello_timeout_sec = (uint16_t)std::stoul(value);
  else if (key == "stats_interval_sec") cfg.stats_interval_sec = (uint16_t)std::stoul(value);
  else if (key == "log_level") cfg.log_level = value;
}

} // namespace

bool load_config_file(Config &cfg, const std::string &path) {
  std::ifstream in(path);
  if (!in) return false;

  std::string line;
  while (std::getline(in, line)) {
    auto comment = line.find('#');
    if (comment != std::string::npos) line.resize(comment);
    auto eq = line.find('=');
    if (eq == std::string::npos) continue;
    auto key = trim(line.substr(0, eq));
    auto value = trim(line.substr(eq + 1));
    if (!key.empty() && !value.empty()) apply_pair(cfg, key, value);
  }
  return true;
}

bool apply_cli_args(Config &cfg, int argc, char **argv) {
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    auto need_value = [&](const char *name) -> std::string {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for " << name << "\n";
        std::exit(2);
      }
      return argv[++i];
    };

    if (arg == "--config") load_config_file(cfg, need_value("--config"));
    else if (arg == "--host") cfg.listen_host = need_value("--host");
    else if (arg == "--port") {
      cfg.listen_port = (uint16_t)std::stoul(need_value("--port"));
      cfg.public_port = cfg.listen_port;
    } else if (arg == "--public-url") cfg.public_url = need_value("--public-url");
    else if (arg == "--center-x") cfg.guardian_center_chunk_x = std::stoi(need_value("--center-x"));
    else if (arg == "--center-z") cfg.guardian_center_chunk_z = std::stoi(need_value("--center-z"));
    else if (arg == "--service-radius") cfg.service_radius_chunks = (uint16_t)std::stoul(need_value("--service-radius"));
    else if (arg == "--aoi") {
      cfg.player_aoi_chunks = (uint16_t)std::stoul(need_value("--aoi"));
      cfg.player_aoi_radius_chunks = (uint8_t)(cfg.player_aoi_chunks / 2);
    } else if (arg == "--tls") cfg.tls = true;
    else if (arg == "--no-tls") cfg.tls = false;
    else if (arg == "--tui") cfg.enable_tui = true;
    else if (arg == "--no-tui") cfg.enable_tui = false;
    else if (arg == "--idle-timeout") cfg.idle_timeout_sec = (uint16_t)std::stoul(need_value("--idle-timeout"));
    else if (arg == "--heartbeat") cfg.heartbeat_interval_sec = (uint16_t)std::stoul(need_value("--heartbeat"));
    else if (arg == "--cert") cfg.cert_file = need_value("--cert");
    else if (arg == "--key") cfg.key_file = need_value("--key");
    else if (arg == "--help") {
      print_startup_guide(argv[0]);
      std::exit(0);
    } else {
      std::cerr << "Unknown argument: " << arg << "\n";
      return false;
    }
  }
  return true;
}

void print_startup_guide(const char *program_name) {
  const char *bin = program_name && program_name[0] ? program_name : "nicechunk-guardian";
  std::cout
    << "NiceChunk Guardian\n"
    << "\n"
    << "Guardian is a regional WebSocket relay node for nearby player movement and events.\n"
    << "It is not an authoritative game server and does not settle assets or world state.\n"
    << "\n"
    << "Quick start, local WS:\n"
    << "  " << bin << " --config ./config/guardian.example.conf --host 0.0.0.0 --port 8080 --public-url ws://YOUR_IP:8080/ws\n"
    << "\n"
    << "Quick start, node-owned WSS behind Nginx or Caddy:\n"
    << "  " << bin << " --config ./config/guardian.example.conf --host 127.0.0.1 --port 8080 --public-url wss://guardian.example.com/ws\n"
    << "\n"
    << "Direct TLS mode:\n"
    << "  " << bin << " --config ./config/guardian.example.conf --host 0.0.0.0 --port 443 --public-url wss://guardian.example.com/ws --tls --cert /path/fullchain.pem --key /path/privkey.pem\n"
    << "\n"
    << "Common options:\n"
    << "  --config PATH              Load guardian.conf style config.\n"
    << "  --host HOST                Listen host, for example 0.0.0.0 or 127.0.0.1.\n"
    << "  --port PORT                Listen port. 443 is optional, not required.\n"
    << "  --public-url URL           Full client endpoint, including ws:// or wss:// and /ws.\n"
    << "  --center-x N --center-z N  Guardian service center chunk.\n"
    << "  --service-radius N         Service radius in chunks. Default 100.\n"
    << "  --aoi 15                   Nearby broadcast diameter. Default 15 chunks.\n"
    << "  --tls --cert PATH --key PATH\n"
    << "                             Enable direct WSS with certificate and private key.\n"
    << "  --heartbeat N              App heartbeat interval in seconds. Default 5.\n"
    << "  --idle-timeout N           Disconnect silent sockets after N seconds. Default 15.\n"
    << "  --tui | --no-tui           Enable or disable the terminal dashboard.\n"
    << "  --help                     Show this guide.\n"
    << "\n"
    << "For this test node:\n"
    << "  " << bin << " --config ./config/guardian.example.conf --host 0.0.0.0 --port 8080 --public-url wss://guardian.203.0.113.10.sslip.io/ws\n"
    << "\n"
    << "If port 8080 is already in use, stop the system service first:\n"
    << "  sudo systemctl stop nicechunk-guardian\n";
}

bool run_startup_wizard(Config &cfg, const char *program_name) {
  const char *bin = program_name && program_name[0] ? program_name : "nicechunk-guardian";
  std::cout
    << "NiceChunk Guardian setup\n"
    << "\n"
    << "Press Enter to accept a default. The final Public URL is the value to register on-chain.\n"
    << "Use ws:// for plain WebSocket, or wss:// when TLS is handled by this process or by Nginx/Caddy.\n"
    << "\n";

  std::string config_path = prompt_string("Config file to load, or - to skip", "./config/guardian.example.conf");
  if (!config_path.empty() && config_path != "-") {
    if (load_config_file(cfg, config_path)) {
      std::cout << "Loaded " << config_path << "\n";
    } else {
      std::cout << "Could not load " << config_path << ", continuing with built-in defaults.\n";
    }
  }

  cfg.guardian_id = prompt_string("Guardian ID", cfg.guardian_id);
  cfg.listen_host = prompt_string("Listen host", cfg.listen_host);
  cfg.listen_port = prompt_u16("Listen port", cfg.listen_port);
  cfg.public_port = cfg.listen_port;
  cfg.path = prompt_string("WebSocket path", cfg.path);

  std::string suggested_url = cfg.public_url;
  if (suggested_url == "ws://127.0.0.1:8080/ws") {
    suggested_url = "ws://YOUR_PUBLIC_IP:" + std::to_string(cfg.listen_port) + cfg.path;
  }
  cfg.public_url = prompt_string("Public Guardian URL for clients and chain registration", suggested_url);

  cfg.guardian_center_chunk_x = prompt_i32("Guardian center chunk X", cfg.guardian_center_chunk_x);
  cfg.guardian_center_chunk_z = prompt_i32("Guardian center chunk Z", cfg.guardian_center_chunk_z);
  cfg.service_radius_chunks = prompt_u16("Service radius in chunks", cfg.service_radius_chunks);
  cfg.player_aoi_chunks = prompt_u16("AOI diameter in chunks", cfg.player_aoi_chunks);
  cfg.player_aoi_radius_chunks = (uint8_t)(cfg.player_aoi_chunks / 2);
  cfg.tls = prompt_bool("Direct TLS in this Guardian process", cfg.tls);
  if (cfg.tls) {
    cfg.cert_file = prompt_string("TLS fullchain.pem path", cfg.cert_file);
    cfg.key_file = prompt_string("TLS privkey.pem path", cfg.key_file);
  }
  cfg.enable_tui = prompt_bool("Enable TUI dashboard", cfg.enable_tui);

  std::cout
    << "\nStartup summary\n"
    << "  Listen: " << cfg.listen_host << ":" << cfg.listen_port << cfg.path << "\n"
    << "  Public URL: " << cfg.public_url << "\n"
    << "  Region: center=(" << cfg.guardian_center_chunk_x << "," << cfg.guardian_center_chunk_z
    << ") radius=" << cfg.service_radius_chunks << "\n"
    << "  AOI: " << cfg.player_aoi_chunks << "x" << cfg.player_aoi_chunks << "\n"
    << "  Direct TLS: " << (cfg.tls ? "yes" : "no") << "\n"
    << "\n"
    << "Equivalent command:\n"
    << "  " << bin
    << (config_path.empty() || config_path == "-" ? "" : " --config " + config_path)
    << " --host " << cfg.listen_host
    << " --port " << cfg.listen_port
    << " --public-url " << cfg.public_url
    << " --center-x " << cfg.guardian_center_chunk_x
    << " --center-z " << cfg.guardian_center_chunk_z
    << " --service-radius " << cfg.service_radius_chunks
    << " --aoi " << cfg.player_aoi_chunks
    << (cfg.tls ? " --tls --cert " + cfg.cert_file + " --key " + cfg.key_file : " --no-tls")
    << (cfg.enable_tui ? " --tui" : " --no-tui")
    << "\n\n";

  if (!prompt_bool("Start Guardian now", true)) return false;
  return true;
}

bool validate_config(const Config &cfg, std::string &error) {
  if (cfg.path.empty() || cfg.path.front() != '/') {
    error = "path must start with /";
    return false;
  }
  if (cfg.max_players == 0 || cfg.max_players > 65535) {
    error = "protocol V1 requires 1 <= max_players <= 65535";
    return false;
  }
  if (cfg.max_building_records == 0 || cfg.max_building_records > 1'000'000) {
    error = "max_building_records must be between 1 and 1000000";
    return false;
  }
  if (cfg.service_radius_chunks > 127) {
    error = "protocol V1 local chunk index requires service_radius_chunks <= 127";
    return false;
  }
  if (cfg.player_aoi_chunks % 2 == 0 || cfg.player_aoi_chunks == 0) {
    error = "player_aoi_chunks must be an odd positive number";
    return false;
  }
  if (cfg.player_aoi_radius_chunks != cfg.player_aoi_chunks / 2) {
    error = "player_aoi_radius_chunks must equal player_aoi_chunks / 2";
    return false;
  }
  if (cfg.tls && (cfg.cert_file.empty() || cfg.key_file.empty())) {
    error = "tls requires cert_file and key_file";
    return false;
  }
  if (cfg.enable_ws_compression) {
    error = "permessage-deflate is intentionally disabled for Guardian V0";
    return false;
  }
  if (cfg.tui_refresh_hz == 0 || cfg.tui_refresh_hz > 10) {
    error = "tui_refresh_hz must be between 1 and 10";
    return false;
  }
  if (cfg.tui_log_capacity < 20) {
    error = "tui_log_capacity must be at least 20";
    return false;
  }
  if (cfg.heartbeat_interval_sec == 0 || cfg.heartbeat_interval_sec >= cfg.idle_timeout_sec) {
    error = "heartbeat_interval_sec must be > 0 and less than idle_timeout_sec";
    return false;
  }
  return true;
}

void print_startup_config(const Config &cfg) {
  std::cout << "NiceChunk Guardian starting\n"
            << "guardian_id=" << cfg.guardian_id << "\n"
            << "listen=" << cfg.listen_host << ":" << cfg.listen_port << " path=" << cfg.path << "\n"
            << "public_url=" << cfg.public_url << " tls=" << (cfg.tls ? "true" : "false") << "\n"
            << "heartbeat_interval_sec=" << cfg.heartbeat_interval_sec << " idle_timeout_sec=" << cfg.idle_timeout_sec << "\n"
            << "center_chunk=(" << cfg.guardian_center_chunk_x << "," << cfg.guardian_center_chunk_z
            << ") service_radius=" << cfg.service_radius_chunks << "\n"
            << "aoi_chunks=" << cfg.player_aoi_chunks << " aoi_radius=" << (uint32_t)cfg.player_aoi_radius_chunks << "\n"
            << "max_connections=" << cfg.max_connections << " max_players=" << cfg.max_players
            << " max_backpressure=" << cfg.max_backpressure << "\n";
}

} // namespace nc
