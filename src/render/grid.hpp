#pragma once

#include "core.hpp"
#include "render/shader_program.hpp"

#include <ranmath/ran.hpp>

namespace mikugl {

enum class GridError : u8 {
  ShaderCreationFailed,
  VaoCreationFailed,
};

template<typename T>
using GridExpect = Expected<T, GridError>;

class Grid final {
private:
  struct create_t {};

public:
  static Expected<Grid, GridError> create();

public:
  Grid(create_t, u32 vao, ShaderProgram&& shader) noexcept;
  ~Grid() noexcept;

  Grid(Grid&& other) noexcept;
  Grid& operator=(Grid&& other) noexcept;
  NTF_NO_COPY(Grid);

public:
  void render(const ran::Mat4f32& view, const ran::Mat4f32& proj) const noexcept;

private:
  u32 _vao;
  ShaderProgram _shader;
};

} // namespace mikugl
