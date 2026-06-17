#pragma once

#include <string_view>

namespace nc {

void log_info(std::string_view msg);
void log_error(std::string_view msg);

} // namespace nc
