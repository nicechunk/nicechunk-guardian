#include "app.h"

#include "guardian.h"
#include "protocol.h"
#include "nc_time.h"

#include <App.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <deque>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <termios.h>
#include <thread>
#include <unordered_set>
#include <unistd.h>

namespace nc {
namespace {

constexpr std::string_view kHeartbeatTopic = "__nicechunk_guardian_heartbeat";

class TuiInput {
public:
  explicit TuiInput(bool enabled) : enabled_(enabled && isatty(STDIN_FILENO)) {
    if (!enabled_) return;
    tcgetattr(STDIN_FILENO, &old_termios_);
    old_flags_ = fcntl(STDIN_FILENO, F_GETFL, 0);

    termios raw = old_termios_;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    fcntl(STDIN_FILENO, F_SETFL, old_flags_ | O_NONBLOCK);

    running_.store(true);
    thread_ = std::thread([this]() { read_loop(); });
  }

  ~TuiInput() {
    if (!enabled_) return;
    running_.store(false);
    if (thread_.joinable()) thread_.join();
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios_);
    fcntl(STDIN_FILENO, F_SETFL, old_flags_);
  }

  std::vector<TuiCommand> drain() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TuiCommand> commands(commands_.begin(), commands_.end());
    commands_.clear();
    return commands;
  }

private:
  void push(TuiCommand command) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (commands_.size() < 64) commands_.push_back(command);
  }

  void read_loop() {
    while (running_.load(std::memory_order_relaxed)) {
      unsigned char ch = 0;
      ssize_t n = read(STDIN_FILENO, &ch, 1);
      if (n == 1) {
        if (ch == '\t') push(TuiCommand::NextSection);
        else if (ch == 'j' || ch == 'J') push(TuiCommand::RowDown);
        else if (ch == 'k' || ch == 'K') push(TuiCommand::RowUp);
        else if (ch >= '1' && ch <= '5') push((TuiCommand)((int)TuiCommand::Section1 + (ch - '1')));
        else if (ch == 27) {
          unsigned char seq[2] = {0, 0};
          if (read(STDIN_FILENO, seq, 2) == 2 && seq[0] == '[') {
            if (seq[1] == 'A') push(TuiCommand::RowUp);
            else if (seq[1] == 'B') push(TuiCommand::RowDown);
          }
        }
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
      }
    }
  }

  bool enabled_ = false;
  std::atomic<bool> running_{false};
  std::thread thread_;
  std::mutex mutex_;
  std::deque<TuiCommand> commands_;
  termios old_termios_{};
  int old_flags_ = 0;
};

bool use_tui(const Config &cfg) {
  return cfg.enable_tui && isatty(STDOUT_FILENO);
}

bool wallet_is_empty(const std::array<uint8_t, 32> &wallet) {
  return std::all_of(wallet.begin(), wallet.end(), [](uint8_t value) { return value == 0; });
}

void restore_terminal_at_exit() {
  std::cout << "\033[?25h\033[0m" << std::endl;
}

struct PerSocketData {
  Player *player = nullptr;
  uint64_t opened_ms = 0;
};

template <bool SSL>
using Ws = uWS::WebSocket<SSL, true, PerSocketData>;

template <bool SSL>
struct SocketRegistry {
  static std::unordered_set<Ws<SSL> *> sockets;
};

template <bool SSL>
std::unordered_set<Ws<SSL> *> SocketRegistry<SSL>::sockets;

template <bool SSL>
void send_binary(Ws<SSL> *ws, std::string_view payload) {
  ws->send(payload, uWS::OpCode::BINARY, false);
}

template <bool SSL>
void send_error(Ws<SSL> *ws, ErrorCode code) {
  auto payload = encode_error(code);
  send_binary(ws, payload);
}

template <bool SSL>
void close_with_error(Ws<SSL> *ws, ErrorCode code) {
  send_error(ws, code);
  ws->end(1008);
}

template <bool SSL>
void handle_hello(Ws<SSL> *ws, GuardianState &state, std::string_view message, const GuardianState::PublishFn &publish) {
  Hello hello;
  if (!decode_hello(message, hello)) {
    state.add_log("Rejected HELLO: bad protocol");
    close_with_error(ws, ERROR_BAD_PROTOCOL);
    return;
  }
  if (ws->getUserData()->player) {
    close_with_error(ws, ERROR_BAD_PROTOCOL);
    return;
  }
  if (wallet_is_empty(hello.wallet_pubkey)) {
    state.add_log("Rejected HELLO: missing wallet identity");
    close_with_error(ws, ERROR_BAD_PROTOCOL);
    return;
  }
  auto retired = state.retire_players_with_wallet(hello.wallet_pubkey);
  for (const auto &notice : retired) {
    auto leave_payload = encode_player_leave(notice.leave);
    state.publish_to_aoi(notice.chunk_x, notice.chunk_z, leave_payload, publish);
    state.add_log("Retired duplicate player #" + std::to_string(notice.leave.local_player_id));
  }
  if (!state.can_accept_player()) {
    state.add_log("Rejected HELLO: server full");
    close_with_error(ws, ERROR_SERVER_FULL);
    return;
  }
  Player *player = state.create_player(hello, now_ms());
  if (!player) {
    state.add_log("Rejected HELLO: out of Guardian range");
    close_with_error(ws, ERROR_OUT_OF_RANGE);
    return;
  }
  ws->getUserData()->player = player;
  ws->subscribe(player->current_chunk_room->topic);

  HelloAck ack;
  ack.local_player_id = player->local_player_id;
  ack.center_chunk_x = state.config().guardian_center_chunk_x;
  ack.center_chunk_z = state.config().guardian_center_chunk_z;
  ack.service_radius_chunks = state.config().service_radius_chunks;
  ack.aoi_radius_chunks = state.config().player_aoi_radius_chunks;
  ack.chunk_index_mode = 1;
  send_binary(ws, encode_hello_ack(ack));

  std::vector<PlayerJoin> snapshot;
  state.collect_snapshot(player, snapshot);
  for (const auto &join : snapshot) {
    send_binary(ws, encode_player_join(join));
  }

  state.add_log("Player #" + std::to_string(player->local_player_id) + " connected in chunk (" +
                std::to_string(player->current_chunk_x) + "," + std::to_string(player->current_chunk_z) + ")");
}

template <bool SSL>
void handle_move(Ws<SSL> *ws, GuardianState &state, std::string_view message) {
  Player *player = ws->getUserData()->player;
  if (!player || !player->connected || !state.is_active_player(player)) {
    close_with_error(ws, ERROR_NOT_HELLO);
    return;
  }
  Move move;
  if (!decode_move(message, move)) {
    close_with_error(ws, ERROR_BAD_PAYLOAD_LENGTH);
    return;
  }
  uint64_t t = now_ms();
  if (!rate_limit_allow(t, player->move_window_ms, player->move_count, state.config().client_move_rate_limit_per_sec)) {
    send_error(ws, ERROR_RATE_LIMIT);
    return;
  }
  int32_t new_chunk_x = 0;
  int32_t new_chunk_z = 0;
  if (!state.local_to_global(move.local_chunk_x, move.local_chunk_z, new_chunk_x, new_chunk_z)) {
    state.add_log("Player #" + std::to_string(player->local_player_id) + " moved out of Guardian range");
    close_with_error(ws, ERROR_OUT_OF_RANGE);
    return;
  }

  bool changed_chunk = new_chunk_x != player->current_chunk_x || new_chunk_z != player->current_chunk_z;
  bool first_pose = !player->has_pose;

  if (changed_chunk) {
    int32_t old_x = player->current_chunk_x;
    int32_t old_z = player->current_chunk_z;
    if (player->has_pose) {
      auto leave_payload = encode_player_leave(state.make_leave(*player, 1));
      state.publish_to_aoi(old_x, old_z, leave_payload,
        [&](std::string_view topic, std::string_view payload) {
          ws->publish(topic, payload, uWS::OpCode::BINARY, false);
        });
    }

    if (player->current_chunk_room) ws->unsubscribe(player->current_chunk_room->topic);
    if (!state.move_player_chunk(player, new_chunk_x, new_chunk_z)) {
      state.add_log("Player #" + std::to_string(player->local_player_id) + " failed chunk transfer");
      close_with_error(ws, ERROR_OUT_OF_RANGE);
      return;
    }
    ws->subscribe(player->current_chunk_room->topic);
  }

  state.update_player_move(player, move, t);

  if (changed_chunk || first_pose) {
    auto join_payload = encode_player_join(state.make_join(*player));
    state.publish_to_aoi(new_chunk_x, new_chunk_z, join_payload,
      [&](std::string_view topic, std::string_view payload) {
        ws->publish(topic, payload, uWS::OpCode::BINARY, false);
      });
    state.add_log("Player #" + std::to_string(player->local_player_id) + " entered chunk (" +
                  std::to_string(new_chunk_x) + "," + std::to_string(new_chunk_z) + ")");
  }

  state.metrics().move_in += 1;
  state.metrics().bytes_in += message.size();
}

template <bool SSL>
void handle_dig(Ws<SSL> *ws, GuardianState &state, std::string_view message) {
  Player *player = ws->getUserData()->player;
  if (!player || !player->connected || !state.is_active_player(player)) {
    close_with_error(ws, ERROR_NOT_HELLO);
    return;
  }
  Dig dig;
  if (!decode_dig(message, dig)) {
    close_with_error(ws, ERROR_BAD_PAYLOAD_LENGTH);
    return;
  }
  uint64_t t = now_ms();
  if (!rate_limit_allow(t, player->dig_window_ms, player->dig_count, state.config().client_dig_rate_limit_per_sec)) {
    send_error(ws, ERROR_RATE_LIMIT);
    return;
  }
  if (!state.accept_dig_seq(player, dig.seq)) {
    send_error(ws, ERROR_BAD_DIG_SEQ);
    return;
  }
  int32_t dig_chunk_x = 0;
  int32_t dig_chunk_z = 0;
  if (!state.local_to_global(dig.local_chunk_x, dig.local_chunk_z, dig_chunk_x, dig_chunk_z)) {
    state.add_log("Rejected DIG: target out of Guardian range");
    close_with_error(ws, ERROR_OUT_OF_RANGE);
    return;
  }
  int32_t dx = dig_chunk_x - player->current_chunk_x;
  int32_t dz = dig_chunk_z - player->current_chunk_z;
  if (dx < -2 || dx > 2 || dz < -2 || dz > 2) {
    state.add_log("Rejected DIG: target too far from player");
    send_error(ws, ERROR_OUT_OF_RANGE);
    return;
  }
  auto payload = encode_dig_event(state.make_dig_event(*player, dig));
  state.publish_to_aoi(dig_chunk_x, dig_chunk_z, payload,
    [&](std::string_view topic, std::string_view bytes) {
      ws->publish(topic, bytes, uWS::OpCode::BINARY, false);
    });
  state.metrics().dig_in += 1;
  state.metrics().bytes_in += message.size();
}

template <bool SSL>
void handle_chat(Ws<SSL> *ws, GuardianState &state, std::string_view message) {
  Player *player = ws->getUserData()->player;
  if (!player || !player->connected || !state.is_active_player(player)) {
    close_with_error(ws, ERROR_NOT_HELLO);
    return;
  }
  Chat chat;
  if (!decode_chat(message, chat)) {
    close_with_error(ws, ERROR_BAD_PAYLOAD_LENGTH);
    return;
  }
  ChatEvent event{
    player->local_player_id,
    chat.seq,
    chat.message,
  };
  auto payload = encode_chat_event(event);
  state.publish_to_aoi(player->current_chunk_x, player->current_chunk_z, payload,
    [&](std::string_view topic, std::string_view bytes) {
      ws->publish(topic, bytes, uWS::OpCode::BINARY, false);
    });
  state.metrics().bytes_in += message.size();
}

template <bool SSL>
void reap_stale_sockets(GuardianState &state) {
  const uint64_t t = now_ms();
  const uint64_t idle_timeout_ms = (uint64_t)state.config().idle_timeout_sec * 1000ull;
  const uint64_t hello_timeout_ms = (uint64_t)state.config().hello_timeout_sec * 1000ull;
  std::vector<Ws<SSL> *> stale;
  stale.reserve(SocketRegistry<SSL>::sockets.size() / 16 + 1);

  for (Ws<SSL> *ws : SocketRegistry<SSL>::sockets) {
    auto *data = ws->getUserData();
    Player *player = data->player;
    if (player && state.is_active_player(player)) {
      if (t > player->last_seen_ms && t - player->last_seen_ms > idle_timeout_ms) {
        stale.push_back(ws);
      }
      continue;
    }
    if (!player && data->opened_ms && t > data->opened_ms && t - data->opened_ms > hello_timeout_ms) {
      stale.push_back(ws);
    }
  }

  for (Ws<SSL> *ws : stale) {
    ws->end(1001);
  }
}

template <bool SSL, typename AppT>
void install_routes(AppT &app, GuardianState &state) {
  const Config &cfg = state.config();
  app.template ws<PerSocketData>(cfg.path, {
    .compression = uWS::DISABLED,
    .maxPayloadLength = cfg.max_payload_length,
    .idleTimeout = cfg.idle_timeout_sec,
    .maxBackpressure = cfg.max_backpressure,
    .closeOnBackpressureLimit = cfg.close_on_backpressure_limit,
    .resetIdleTimeoutOnSend = false,
    .sendPingsAutomatically = true,
    .open = [&state](Ws<SSL> *ws) {
      ws->getUserData()->opened_ms = now_ms();
      SocketRegistry<SSL>::sockets.insert(ws);
      ws->subscribe(kHeartbeatTopic);
      state.metrics().connections += 1;
    },
    .message = [&app, &state](Ws<SSL> *ws, std::string_view message, uWS::OpCode opCode) {
      if (opCode != uWS::OpCode::BINARY) {
        if (!state.config().enable_text_frame) {
          close_with_error(ws, ERROR_BAD_PROTOCOL);
        }
        return;
      }
      if (message.empty()) {
        close_with_error(ws, ERROR_BAD_PAYLOAD_LENGTH);
        return;
      }
      uint8_t type = (uint8_t)message[0];
      if (!valid_type(type)) {
        close_with_error(ws, ERROR_BAD_PROTOCOL);
        return;
      }
      if (Player *player = ws->getUserData()->player; player && state.is_active_player(player)) {
        player->last_seen_ms = now_ms();
      }
      switch (type) {
        case MSG_HELLO:
          handle_hello(ws, state, message, [&](std::string_view topic, std::string_view payload) {
            app.publish(topic, payload, uWS::OpCode::BINARY, false);
          });
          break;
        case MSG_MOVE:
          handle_move(ws, state, message);
          break;
        case MSG_DIG:
          handle_dig(ws, state, message);
          break;
        case MSG_CHAT:
          handle_chat(ws, state, message);
          break;
        case MSG_PONG:
          if (!decode_pong(message)) close_with_error(ws, ERROR_BAD_PAYLOAD_LENGTH);
          break;
        default:
          close_with_error(ws, ERROR_BAD_PROTOCOL);
          break;
      }
    },
    .drain = [&state](Ws<SSL> *ws) {
      if (ws->getBufferedAmount() > state.config().max_backpressure / 2) {
        state.metrics().backpressure_connections += 1;
      }
    },
    .close = [&app, &state](Ws<SSL> *ws, int, std::string_view) {
      SocketRegistry<SSL>::sockets.erase(ws);
      Player *player = ws->getUserData()->player;
      if (player) {
        if (state.is_active_player(player)) {
          auto leave_payload = encode_player_leave(state.make_leave(*player, 2));
          int32_t old_x = player->current_chunk_x;
          int32_t old_z = player->current_chunk_z;
          state.publish_to_aoi(old_x, old_z, leave_payload,
            [&](std::string_view topic, std::string_view payload) {
              app.publish(topic, payload, uWS::OpCode::BINARY, false);
            });
          player->connected = false;
          state.remove_player(player);
        } else {
          state.release_retired_player(player);
        }
        ws->getUserData()->player = nullptr;
      }
      if (state.metrics().connections > 0) state.metrics().connections -= 1;
    }
  });
}

template <bool SSL, typename AppT>
int run_app_instance(AppT &app, GuardianState &state) {
  std::atomic<bool> running{true};
  auto *loop = uWS::Loop::get();
  const uint64_t move_interval_ms = 1000ull / state.config().movement_broadcast_hz;
  const uint64_t stats_interval_ms = (uint64_t)state.config().stats_interval_sec * 1000ull;
  const bool tui_enabled = use_tui(state.config());
  const uint64_t tui_interval_ms = tui_enabled ? 1000ull / state.config().tui_refresh_hz : 0;
  TuiInput tui_input(tui_enabled);
  if (tui_enabled) {
    std::atexit(restore_terminal_at_exit);
    std::cout << "\033[?25l\033[2J\033[H" << std::flush;
  }

  std::thread ticker([&]() {
    uint64_t last_move = now_ms();
    uint64_t last_heartbeat = now_ms();
    uint64_t last_reap = now_ms();
    uint64_t last_stats = now_ms();
    uint64_t last_tui = now_ms();
    const uint64_t heartbeat_interval_ms = (uint64_t)state.config().heartbeat_interval_sec * 1000ull;
    while (running.load(std::memory_order_relaxed)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      uint64_t t = now_ms();
      if (t - last_move >= move_interval_ms) {
        last_move = t;
        loop->defer([&app, &state]() {
          state.flush_pending_moves([&](std::string_view topic, std::string_view payload) {
            app.publish(topic, payload, uWS::OpCode::BINARY, false);
          });
        });
      }
      if (t - last_heartbeat >= heartbeat_interval_ms) {
        last_heartbeat = t;
        loop->defer([&app]() {
          app.publish(kHeartbeatTopic, encode_ping(), uWS::OpCode::BINARY, false);
        });
      }
      if (t - last_reap >= 1000) {
        last_reap = t;
        loop->defer([&state]() { reap_stale_sockets<SSL>(state); });
      }
      if (tui_enabled && t - last_tui >= tui_interval_ms) {
        last_tui = t;
        auto commands = tui_input.drain();
        loop->defer([&state, commands = std::move(commands)]() {
          for (TuiCommand command : commands) state.apply_tui_command(command);
          std::cout << state.render_tui(now_ms()) << std::flush;
        });
      } else if (!tui_enabled && t - last_stats >= stats_interval_ms) {
        last_stats = t;
        loop->defer([&state]() { state.print_stats(now_ms()); });
      }
    }
  });

  bool listening = false;
  app.listen(state.config().listen_host, state.config().listen_port, [&](auto *token) {
    listening = token != nullptr;
    if (listening) {
      state.add_log("Listening on " + state.config().listen_host + ":" + std::to_string(state.config().listen_port) + state.config().path);
      if (!tui_enabled) {
        std::cout << "listening on " << state.config().listen_host << ":" << state.config().listen_port << state.config().path << "\n";
      }
    } else {
      state.add_log("Failed to listen on " + state.config().listen_host + ":" + std::to_string(state.config().listen_port));
      if (!tui_enabled) {
        std::cerr << "failed to listen on " << state.config().listen_host << ":" << state.config().listen_port << "\n";
      }
    }
  });
  if (listening) app.run();
  running.store(false);
  ticker.join();
  if (tui_enabled) std::cout << "\033[?25h\033[0m" << std::endl;
  return listening ? 0 : 1;
}

} // namespace

int run_guardian_app(const Config &cfg) {
  GuardianState state(cfg);
  state.add_log("Guardian boot: " + cfg.guardian_id);
  state.add_log("Public endpoint: " + cfg.public_url);
  state.add_log("TUI: " + std::string(use_tui(cfg) ? "enabled" : "disabled"));
  if (cfg.tls) {
    uWS::SSLApp app({
      .key_file_name = cfg.key_file.c_str(),
      .cert_file_name = cfg.cert_file.c_str(),
    });
	    install_routes<true>(app, state);
	    return run_app_instance<true>(app, state);
  }

  uWS::App app;
  install_routes<false>(app, state);
  return run_app_instance<false>(app, state);
}

} // namespace nc
