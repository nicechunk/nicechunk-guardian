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

  if (nc::decode_move(dig_bytes, decoded_move)) return 1;
  std::string bad_magic = move_bytes;
  bad_magic[0] = (char)0xfe;
  if (nc::decode_move(bad_magic, decoded_move)) return 1;
  if (nc::valid_type(0xfe)) return 1;
  return 0;
}
