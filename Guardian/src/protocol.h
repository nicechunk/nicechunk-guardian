#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace nc {

constexpr uint8_t kProtocolVersion = 1;
constexpr uint16_t kServerFlagEquipmentSync = 1u << 0;
constexpr uint16_t kServerFlagDigBatch = 1u << 1;
constexpr uint16_t kServerFlagBuildingManifest = 1u << 2;

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
  MSG_DIG_BATCH = 0x22,
  MSG_DIG_EVENT_BATCH = 0x23,
  MSG_PLAYER_JOIN = 0x30,
  MSG_PLAYER_LEAVE = 0x31,
  MSG_CHAT = 0x40,
  MSG_CHAT_EVENT = 0x41,
  MSG_EQUIPMENT = 0x42,
  MSG_EQUIPMENT_EVENT = 0x43,
  MSG_PLAYER_IDENTITY = 0x44,
  MSG_PLAYER_IDENTITY_EVENT = 0x45,
  MSG_BUILDING_REGION_DIGEST = 0x50,
  MSG_BUILDING_MANIFEST_REQUEST = 0x51,
  MSG_BUILDING_MANIFEST_PAGE = 0x52,
  MSG_BUILDING_ANNOUNCE = 0x53,
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

enum EquipmentKind : uint8_t {
  EQUIPMENT_EMPTY = 0,
  EQUIPMENT_PICKAXE = 1,
  EQUIPMENT_BLOCK = 2,
  EQUIPMENT_FORGED = 3,
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
  std::array<uint8_t, 32> owner_wallet{};
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
  std::array<uint8_t, 32> owner_wallet{};
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

struct Equipment {
  uint16_t seq = 0;
  uint8_t right_hand_kind = 0;
  uint8_t right_hand_variant = 0;
  uint8_t flags = 0;
  uint32_t design_hash = 0;
  std::string_view payload;
};

struct EquipmentEvent {
  uint16_t local_player_id = 0;
  uint16_t seq = 0;
  uint8_t right_hand_kind = 0;
  uint8_t right_hand_variant = 0;
  uint8_t flags = 0;
  uint32_t design_hash = 0;
  std::string_view payload;
};

struct PlayerIdentity {
  uint16_t local_player_id = 0;
  std::array<uint8_t, 32> owner_wallet{};
  std::string_view display_name;
};

struct BuildingRecord {
  uint64_t foundation_id = 0;
  int32_t min_x = 0;
  int32_t min_z = 0;
  int16_t surface_y = 0;
  uint16_t flags = 1;
  uint32_t width = 0;
  uint32_t depth = 0;
  uint32_t active_revision = 0;
  std::array<uint8_t, 16> content_hash{};
  uint64_t updated_slot = 0;
};

struct BuildingRegionDigest {
  int32_t region_x = 0;
  int32_t region_z = 0;
  uint64_t revision = 0;
  uint32_t record_count = 0;
  std::array<uint8_t, 16> hash{};
};

struct BuildingManifestRequest {
  uint64_t known_revision = 0;
};

constexpr size_t kHelloSize = 52;
constexpr size_t kHelloAckSize = 20;
constexpr size_t kMoveSize = 13;
constexpr size_t kMoveBatchHeaderSize = 4;
constexpr size_t kMoveItemSize = 12;
constexpr size_t kDigSize = 11;
constexpr size_t kDigEventSize = 14;
constexpr size_t kDigBatchHeaderSize = 2;
constexpr size_t kDigBatchItemSize = 10;
constexpr size_t kDigEventBatchHeaderSize = 6;
constexpr size_t kDigEventBatchItemSize = 9;
constexpr size_t kLegacyPlayerJoinSize = 13;
constexpr size_t kPlayerJoinV1Size = 17;
constexpr size_t kPlayerJoinV2Size = 25;
constexpr size_t kPlayerJoinSize = 57;
constexpr size_t kLegacyPlayerLeaveSize = 4;
constexpr size_t kPlayerLeaveV1Size = 8;
constexpr size_t kPlayerLeaveV2Size = 16;
constexpr size_t kPlayerLeaveSize = 48;
constexpr size_t kChatHeaderSize = 4;
constexpr size_t kChatEventHeaderSize = 6;
constexpr size_t kMaxChatBytes = 120;
constexpr size_t kEquipmentHeaderSize = 12;
constexpr size_t kEquipmentEventHeaderSize = 14;
constexpr size_t kMaxEquipmentPayloadBytes = 2048;
constexpr size_t kPlayerIdentityHeaderSize = 2;
constexpr size_t kPlayerIdentityEventHeaderSize = 36;
constexpr size_t kMaxPlayerIdentityNameBytes = 64;
constexpr size_t kLegacyBuildingRecordSize = 48;
constexpr size_t kBuildingRecordSize = 56;
constexpr size_t kBuildingRegionDigestSize = 38;
constexpr size_t kBuildingManifestRequestSize = 9;
constexpr size_t kBuildingManifestPageHeaderSize = 44;
constexpr size_t kBuildingAnnounceSize = 1 + kBuildingRecordSize;
constexpr size_t kLegacyBuildingAnnounceSize = 1 + kLegacyBuildingRecordSize;
constexpr size_t kBuildingManifestPageRecords = 128;

bool decode_hello(std::string_view bytes, Hello &out);
bool decode_move(std::string_view bytes, Move &out);
bool decode_dig(std::string_view bytes, Dig &out);
bool decode_dig_batch(std::string_view bytes, std::vector<Dig> &out);
bool decode_pong(std::string_view bytes);
bool decode_chat(std::string_view bytes, Chat &out);
bool decode_equipment(std::string_view bytes, Equipment &out);
bool decode_player_identity(std::string_view bytes, PlayerIdentity &out);
bool decode_building_manifest_request(std::string_view bytes, BuildingManifestRequest &out);
bool decode_building_announce(std::string_view bytes, BuildingRecord &out);

std::string encode_hello(const Hello &msg);
std::string encode_hello_ack(const HelloAck &msg);
std::string encode_error(ErrorCode code);
std::string encode_ping();
std::string encode_pong();
std::string encode_move(const Move &msg);
std::string encode_move_batch(uint16_t server_tick, const MoveItem *items, size_t count);
std::string encode_dig(const Dig &msg);
std::string encode_dig_event(const DigEvent &msg);
std::string encode_dig_batch(const Dig *items, size_t count);
std::string encode_dig_event_batch(uint16_t local_player_id, uint16_t server_tick, const DigEvent *items, size_t count);
std::string encode_player_join(const PlayerJoin &msg);
std::string encode_player_leave(const PlayerLeave &msg);
std::string encode_chat_event(const ChatEvent &msg);
std::string encode_equipment_event(const EquipmentEvent &msg);
std::string encode_player_identity(const PlayerIdentity &msg);
std::string encode_building_region_digest(const BuildingRegionDigest &msg);
std::string encode_building_manifest_page(
  const BuildingRegionDigest &digest,
  uint16_t page_index,
  uint16_t page_count,
  const BuildingRecord *records,
  size_t count);
std::string encode_building_announce(const BuildingRecord &record);
void append_building_record(std::string &out, const BuildingRecord &record);
bool decode_building_record(std::string_view bytes, BuildingRecord &out);

bool valid_type(uint8_t type);

} // namespace nc
