#include "guardian.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <sys/ioctl.h>
#include <unistd.h>

namespace nc {
namespace {

constexpr const char *kReset = "\033[0m";
constexpr const char *kCyan = "\033[38;5;51m";
constexpr const char *kMagenta = "\033[38;5;201m";
constexpr const char *kGreen = "\033[38;5;118m";
constexpr const char *kYellow = "\033[38;5;226m";
constexpr const char *kDim = "\033[38;5;244m";
constexpr const char *kPanel = "\033[38;5;45m";
constexpr const char *kHot = "\033[48;5;53m\033[38;5;231m";
constexpr const char *kFrame = "\033[38;5;33m";
constexpr const char *kGlow = "\033[38;5;87m";

std::string at(int row, int col) {
  return "\033[" + std::to_string(row) + ";" + std::to_string(col) + "H";
}

std::string clear_line() {
  return "\033[2K";
}

std::string repeat_utf8(std::string_view value, int count) {
  std::string out;
  if (count <= 0) return out;
  out.reserve(value.size() * (size_t)count);
  for (int i = 0; i < count; ++i) out.append(value);
  return out;
}

std::string clip(std::string value, int width) {
  if (width <= 0) return "";
  if ((int)value.size() <= width) return value;
  if (width <= 3) return value.substr(0, width);
  return value.substr(0, width - 3) + "...";
}

std::string bytes_per_sec(uint64_t bytes) {
  std::ostringstream out;
  if (bytes >= 1024ull * 1024ull) out << std::fixed << std::setprecision(2) << (double)bytes / (1024.0 * 1024.0) << " MB/s";
  else if (bytes >= 1024ull) out << std::fixed << std::setprecision(1) << (double)bytes / 1024.0 << " KB/s";
  else out << bytes << " B/s";
  return out.str();
}

std::pair<int, int> terminal_size() {
  winsize ws {};
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
    return {ws.ws_col, ws.ws_row};
  }
  return {120, 38};
}

std::string stat_line(std::string_view label, std::string_view value, int width) {
  std::string left(label);
  std::string right(value);
  int gap = width - (int)left.size() - (int)right.size();
  if (gap < 1) gap = 1;
  return left + std::string((size_t)gap, ' ') + right;
}

std::string panel_top(int width, std::string_view title = "") {
  int inner = std::max(2, width - 2);
  if (title.empty()) return std::string("╭") + repeat_utf8("─", inner) + "╮";
  std::string label = " " + std::string(title) + " ";
  int left = 2;
  int right = std::max(0, inner - left - (int)label.size());
  return std::string("╭") + repeat_utf8("─", left) + label + repeat_utf8("─", right) + "╮";
}

std::string panel_bottom(int width) {
  return std::string("╰") + repeat_utf8("─", std::max(2, width - 2)) + "╯";
}

std::string panel_rule(int width) {
  return std::string("├") + repeat_utf8("─", std::max(2, width - 2)) + "┤";
}

int max_logo_width(const char **logo, int count) {
  int width = 0;
  for (int i = 0; i < count; ++i) {
    width = std::max(width, (int)std::string_view(logo[i]).size());
  }
  return width;
}

} // namespace

std::string GuardianState::render_tui(uint64_t t_ms) {
  StatsRates rates = update_rates(t_ms);
  auto [term_w, term_h] = terminal_size();
  term_w = std::max(term_w, 110);
  term_h = std::max(term_h, 28);

  const int left_w = std::min(58, std::max(48, (term_w * 42) / 100));
  const int right_x = left_w + 4;
  const int right_w = term_w - right_x - 1;
  const int content_top = 8;

  std::ostringstream out;
  out << "\033[?25l\033[H";

  const char *logo[] = {
    " _   _ _          _____ _                 _    ",
    "| \\ | (_)        / ____| |               | |   ",
    "|  \\| |_  ___ ___| |    | |__  _   _ _ __ | | __",
    "| . ` | |/ __/ _ \\ |    | '_ \\| | | | '_ \\| |/ /",
    "| |\\  | | (_|  __/ |____| | | | |_| | | | |   < ",
    "|_| \\_|_|\\___\\___|\\_____|_| |_|\\__,_|_| |_|_|\\_\\"
  };
  const int logo_w = max_logo_width(logo, 6);
  const int header_x = std::max(right_x, logo_w + 5);
  const int header_w = std::max(20, term_w - header_x - 1);

  for (int i = 0; i < 6; ++i) {
    out << at(i + 1, 2) << kCyan << logo[i] << kReset;
  }
  out << at(2, header_x) << kMagenta << "GUARDIAN REALTIME RELAY" << kReset;
  out << at(3, header_x) << kDim << clip("binary WS / AOI topics / non-authoritative", header_w) << kReset;
  out << at(4, header_x) << kGreen << clip(cfg_.guardian_id, header_w / 3) << kReset
      << "  " << clip(cfg_.listen_host + ":" + std::to_string(cfg_.listen_port) + cfg_.path, header_w / 2)
      << "  " << (cfg_.tls ? "TLS" : "plain WS");
  out << at(5, header_x) << clip("Region center=(" + std::to_string(cfg_.guardian_center_chunk_x) + "," +
                                 std::to_string(cfg_.guardian_center_chunk_z) + ") radius=" +
                                 std::to_string(cfg_.service_radius_chunks) + " chunks  AOI=" +
                                 std::to_string(cfg_.player_aoi_chunks) + "x" +
                                 std::to_string(cfg_.player_aoi_chunks), header_w);
  out << at(6, header_x) << kYellow << "Public URL: " << kReset << clip(cfg_.public_url, header_w - 12);

  for (int row = content_top; row <= term_h; ++row) {
    out << at(row, 1) << clear_line();
  }

  const char *sections[] = {"Overview", "Online Players", "Resource Mining", "Item Creation", "Chunk Rooms"};
  out << at(content_top, 2) << kFrame << panel_top(left_w, " COMMAND DECK ") << kReset;
  out << at(content_top + 1, 2) << kFrame << "│ " << kReset << kGlow << "▌" << kReset
      << kYellow << " NiceChunk Guardian" << kReset
      << std::string(std::max(0, left_w - 26), ' ') << kFrame << " │" << kReset;
  out << at(content_top + 2, 2) << kFrame << panel_rule(left_w) << kReset;
  for (int i = 0; i < 5; ++i) {
    bool selected = tui_section_ == i;
    std::string label = std::string(selected ? "▶ " : "  ") + std::to_string(i + 1) + "  " + sections[i];
    std::string visible = clip(label, left_w - 5);
    out << at(content_top + 3 + i, 2) << kFrame << "│" << kReset
        << (selected ? kGlow : kDim) << "▌" << kReset << " "
        << (selected ? kHot : "")
        << visible
        << (selected ? kReset : "")
        << std::string(std::max(0, left_w - 4 - (int)visible.size()), ' ')
        << kFrame << "│" << kReset;
  }
  out << at(content_top + 8, 2) << kFrame << panel_bottom(left_w) << kReset;
  out << at(content_top + 10, 2) << kDim << "Keys: 1-5 select  Tab next  j/k rows" << kReset;

  int detail = content_top + 12;
  int detail_w = left_w - 4;
  out << at(detail, 2) << kMagenta << "DATA PANEL" << kReset;
  detail += 2;

  auto write_detail = [&](std::string text) {
    if (detail < term_h - 1) out << at(detail++, 2) << kGlow << "▸ " << kReset << clip(std::move(text), detail_w - 2);
  };

  if (tui_section_ == 0) {
    write_detail("Register URL: " + cfg_.public_url);
    write_detail(stat_line("Connections", std::to_string(metrics_.connections), detail_w));
    write_detail(stat_line("Players", std::to_string(metrics_.players), detail_w));
    write_detail(stat_line("Active chunks", std::to_string(metrics_.active_chunks), detail_w));
    write_detail(stat_line("CPU", std::to_string((int)rates.cpu_percent) + "%", detail_w));
    write_detail(stat_line("RSS", std::to_string(rates.memory_usage_mb) + " MB", detail_w));
    write_detail(stat_line("Inbound", bytes_per_sec(rates.bytes_in_per_sec), detail_w));
    write_detail(stat_line("Outbound", bytes_per_sec(rates.bytes_out_per_sec), detail_w));
    write_detail(stat_line("Moves/s", std::to_string(rates.move_in_per_sec), detail_w));
    write_detail(stat_line("Digs/s", std::to_string(rates.dig_in_per_sec), detail_w));
    write_detail(stat_line("Backpressure", std::to_string(metrics_.backpressure_connections), detail_w));
  } else if (tui_section_ == 1) {
    uint32_t idx = 0;
    for (const auto &entry : players_) {
      if (idx++ < tui_row_) continue;
      const Player &p = *entry.second;
      std::ostringstream line;
      line << "#" << p.local_player_id << " c(" << p.current_chunk_x << "," << p.current_chunk_z << ")"
           << " p(" << p.pos_x << "," << p.pos_y << "," << p.pos_z << ")";
      write_detail(line.str());
      if (detail >= term_h - 2) break;
    }
    if (players_.empty()) write_detail("No online players.");
  } else if (tui_section_ == 2) {
    write_detail(stat_line("DIG events total", std::to_string(metrics_.dig_in), detail_w));
    write_detail(stat_line("DIG events/s", std::to_string(rates.dig_in_per_sec), detail_w));
    write_detail("Guardian only relays mining events.");
    write_detail("Final resources settle on Solana.");
  } else if (tui_section_ == 3) {
    write_detail("Item creation relay is reserved.");
    write_detail("Current protocol does not mint items.");
    write_detail("Final item ownership stays on Solana.");
  } else {
    std::vector<ChunkRoom *> rooms;
    rooms.reserve(rooms_.size());
    for (const auto &entry : rooms_) rooms.push_back(entry.second.get());
    std::sort(rooms.begin(), rooms.end(), [](const ChunkRoom *a, const ChunkRoom *b) {
      return a->players.size() > b->players.size();
    });
    if (!rooms.empty() && tui_row_ >= rooms.size()) tui_row_ = (uint32_t)rooms.size() - 1;
    for (size_t i = 0; i < rooms.size(); ++i) {
      if (i < tui_row_) continue;
      ChunkRoom *room = rooms[i];
      std::ostringstream line;
      line << (i == tui_row_ ? "> " : "  ")
           << "chunk(" << room->chunk_x << "," << room->chunk_z << ") players=" << room->players.size()
           << " topic=" << room->topic;
      write_detail(line.str());
      if (i == tui_row_) {
        write_detail("  local_index=" + std::to_string(room->local_chunk_index));
      }
      if (detail >= term_h - 2) break;
    }
    if (rooms.empty()) write_detail("No active chunk rooms.");
  }

  out << at(content_top, right_x) << kFrame << panel_top(right_w, " EVENT LOG ") << kReset;
  out << at(content_top + 1, right_x) << kFrame << "│ " << kReset << kGlow << "▌" << kReset
      << kYellow << " bounded relay telemetry" << kReset
      << std::string(std::max(0, right_w - 29), ' ') << kFrame << " │" << kReset;
  out << at(content_top + 2, right_x) << kFrame << panel_rule(right_w) << kReset;

  int log_row = content_top + 3;
  int log_h = term_h - log_row - 1;
  int start = std::max(0, (int)tui_logs_.size() - log_h);
  for (int i = start; i < (int)tui_logs_.size() && log_row < term_h; ++i) {
    std::string line = clip(tui_logs_[(size_t)i].message, right_w - 4);
    out << at(log_row++, right_x) << kFrame << "│ " << kReset
        << line
        << std::string(std::max(0, right_w - 4 - (int)line.size()), ' ')
        << kFrame << " │" << kReset;
  }
  while (log_row < term_h) {
    out << at(log_row++, right_x) << kFrame << "│ " << std::string(right_w - 4, ' ') << " │" << kReset;
  }
  out << at(term_h, right_x) << kFrame << panel_bottom(right_w) << kReset;

  out << at(term_h, 2) << kDim << "NiceChunk Guardian is a communication relay, not final settlement." << kReset;
  return out.str();
}

} // namespace nc
