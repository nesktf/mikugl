#pragma once

#include "model/model_data.hpp"

#include <string_view>

namespace mikugl {

class ModelLoader final {
public:
  static ModelExpect<ModelData> load_file(std::string_view path);
};

} // namespace mikugl
