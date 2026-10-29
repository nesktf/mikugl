#pragma once

#include "core.hpp"

#include <string_view>

namespace mikugl {

class LoadingIndicator final {
public:
  static void draw(bool is_loading, std::string_view path);
};

} // namespace mikugl
