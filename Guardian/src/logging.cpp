#include "logging.h"

#include <iostream>

namespace nc {

void log_info(std::string_view msg) {
  std::cout << msg << std::endl;
}

void log_error(std::string_view msg) {
  std::cerr << msg << std::endl;
}

} // namespace nc
