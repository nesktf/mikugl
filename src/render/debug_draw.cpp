#include "render/debug_draw.hpp"

#include <embedded_shaders.hpp>

#include <glad/glad.h>

#include <utility>

namespace mikugl {

ShaderExpect<DebugDraw> DebugDraw::create() {
  return ShaderProgram::create(shaders::k_debug_line_vert, shaders::k_debug_line_frag)
    .and_then([](ShaderProgram&& prog) -> ShaderExpect<DebugDraw> {
      GLuint vbo = 0;
      GLuint vao = 0;
      glGenVertexArrays(1, &vao);
      glGenBuffers(1, &vbo);
      miku_assert(vbo && vbo); // handle errors later

      glBindVertexArray(vao);
      glBindBuffer(GL_ARRAY_BUFFER, vbo);

      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex),
                            reinterpret_cast<const void*>(offsetof(DebugVertex, position)));

      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(DebugVertex),
                            reinterpret_cast<const void*>(offsetof(DebugVertex, color)));

      glBindVertexArray(0);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      return {ntf::in_place, create_t(), vao, vbo, std::move(prog)};
    });
}

void DebugDraw::_destroy() noexcept {
  if (_vbo) {
    GLuint v = _vbo;
    glDeleteBuffers(1, &v);
    _vbo = 0;
  }
  if (_vao) {
    GLuint v = _vao;
    glDeleteVertexArrays(1, &v);
    _vao = 0;
  }
  _vertices.clear();
  // shader destroys itself
}

DebugDraw::DebugDraw(create_t, u32 vao, u32 vbo, ShaderProgram&& prog) noexcept :
    _shader(std::move(prog)), _vao(vao), _vbo(vbo), _view_proj(ran::Mat4f32::identity()),
    _vertices() {}

DebugDraw::~DebugDraw() noexcept {
  _destroy();
}

DebugDraw::DebugDraw(DebugDraw&& other) noexcept :
    _shader(std::move(other._shader)), _vao(std::exchange(other._vao, 0)),
    _vbo(std::exchange(other._vbo, 0)), _view_proj(other._view_proj),
    _vertices(std::move(other._vertices)) {}

DebugDraw& DebugDraw::operator=(DebugDraw&& other) noexcept {
  if (this != &other) {
    _destroy();
    _shader = std::move(other._shader);
    _vao = std::exchange(other._vao, 0);
    _vbo = std::exchange(other._vbo, 0);
    _view_proj = other._view_proj;
    _vertices = std::move(other._vertices);
  }
  return *this;
}

DebugDraw& DebugDraw::begin(const ran::Mat4f32& view_proj) noexcept {
  _view_proj = view_proj;
  _vertices.clear();
  return *this;
}

DebugDraw& DebugDraw::draw_line(const ran::Vec3f32& p0, const ran::Vec3f32& p1,
                                const ran::Vec4f32& color) noexcept {
  _vertices.emplace_back(p0, color);
  _vertices.emplace_back(p1, color);
  return *this;
}

DebugDraw& DebugDraw::draw_cross(const ran::Vec3f32& pos, f32 size,
                                 const ran::Vec4f32& color) noexcept {
  const f32 hs = size * 0.5f;
  draw_line(ran::Vec3f32{pos.x - hs, pos.y, pos.z}, ran::Vec3f32{pos.x + hs, pos.y, pos.z}, color);
  draw_line(ran::Vec3f32{pos.x, pos.y - hs, pos.z}, ran::Vec3f32{pos.x, pos.y + hs, pos.z}, color);
  draw_line(ran::Vec3f32{pos.x, pos.y, pos.z - hs}, ran::Vec3f32{pos.x, pos.y, pos.z + hs}, color);
  return *this;
}

DebugDraw& DebugDraw::draw_skeleton(const Skeleton& skeleton, i32 selected_bone) noexcept {
  const auto bones = skeleton.bones();
  for (size_t i = 0; i < bones.size(); ++i) {
    const auto& b = bones[i];
    const bool is_selected = (static_cast<i32>(i) == selected_bone);
    const auto joint_color =
      is_selected ? ran::Vec4f32{1.0f, 0.85f, 0.2f, 0.95f} : ran::Vec4f32{0.2f, 0.85f, 1.0f, 0.8f};
    const f32 cross_size = is_selected ? 0.25f : 0.12f;
    draw_cross(b.world_position, cross_size, joint_color);

    if (b.parent_index >= 0 && (size_t)b.parent_index < bones.size()) {
      const auto& parent = bones[b.parent_index];
      const auto line_color = is_selected ? ran::Vec4f32{1.0f, 0.9f, 0.3f, 0.9f}
                                          : ran::Vec4f32{0.3f, 0.75f, 1.0f, 0.55f};
      draw_line(parent.world_position, b.world_position, line_color);
    }
  }
  return *this;
}

void DebugDraw::render() noexcept {
  if (_vertices.empty() || moved_from()) {
    return;
  }

  glBindBuffer(GL_ARRAY_BUFFER, (GLuint)_vbo);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(_vertices.size() * sizeof(DebugVertex)),
               _vertices.data(), GL_DYNAMIC_DRAW);

  _shader.bind();
  _shader.set_uniform_mat4("u_view_proj", _view_proj);

  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glLineWidth(2.0f);

  glBindVertexArray((GLuint)_vao);
  glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(_vertices.size()));
  glBindVertexArray(0);

  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  _vertices.clear();
}

} // namespace mikugl
