#include "protocol.h"

#include <algorithm>
#include <cstring>

namespace nc {
namespace {

uint16_t rd_u16(const uint8_t *p) {
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

uint32_t rd_u32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

uint64_t rd_u64(const uint8_t *p) {
  uint64_t v = 0;
  for (int i = 7; i >= 0; --i) v = (v << 8) | p[i];
  return v;
}

int16_t rd_i16(const uint8_t *p) {
  return (int16_t)rd_u16(p);
}

void wr_u16(std::string &out, uint16_t v) {
  out.push_back((char)(v & 0xff));
  out.push_back((char)((v >> 8) & 0xff));
}

void wr_u32(std::string &out, uint32_t v) {
  out.push_back((char)(v & 0xff));
  out.push_back((char)((v >> 8) & 0xff));
  out.push_back((char)((v >> 16) & 0xff));
  out.push_back((char)((v >> 24) & 0xff));
}

void wr_u64(std::string &out, uint64_t v) {
  for (int i = 0; i < 8; ++i) out.push_back((char)((v >> (i * 8)) & 0xff));
}

void wr_i16(std::string &out, int16_t v) {
  wr_u16(out, (uint16_t)v);
}

void wr_i32(std::string &out, int32_t v) {
  wr_u32(out, (uint32_t)v);
}

} // namespace

bool valid_type(uint8_t type) {
  switch (type) {
    case MSG_HELLO:
    case MSG_HELLO_ACK:
    case MSG_ERROR:
    case MSG_PING:
    case MSG_PONG:
    case MSG_MOVE:
    case MSG_MOVE_BATCH:
    case MSG_DIG:
    case MSG_DIG_EVENT:
    case MSG_DIG_BATCH:
    case MSG_DIG_EVENT_BATCH:
    case MSG_PLAYER_JOIN:
    case MSG_PLAYER_LEAVE:
    case MSG_CHAT:
    case MSG_CHAT_EVENT:
    case MSG_EQUIPMENT:
    case MSG_EQUIPMENT_EVENT:
    case MSG_PLAYER_IDENTITY:
    case MSG_PLAYER_IDENTITY_EVENT:
    case MSG_BUILDING_REGION_DIGEST:
    case MSG_BUILDING_MANIFEST_REQUEST:
    case MSG_BUILDING_MANIFEST_PAGE:
    case MSG_BUILDING_ANNOUNCE:
      return true;
    default:
      return false;
  }
}

bool decode_hello(std::string_view bytes, Hello &out) {
  if (bytes.size() != kHelloSize || (uint8_t)bytes[0] != MSG_HELLO) return false;
  const auto *p = reinterpret_cast<const uint8_t *>(bytes.data());
  out.protocol_version = p[1];
  out.client_flags = rd_u16(p + 2);
  std::memcpy(out.wallet_pubkey.data(), p + 4, out.wallet_pubkey.size());
  out.start_chunk_x = (int32_t)rd_u32(p + 36);
  out.start_chunk_z = (int32_t)rd_u32(p + 40);
  out.client_nonce = rd_u64(p + 44);
  return out.protocol_version == kProtocolVersion;
}

bool decode_move(std::string_view bytes, Move &out) {
  if (bytes.size() != kMoveSize || (uint8_t)bytes[0] != MSG_MOVE) return false;
  const auto *p = reinterpret_cast<const uint8_t *>(bytes.data());
  out.local_chunk_x = p[1];
  out.local_chunk_z = p[2];
  out.pos_x = rd_u16(p + 3);
  out.pos_y = rd_u16(p + 5);
  out.pos_z = rd_u16(p + 7);
  out.yaw = p[9];
  out.pitch = (int8_t)p[10];
  out.client_tick = rd_u16(p + 11);
  return true;
}

bool decode_dig(std::string_view bytes, Dig &out) {
  if (bytes.size() != kDigSize || (uint8_t)bytes[0] != MSG_DIG) return false;
  const auto *p = reinterpret_cast<const uint8_t *>(bytes.data());
  out.seq = rd_u16(p + 1);
  out.local_chunk_x = p[3];
  out.local_chunk_z = p[4];
  out.block_x = p[5];
  out.block_y = rd_u16(p + 6);
  out.block_z = p[8];
  out.action = p[9];
  out.tool_hint = p[10];
  return true;
}

bool decode_dig_batch(std::string_view bytes, std::vector<Dig> &out) {
  out.clear();
  if (bytes.size() < kDigBatchHeaderSize || (uint8_t)bytes[0] != MSG_DIG_BATCH) return false;
  const auto *p = reinterpret_cast<const uint8_t *>(bytes.data());
  size_t count = p[1];
  if (count == 0 || bytes.size() != kDigBatchHeaderSize + count * kDigBatchItemSize) return false;
  out.reserve(count);
  size_t offset = kDigBatchHeaderSize;
  for (size_t i = 0; i < count; ++i) {
    Dig item;
    item.seq = rd_u16(p + offset);
    item.local_chunk_x = p[offset + 2];
    item.local_chunk_z = p[offset + 3];
    item.block_x = p[offset + 4];
    item.block_y = rd_u16(p + offset + 5);
    item.block_z = p[offset + 7];
    item.action = p[offset + 8];
    item.tool_hint = p[offset + 9];
    out.push_back(item);
    offset += kDigBatchItemSize;
  }
  return true;
}

bool decode_pong(std::string_view bytes) {
  return bytes.size() == 1 && (uint8_t)bytes[0] == MSG_PONG;
}

bool decode_chat(std::string_view bytes, Chat &out) {
  if (bytes.size() < kChatHeaderSize || (uint8_t)bytes[0] != MSG_CHAT) return false;
  const auto *p = reinterpret_cast<const uint8_t *>(bytes.data());
  size_t message_len = p[3];
  if (message_len == 0 || message_len > kMaxChatBytes || bytes.size() != kChatHeaderSize + message_len) return false;
  out.seq = rd_u16(p + 1);
  out.message = bytes.substr(kChatHeaderSize, message_len);
  return true;
}

bool decode_equipment(std::string_view bytes, Equipment &out) {
  if (bytes.size() < kEquipmentHeaderSize || (uint8_t)bytes[0] != MSG_EQUIPMENT) return false;
  const auto *p = reinterpret_cast<const uint8_t *>(bytes.data());
  size_t payload_len = rd_u16(p + 6);
  if (payload_len > kMaxEquipmentPayloadBytes || bytes.size() != kEquipmentHeaderSize + payload_len) return false;
  out.seq = rd_u16(p + 1);
  out.right_hand_kind = p[3];
  out.right_hand_variant = p[4];
  out.flags = p[5];
  out.design_hash = rd_u32(p + 8);
  out.payload = bytes.substr(kEquipmentHeaderSize, payload_len);
  return true;
}

bool decode_player_identity(std::string_view bytes, PlayerIdentity &out) {
  if (bytes.size() < kPlayerIdentityHeaderSize || (uint8_t)bytes[0] != MSG_PLAYER_IDENTITY) return false;
  const auto *p = reinterpret_cast<const uint8_t *>(bytes.data());
  size_t name_len = p[1];
  if (name_len > kMaxPlayerIdentityNameBytes || bytes.size() != kPlayerIdentityHeaderSize + name_len) return false;
  out.display_name = bytes.substr(kPlayerIdentityHeaderSize, name_len);
  return true;
}

bool decode_building_record(std::string_view bytes, BuildingRecord &out) {
  if (bytes.size() != kBuildingRecordSize && bytes.size() != kLegacyBuildingRecordSize) return false;
  const auto *p = reinterpret_cast<const uint8_t *>(bytes.data());
  out.foundation_id = rd_u64(p);
  out.min_x = (int32_t)rd_u32(p + 8);
  out.min_z = (int32_t)rd_u32(p + 12);
  out.surface_y = rd_i16(p + 16);
  out.flags = rd_u16(p + 18);
  out.width = rd_u32(p + 20);
  out.depth = rd_u32(p + 24);
  out.active_revision = rd_u32(p + 28);
  std::memcpy(out.content_hash.data(), p + 32, out.content_hash.size());
  out.updated_slot = bytes.size() == kBuildingRecordSize ? rd_u64(p + 48) : 0;
  return true;
}

bool decode_building_manifest_request(std::string_view bytes, BuildingManifestRequest &out) {
  if (bytes.size() != kBuildingManifestRequestSize || (uint8_t)bytes[0] != MSG_BUILDING_MANIFEST_REQUEST) return false;
  out.known_revision = rd_u64(reinterpret_cast<const uint8_t *>(bytes.data()) + 1);
  return true;
}

bool decode_building_announce(std::string_view bytes, BuildingRecord &out) {
  return (bytes.size() == kBuildingAnnounceSize || bytes.size() == kLegacyBuildingAnnounceSize)
    && (uint8_t)bytes[0] == MSG_BUILDING_ANNOUNCE
    && decode_building_record(bytes.substr(1), out);
}

std::string encode_hello(const Hello &msg) {
  std::string out;
  out.reserve(kHelloSize);
  out.push_back((char)MSG_HELLO);
  out.push_back((char)msg.protocol_version);
  wr_u16(out, msg.client_flags);
  out.append(reinterpret_cast<const char *>(msg.wallet_pubkey.data()), msg.wallet_pubkey.size());
  wr_i32(out, msg.start_chunk_x);
  wr_i32(out, msg.start_chunk_z);
  for (int i = 0; i < 8; ++i) out.push_back((char)((msg.client_nonce >> (i * 8)) & 0xff));
  return out;
}

std::string encode_hello_ack(const HelloAck &msg) {
  std::string out;
  out.reserve(kHelloAckSize);
  out.push_back((char)MSG_HELLO_ACK);
  out.push_back((char)msg.protocol_version);
  wr_u16(out, msg.local_player_id);
  wr_u16(out, msg.server_flags);
  wr_i32(out, msg.center_chunk_x);
  wr_i32(out, msg.center_chunk_z);
  wr_u16(out, msg.service_radius_chunks);
  out.push_back((char)msg.aoi_radius_chunks);
  out.push_back((char)msg.chunk_index_mode);
  wr_u16(out, msg.server_tick);
  return out;
}

std::string encode_error(ErrorCode code) {
  std::string out;
  out.reserve(3);
  out.push_back((char)MSG_ERROR);
  wr_u16(out, code);
  return out;
}

std::string encode_ping() {
  return std::string(1, (char)MSG_PING);
}

std::string encode_pong() {
  return std::string(1, (char)MSG_PONG);
}

std::string encode_move(const Move &msg) {
  std::string out;
  out.reserve(kMoveSize);
  out.push_back((char)MSG_MOVE);
  out.push_back((char)msg.local_chunk_x);
  out.push_back((char)msg.local_chunk_z);
  wr_u16(out, msg.pos_x);
  wr_u16(out, msg.pos_y);
  wr_u16(out, msg.pos_z);
  out.push_back((char)msg.yaw);
  out.push_back((char)msg.pitch);
  wr_u16(out, msg.client_tick);
  return out;
}

std::string encode_move_batch(uint16_t server_tick, const MoveItem *items, size_t count) {
  if (count > 255) count = 255;
  std::string out;
  out.reserve(kMoveBatchHeaderSize + count * kMoveItemSize);
  out.push_back((char)MSG_MOVE_BATCH);
  wr_u16(out, server_tick);
  out.push_back((char)count);
  for (size_t i = 0; i < count; ++i) {
    wr_u16(out, items[i].local_player_id);
    out.push_back((char)items[i].local_chunk_x);
    out.push_back((char)items[i].local_chunk_z);
    wr_u16(out, items[i].pos_x);
    wr_u16(out, items[i].pos_y);
    wr_u16(out, items[i].pos_z);
    out.push_back((char)items[i].yaw);
    out.push_back((char)items[i].pitch);
  }
  return out;
}

std::string encode_dig(const Dig &msg) {
  std::string out;
  out.reserve(kDigSize);
  out.push_back((char)MSG_DIG);
  wr_u16(out, msg.seq);
  out.push_back((char)msg.local_chunk_x);
  out.push_back((char)msg.local_chunk_z);
  out.push_back((char)msg.block_x);
  wr_u16(out, msg.block_y);
  out.push_back((char)msg.block_z);
  out.push_back((char)msg.action);
  out.push_back((char)msg.tool_hint);
  return out;
}

std::string encode_dig_event(const DigEvent &msg) {
  std::string out;
  out.reserve(kDigEventSize);
  out.push_back((char)MSG_DIG_EVENT);
  wr_u16(out, msg.local_player_id);
  wr_u16(out, msg.seq);
  out.push_back((char)msg.local_chunk_x);
  out.push_back((char)msg.local_chunk_z);
  out.push_back((char)msg.block_x);
  wr_u16(out, msg.block_y);
  out.push_back((char)msg.block_z);
  out.push_back((char)msg.action);
  wr_u16(out, msg.server_tick);
  return out;
}

std::string encode_dig_batch(const Dig *items, size_t count) {
  if (count > 255) count = 255;
  std::string out;
  out.reserve(kDigBatchHeaderSize + count * kDigBatchItemSize);
  out.push_back((char)MSG_DIG_BATCH);
  out.push_back((char)count);
  for (size_t i = 0; i < count; ++i) {
    wr_u16(out, items[i].seq);
    out.push_back((char)items[i].local_chunk_x);
    out.push_back((char)items[i].local_chunk_z);
    out.push_back((char)items[i].block_x);
    wr_u16(out, items[i].block_y);
    out.push_back((char)items[i].block_z);
    out.push_back((char)items[i].action);
    out.push_back((char)items[i].tool_hint);
  }
  return out;
}

std::string encode_dig_event_batch(uint16_t local_player_id, uint16_t server_tick, const DigEvent *items, size_t count) {
  if (count > 255) count = 255;
  std::string out;
  out.reserve(kDigEventBatchHeaderSize + count * kDigEventBatchItemSize);
  out.push_back((char)MSG_DIG_EVENT_BATCH);
  wr_u16(out, local_player_id);
  wr_u16(out, server_tick);
  out.push_back((char)count);
  for (size_t i = 0; i < count; ++i) {
    wr_u16(out, items[i].seq);
    out.push_back((char)items[i].local_chunk_x);
    out.push_back((char)items[i].local_chunk_z);
    out.push_back((char)items[i].block_x);
    wr_u16(out, items[i].block_y);
    out.push_back((char)items[i].block_z);
    out.push_back((char)items[i].action);
  }
  return out;
}

std::string encode_player_join(const PlayerJoin &msg) {
  std::string out;
  out.reserve(kPlayerJoinSize);
  out.push_back((char)MSG_PLAYER_JOIN);
  wr_u16(out, msg.local_player_id);
  wr_u32(out, msg.owner_hash);
  wr_u64(out, msg.owner_fingerprint);
  out.append(reinterpret_cast<const char *>(msg.owner_wallet.data()), msg.owner_wallet.size());
  out.push_back((char)msg.local_chunk_x);
  out.push_back((char)msg.local_chunk_z);
  wr_u16(out, msg.pos_x);
  wr_u16(out, msg.pos_y);
  wr_u16(out, msg.pos_z);
  out.push_back((char)msg.yaw);
  out.push_back((char)msg.pitch);
  return out;
}

std::string encode_player_leave(const PlayerLeave &msg) {
  std::string out;
  out.reserve(kPlayerLeaveSize);
  out.push_back((char)MSG_PLAYER_LEAVE);
  wr_u16(out, msg.local_player_id);
  out.push_back((char)msg.reason);
  wr_u32(out, msg.owner_hash);
  wr_u64(out, msg.owner_fingerprint);
  out.append(reinterpret_cast<const char *>(msg.owner_wallet.data()), msg.owner_wallet.size());
  return out;
}

std::string encode_chat_event(const ChatEvent &msg) {
  size_t message_len = std::min(msg.message.size(), kMaxChatBytes);
  std::string out;
  out.reserve(kChatEventHeaderSize + message_len);
  out.push_back((char)MSG_CHAT_EVENT);
  wr_u16(out, msg.local_player_id);
  wr_u16(out, msg.seq);
  out.push_back((char)message_len);
  out.append(msg.message.data(), message_len);
  return out;
}

std::string encode_equipment_event(const EquipmentEvent &msg) {
  size_t payload_len = std::min(msg.payload.size(), kMaxEquipmentPayloadBytes);
  std::string out;
  out.reserve(kEquipmentEventHeaderSize + payload_len);
  out.push_back((char)MSG_EQUIPMENT_EVENT);
  wr_u16(out, msg.local_player_id);
  wr_u16(out, msg.seq);
  out.push_back((char)msg.right_hand_kind);
  out.push_back((char)msg.right_hand_variant);
  out.push_back((char)msg.flags);
  wr_u16(out, (uint16_t)payload_len);
  wr_u32(out, msg.design_hash);
  out.append(msg.payload.data(), payload_len);
  return out;
}

std::string encode_player_identity(const PlayerIdentity &msg) {
  size_t name_len = std::min(msg.display_name.size(), kMaxPlayerIdentityNameBytes);
  std::string out;
  out.reserve(kPlayerIdentityEventHeaderSize + name_len);
  out.push_back((char)MSG_PLAYER_IDENTITY_EVENT);
  wr_u16(out, msg.local_player_id);
  out.append(reinterpret_cast<const char *>(msg.owner_wallet.data()), msg.owner_wallet.size());
  out.push_back((char)name_len);
  out.append(msg.display_name.data(), name_len);
  return out;
}

void append_building_record(std::string &out, const BuildingRecord &record) {
  wr_u64(out, record.foundation_id);
  wr_i32(out, record.min_x);
  wr_i32(out, record.min_z);
  wr_i16(out, record.surface_y);
  wr_u16(out, record.flags);
  wr_u32(out, record.width);
  wr_u32(out, record.depth);
  wr_u32(out, record.active_revision);
  out.append(reinterpret_cast<const char *>(record.content_hash.data()), record.content_hash.size());
  wr_u64(out, record.updated_slot);
}

std::string encode_building_region_digest(const BuildingRegionDigest &msg) {
  std::string out;
  out.reserve(kBuildingRegionDigestSize);
  out.push_back((char)MSG_BUILDING_REGION_DIGEST);
  out.push_back(3);
  wr_i32(out, msg.region_x);
  wr_i32(out, msg.region_z);
  wr_u64(out, msg.revision);
  wr_u32(out, msg.record_count);
  out.append(reinterpret_cast<const char *>(msg.hash.data()), msg.hash.size());
  return out;
}

std::string encode_building_manifest_page(
  const BuildingRegionDigest &digest,
  uint16_t page_index,
  uint16_t page_count,
  const BuildingRecord *records,
  size_t count) {
  if (count > kBuildingManifestPageRecords) count = kBuildingManifestPageRecords;
  std::string out;
  out.reserve(kBuildingManifestPageHeaderSize + count * kBuildingRecordSize);
  out.push_back((char)MSG_BUILDING_MANIFEST_PAGE);
  out.push_back(3);
  wr_u16(out, page_index);
  wr_u16(out, page_count);
  wr_u16(out, (uint16_t)count);
  wr_u32(out, digest.record_count);
  wr_i32(out, digest.region_x);
  wr_i32(out, digest.region_z);
  wr_u64(out, digest.revision);
  out.append(reinterpret_cast<const char *>(digest.hash.data()), digest.hash.size());
  for (size_t i = 0; i < count; ++i) append_building_record(out, records[i]);
  return out;
}

std::string encode_building_announce(const BuildingRecord &record) {
  std::string out;
  out.reserve(kBuildingAnnounceSize);
  out.push_back((char)MSG_BUILDING_ANNOUNCE);
  append_building_record(out, record);
  return out;
}

} // namespace nc
