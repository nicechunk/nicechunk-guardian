#include "guardian.h"

#include <cstdio>
#include <fstream>
#include <iterator>
#include <unistd.h>

namespace {

nc::BuildingRecord record(uint64_t id, int32_t x, int32_t z, uint32_t width, uint32_t depth) {
  nc::BuildingRecord value;
  value.foundation_id = id;
  value.min_x = x;
  value.min_z = z;
  value.surface_y = 97;
  value.flags = 1;
  value.width = width;
  value.depth = depth;
  value.updated_slot = id * 10;
  return value;
}

} // namespace

int main() {
  nc::Config cfg;
  cfg.guardian_center_chunk_x = 0;
  cfg.guardian_center_chunk_z = 0;
  cfg.building_manifest_file.clear();
  nc::GuardianState state(cfg);

  const auto first = record(1, 0, 0, 16, 16);
  if (!state.upsert_building(first)) return 1;
  if (state.building_digest().record_count != 1) return 1;
  const std::array<uint8_t, 16> expected_first_hash = {
    0x1a, 0x80, 0xd4, 0x2d, 0x4b, 0xe9, 0xfb, 0x0d,
    0xb7, 0xbb, 0x6f, 0x93, 0x6e, 0x52, 0x6d, 0x15,
  };
  if (state.building_digest().hash != expected_first_hash) return 1;

  const auto overlapping = record(2, 8, 8, 16, 16);
  if (state.upsert_building(overlapping)) return 1;

  auto resized = first;
  resized.width = 24;
  resized.updated_slot = 11;
  if (!state.upsert_building(resized)) return 1;

  auto changed = resized;
  changed.active_revision = 1;
  changed.content_hash[0] = 7;
  changed.updated_slot = 12;
  if (!state.upsert_building(changed)) return 1;

  auto same_revision = changed;
  same_revision.content_hash[1] = 9;
  if (state.upsert_building(same_revision)) return 1;

  auto removed = changed;
  removed.flags = 0;
  removed.updated_slot = 13;
  if (!state.upsert_building(removed)) return 1;
  if (state.building_digest().record_count != 0) return 1;

  const auto outside = record(3, 1600, 0, 16, 16);
  if (state.upsert_building(outside)) return 1;

  nc::Config source_cfg;
  source_cfg.guardian_center_chunk_x = 0;
  source_cfg.guardian_center_chunk_z = 0;
  source_cfg.building_manifest_file.clear();
  nc::GuardianState source(source_cfg);
  if (!source.upsert_building(first)) return 1;
  std::string legacy_v2 = source.building_manifest_binary();
  legacy_v2.replace(0, 8, "NCKBRG02");
  legacy_v2[8] = 2;
  legacy_v2[9] = 0;
  const std::string migration_path = "/tmp/nicechunk-guardian-manifest-" + std::to_string((long long)getpid()) + ".bin";
  {
    std::ofstream output(migration_path, std::ios::binary | std::ios::trunc);
    output.write(legacy_v2.data(), (std::streamsize)legacy_v2.size());
    if (!output) return 1;
  }
  nc::Config migrated_cfg = source_cfg;
  migrated_cfg.building_manifest_file = migration_path;
  nc::GuardianState migrated(migrated_cfg);
  if (migrated.building_digest().revision != source.building_digest().revision + 1
      || migrated.building_digest().hash != source.building_digest().hash) {
    std::remove(migration_path.c_str());
    return 1;
  }
  std::ifstream migrated_input(migration_path, std::ios::binary);
  std::string migrated_bytes((std::istreambuf_iterator<char>(migrated_input)), std::istreambuf_iterator<char>());
  std::remove(migration_path.c_str());
  if (migrated_bytes.size() < 10 || migrated_bytes.compare(0, 8, "NCKBRG03") != 0
      || (uint8_t)migrated_bytes[8] != 3 || (uint8_t)migrated_bytes[9] != 0) return 1;
  return 0;
}
