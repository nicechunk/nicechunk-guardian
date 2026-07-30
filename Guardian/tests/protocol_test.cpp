#include "protocol.h"

int main() {
  nc::Hello hello;
  hello.client_flags = 7;
  hello.start_chunk_x = -3;
  hello.start_chunk_z = 4;
  hello.client_nonce = 99;
  auto hello_bytes = nc::encode_hello(hello);
  if (hello_bytes.size() != nc::kHelloSize) return 1;
  nc::Hello decoded_hello;
  if (!nc::decode_hello(hello_bytes, decoded_hello)) return 1;
  if (decoded_hello.start_chunk_x != -3) return 1;
  if (decoded_hello.start_chunk_z != 4) return 1;
  if (decoded_hello.client_nonce != 99) return 1;

  nc::HelloAck ack;
  ack.local_player_id = 123;
  ack.center_chunk_x = -1;
  ack.center_chunk_z = 2;
  ack.service_radius_chunks = 100;
  ack.aoi_radius_chunks = 7;
  ack.chunk_index_mode = 1;
  ack.server_tick = 55;
  if (nc::encode_hello_ack(ack).size() != 20) return 1;

  nc::Move move;
  move.local_chunk_x = 100;
  move.local_chunk_z = 101;
  move.pos_x = 123;
  move.pos_y = 64;
  move.pos_z = 456;
  move.yaw = 33;
  move.pitch = -5;
  move.client_tick = 77;
  auto move_bytes = nc::encode_move(move);
  if (move_bytes.size() != 13) return 1;
  nc::Move decoded_move;
  if (!nc::decode_move(move_bytes, decoded_move)) return 1;
  if (decoded_move.pos_z != 456) return 1;
  if (decoded_move.pitch != -5) return 1;

  nc::MoveItem item;
  item.local_player_id = 42;
  item.local_chunk_x = 100;
  item.local_chunk_z = 100;
  item.pos_x = 1;
  item.pos_y = 2;
  item.pos_z = 3;
  auto batch = nc::encode_move_batch(9, &item, 1);
  if (batch.size() != 16) return 1;

  nc::Dig dig;
  dig.seq = 11;
  dig.local_chunk_x = 99;
  dig.local_chunk_z = 98;
  dig.block_x = 1;
  dig.block_y = 44;
  dig.block_z = 2;
  dig.action = 3;
  dig.tool_hint = 4;
  auto dig_bytes = nc::encode_dig(dig);
  if (dig_bytes.size() != 11) return 1;
  nc::Dig decoded_dig;
  if (!nc::decode_dig(dig_bytes, decoded_dig)) return 1;
  if (decoded_dig.seq != 11) return 1;
  if (decoded_dig.block_y != 44) return 1;

  nc::Dig dig_items[2] = {dig, dig};
  dig_items[1].seq = 12;
  dig_items[1].block_x = 3;
  auto dig_batch_bytes = nc::encode_dig_batch(dig_items, 2);
  if (dig_batch_bytes.size() != nc::kDigBatchHeaderSize + 2 * nc::kDigBatchItemSize) return 1;
  std::vector<nc::Dig> decoded_dig_batch;
  if (!nc::decode_dig_batch(dig_batch_bytes, decoded_dig_batch)) return 1;
  if (decoded_dig_batch.size() != 2) return 1;
  if (decoded_dig_batch[1].seq != 12) return 1;
  if (decoded_dig_batch[1].block_x != 3) return 1;

  nc::DigEvent event;
  event.local_player_id = 7;
  event.seq = 11;
  event.local_chunk_x = 99;
  event.local_chunk_z = 98;
  event.block_x = 1;
  event.block_y = 44;
  event.block_z = 2;
  event.action = 3;
  event.server_tick = 15;
  if (nc::encode_dig_event(event).size() != 14) return 1;
  nc::DigEvent event_items[2] = {event, event};
  event_items[1].seq = 12;
  if (nc::encode_dig_event_batch(event.local_player_id, event.server_tick, event_items, 2).size() != nc::kDigEventBatchHeaderSize + 2 * nc::kDigEventBatchItemSize) return 1;

  nc::PlayerJoin join;
  join.local_player_id = 9;
  join.owner_hash = 123456;
  join.owner_fingerprint = 987654321;
  join.local_chunk_x = 100;
  join.local_chunk_z = 100;
  if (nc::encode_player_join(join).size() != nc::kPlayerJoinSize) return 1;

  nc::PlayerLeave leave;
  leave.local_player_id = 9;
  leave.reason = 2;
  leave.owner_hash = 123456;
  leave.owner_fingerprint = 987654321;
  if (nc::encode_player_leave(leave).size() != nc::kPlayerLeaveSize) return 1;

  std::string equipment_bytes;
  equipment_bytes.push_back((char)nc::MSG_EQUIPMENT);
  equipment_bytes.push_back((char)0x34);
  equipment_bytes.push_back((char)0x12);
  equipment_bytes.push_back((char)3);
  equipment_bytes.push_back((char)2);
  equipment_bytes.push_back((char)1);
  equipment_bytes.push_back((char)3);
  equipment_bytes.push_back((char)0);
  equipment_bytes.push_back((char)0xef);
  equipment_bytes.push_back((char)0xcd);
  equipment_bytes.push_back((char)0xab);
  equipment_bytes.push_back((char)0x89);
  equipment_bytes.append("abc", 3);
  nc::Equipment equipment;
  if (!nc::decode_equipment(equipment_bytes, equipment)) return 1;
  if (equipment.seq != 0x1234) return 1;
  if (equipment.right_hand_kind != 3) return 1;
  if (equipment.right_hand_variant != 2) return 1;
  if (equipment.flags != 1) return 1;
  if (equipment.design_hash != 0x89abcdef) return 1;
  if (equipment.payload != "abc") return 1;

  nc::EquipmentEvent equipment_event;
  equipment_event.local_player_id = 9;
  equipment_event.seq = equipment.seq;
  equipment_event.right_hand_kind = equipment.right_hand_kind;
  equipment_event.right_hand_variant = equipment.right_hand_variant;
  equipment_event.flags = equipment.flags;
  equipment_event.design_hash = equipment.design_hash;
  equipment_event.payload = equipment.payload;
  if (nc::encode_equipment_event(equipment_event).size() != nc::kEquipmentEventHeaderSize + 3) return 1;

  std::string identity_bytes;
  identity_bytes.push_back((char)nc::MSG_PLAYER_IDENTITY);
  identity_bytes.push_back((char)5);
  identity_bytes.append("Alice", 5);
  nc::PlayerIdentity identity;
  if (!nc::decode_player_identity(identity_bytes, identity)) return 1;
  if (identity.display_name != "Alice") return 1;
  identity.local_player_id = 9;
  for (size_t i = 0; i < identity.owner_wallet.size(); ++i) identity.owner_wallet[i] = (uint8_t)i;
  if (nc::encode_player_identity(identity).size() != nc::kPlayerIdentityEventHeaderSize + 5) return 1;

  nc::BuildingRecord building;
  building.foundation_id = 77;
  building.min_x = -1600;
  building.min_z = 3200;
  building.surface_y = 97;
  building.width = 32;
  building.depth = 48;
  building.active_revision = 3;
  building.content_hash[0] = 0xaa;
  building.updated_slot = 99;
  auto announcement = nc::encode_building_announce(building);
  if (announcement.size() != nc::kBuildingAnnounceSize) return 1;
  nc::BuildingRecord decoded_building;
  if (!nc::decode_building_announce(announcement, decoded_building)) return 1;
  if (decoded_building.foundation_id != 77 || decoded_building.content_hash[0] != 0xaa
      || decoded_building.updated_slot != 99) return 1;

  nc::BuildingRegionDigest building_digest;
  building_digest.region_x = -1;
  building_digest.region_z = 2;
  building_digest.revision = 9;
  building_digest.record_count = 1;
  building_digest.hash[0] = 0xbb;
  const auto digest_bytes = nc::encode_building_region_digest(building_digest);
  if (digest_bytes.size() != nc::kBuildingRegionDigestSize || (uint8_t)digest_bytes[1] != 3) return 1;
  const auto manifest_page = nc::encode_building_manifest_page(building_digest, 0, 1, &building, 1);
  if (manifest_page.size() != nc::kBuildingManifestPageHeaderSize + nc::kBuildingRecordSize
      || (uint8_t)manifest_page[1] != 3) return 1;

  std::string manifest_request(nc::kBuildingManifestRequestSize, '\0');
  manifest_request[0] = (char)nc::MSG_BUILDING_MANIFEST_REQUEST;
  manifest_request[1] = 9;
  nc::BuildingManifestRequest decoded_request;
  if (!nc::decode_building_manifest_request(manifest_request, decoded_request)) return 1;
  if (decoded_request.known_revision != 9) return 1;

  if (nc::decode_move(dig_bytes, decoded_move)) return 1;
  std::string bad_magic = move_bytes;
  bad_magic[0] = (char)0xfe;
  if (nc::decode_move(bad_magic, decoded_move)) return 1;
  if (nc::valid_type(0xfe)) return 1;
  return 0;
}
