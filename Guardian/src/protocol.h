#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace nc {

constexpr uint8_t kProtocolVersion = 1;

enum MsgType : uint8_t {
  MSG_HELLO = 0x01,
  MSG_HELLO_ACK = 0x02,
  MSG_ERROR = 0x03,
  MSG_PING = 0x04,
  MSG_PONG = 0x05,
  MSG_MOVE = 0x10,
  MSG_MOVE_BATCH = 0x11,
  MSG_DIG = 0x20,
  MSG_DIG_EVENT = 0x21,
  MSG_PLAYER_JOIN = 0x30,
  MSG_PLAYER_LEAVE = 0x31,
  MSG_CHAT = 0x40,
  MSG_CHAT_EVENT = 0x41,
};

enum ErrorCode : uint16_t {
  ERROR_BAD_PROTOCOL = 1,
  ERROR_NOT_HELLO = 2,
  ERROR_OUT_OF_RANGE = 3,
  ERROR_BAD_PAYLOAD_LENGTH = 4,
  ERROR_RATE_LIMIT = 5,
  ERROR_BAD_PLAYER_ID = 6,
  ERROR_SERVER_FULL = 7,
  ERROR_UNSUPPORTED_CHUNK_INDEX_MODE = 8,
  ERROR_BAD_DIG_SEQ = 9,
  ERROR_BACKPRESSURE_LIMIT = 10,
};

struct Hello {
  uint8_t protocol_version = kProtocolVersion;
  uint16_t client_flags = 0;
  std::array<uint8_t, 32> wallet_pubkey{};
  int32_t start_chunk_x = 0;
  int32_t start_chunk_z = 0;
  uint64_t client_nonce = 0;
};

struct HelloAck {
  uint8_t protocol_version = kProtocolVersion;
  uint16_t local_player_id = 0;
  uint16_t server_flags = 0;
  int32_t center_chunk_x = 0;
  int32_t center_chunk_z = 0;
  uint16_t service_radius_chunks = 0;
  uint8_t aoi_radius_chunks = 0;
  uint8_t chunk_index_mode = 1;
  uint16_t server_tick = 0;
};

struct Move {
  uint8_t local_chunk_x = 0;
  uint8_t local_chunk_z = 0;
  uint16_t pos_x = 0;
  uint16_t pos_y = 0;
  uint16_t pos_z = 0;
  uint8_t yaw = 0;
  int8_t pitch = 0;
  uint16_t client_tick = 0;
};

struct MoveItem {
  uint16_t local_player_id = 0;
  uint8_t local_chunk_x = 0;
  uint8_t local_chunk_z = 0;
  uint16_t pos_x = 0;
  uint16_t pos_y = 0;
  uint16_t pos_z = 0;
  uint8_t yaw = 0;
  int8_t pitch = 0;
};

struct Dig {
  uint16_t seq = 0;
  uint8_t local_chunk_x = 0;
  uint8_t local_chunk_z = 0;
  uint8_t block_x = 0;
  uint16_t block_y = 0;
  uint8_t block_z = 0;
  uint8_t action = 0;
  uint8_t tool_hint = 0;
};

struct DigEvent {
  uint16_t local_player_id = 0;
  uint16_t seq = 0;
  uint8_t local_chunk_x = 0;
  uint8_t local_chunk_z = 0;
  uint8_t block_x = 0;
  uint16_t block_y = 0;
  uint8_t block_z = 0;
  uint8_t action = 0;
  uint16_t server_tick = 0;
};

struct PlayerJoin {
  uint16_t local_player_id = 0;
  uint32_t owner_hash = 0;
  uint64_t owner_fingerprint = 0;
  uint8_t local_chunk_x = 0;
  uint8_t local_chunk_z = 0;
  uint16_t pos_x = 0;
  uint16_t pos_y = 0;
  uint16_t pos_z = 0;
  uint8_t yaw = 0;
  int8_t pitch = 0;
};

struct PlayerLeave {
  uint16_t local_player_id = 0;
  uint8_t reason = 0;
  uint32_t owner_hash = 0;
  uint64_t owner_fingerprint = 0;
};

struct Chat {
  uint16_t seq = 0;
  std::string_view message;
};

struct ChatEvent {
  uint16_t local_player_id = 0;
  uint16_t seq = 0;
  std::string_view message;
};

constexpr size_t kHelloSize = 52;
constexpr size_t kHelloAckSize = 20;
constexpr size_t kMoveSize = 13;
constexpr size_t kMoveBatchHeaderSize = 4;
constexpr size_t kMoveItemSize = 12;
constexpr size_t kDigSize = 11;
constexpr size_t kDigEventSize = 14;
constexpr size_t kLegacyPlayerJoinSize = 13;
constexpr size_t kPlayerJoinV1Size = 17;
constexpr size_t kPlayerJoinSize = 25;
constexpr size_t kLegacyPlayerLeaveSize = 4;
constexpr size_t kPlayerLeaveV1Size = 8;
constexpr size_t kPlayerLeaveSize = 16;
constexpr size_t kChatHeaderSize = 4;
constexpr size_t kChatEventHeaderSize = 6;
constexpr size_t kMaxChatBytes = 120;

bool decode_hello(std::string_view bytes, Hello &out);
bool decode_move(std::string_view bytes, Move &out);
bool decode_dig(std::string_view bytes, Dig &out);
bool decode_pong(std::string_view bytes);
bool decode_chat(std::string_view bytes, Chat &out);

std::string encode_hello(const Hello &msg);
std::string encode_hello_ack(const HelloAck &msg);
std::string encode_error(ErrorCode code);
std::string encode_ping();
std::string encode_pong();
std::string encode_move(const Move &msg);
std::string encode_move_batch(uint16_t server_tick, const MoveItem *items, size_t count);
std::string encode_dig(const Dig &msg);
std::string encode_dig_event(const DigEvent &msg);
std::string encode_player_join(const PlayerJoin &msg);
std::string encode_player_leave(const PlayerLeave &msg);
std::string encode_chat_event(const ChatEvent &msg);

bool valid_type(uint8_t type);

} // namespace nc
