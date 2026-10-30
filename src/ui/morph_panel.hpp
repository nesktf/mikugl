#pragma once

#include "animation/morph.hpp"

namespace mikugl {

class MorphPanel final {
public:
  static void draw(MorphManager& morph_manager, bool* p_open = nullptr);
};

} // namespace mikugl
