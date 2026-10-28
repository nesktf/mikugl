#include "render/grid.hpp"

#include <glad/glad.h>

#include <embedded_shaders.hpp>
#include <utility>

namespace mikugl {

GridExpect<Grid> Grid::create() {
  return ShaderProgram::create(shaders::k_grid_vert, shaders::k_grid_frag)
    .transform_error([&](ShaderError err) -> GridError {
      MIKU_UNUSED(err);
      return GridError::ShaderCreationFailed;
    })
    .and_then([&](ShaderProgram&& shader_res) -> GridExpect<Grid> {
      GLuint vao;
      glGenVertexArrays(1, &vao);
      if (!vao) {
        return Unexpected<GridError>(GridError::VaoCreationFailed);
      }
      return {ntf::in_place, create_t(), (u32)vao, std::move(shader_res)};
    });
}

Grid::Grid(create_t, u32 vao, ShaderProgram&& shader) noexcept :
    _vao(vao), _shader(std::move(shader)) {}

Grid::~Grid() noexcept {
  if (_vao) {
    glDeleteVertexArrays(1, &_vao);
    _vao = 0;
  }
}

Grid::Grid(Grid&& other) noexcept : _vao(other._vao), _shader(std::move(other._shader)) {
  other._vao = 0;
}

Grid& Grid::operator=(Grid&& other) noexcept {
  if (this != &other) {
    if (_vao != 0) {
      glDeleteVertexArrays(1, &_vao);
    }
    _vao = other._vao;
    _shader = std::move(other._shader);
    other._vao = 0;
  }
  return *this;
}

void Grid::render(const ran::Mat4f32& view, const ran::Mat4f32& proj) const noexcept {
  if (!_vao || !_shader.moved_from()) {
    return;
  }

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  _shader.bind();
  _shader.set_uniform_mat4("u_view", view);
  _shader.set_uniform_mat4("u_proj", proj);

  glBindVertexArray(_vao);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);

  ShaderProgram::unbind();
}

} // namespace mikugl
