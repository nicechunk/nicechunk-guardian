#include "app.h"
#include "config.h"

#include <iostream>
#include <unistd.h>

int main(int argc, char **argv) {
  nc::Config cfg;
  if (argc == 1) {
    if (!isatty(STDIN_FILENO)) {
      nc::print_startup_guide(argv[0]);
      return 0;
    }
    if (!nc::run_startup_wizard(cfg, argv[0])) return 0;
  } else {
    if (!nc::apply_cli_args(cfg, argc, argv)) return 2;
  }

  std::string error;
  if (!nc::validate_config(cfg, error)) {
    std::cerr << "Invalid Guardian config: " << error << "\n";
    return 2;
  }

  if (!cfg.enable_tui) nc::print_startup_config(cfg);
  return nc::run_guardian_app(cfg);
}
