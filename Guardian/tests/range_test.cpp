#include "chunk.h"
#include "config.h"

static int check(bool condition) {
  return condition ? 0 : 1;
}

int main() {
  nc::Config cfg;
  cfg.guardian_center_chunk_x = 0;
  cfg.guardian_center_chunk_z = 0;
  cfg.service_radius_chunks = 100;

  if (check(nc::contains_chunk(cfg, 0, 0))) return 1;
  if (check(nc::contains_chunk(cfg, 100, 100))) return 1;
  if (check(nc::contains_chunk(cfg, -100, -100))) return 1;
  if (check(!nc::contains_chunk(cfg, 101, 0))) return 1;
  if (check(!nc::contains_chunk(cfg, 0, 101))) return 1;
  if (check(!nc::contains_chunk(cfg, -101, 0))) return 1;
  return 0;
}
