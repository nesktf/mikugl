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

DebugDraw& DebugDraw::draw_skeleton(const Skeleton& skeleton, i32 selected_bone, i32 hovered_bone,
                                    const ran::Mat4f32& model_matrix) noexcept {
  const auto transform_point = [&](const ran::Vec3f32& p) -> ran::Vec3f32 {
    const ran::Vec4f32 wp = model_matrix * ran::Vec4f32{p.x, p.y, p.z, 1.0f};
    return ran::Vec3f32{wp.x, wp.y, wp.z};
  };

  const auto bones = skeleton.bones();
  for (size_t i = 0; i < bones.size(); ++i) {
    const auto& b = bones[i];
    if (!b.visible) {
      continue;
    }
    const bool is_selected = (static_cast<i32>(i) == selected_bone);
    const bool is_hovered = (!is_selected && static_cast<i32>(i) == hovered_bone);

    ran::Vec4f32 joint_color{0.2f, 0.85f, 1.0f, 0.8f};
    ran::Vec4f32 line_color{0.3f, 0.75f, 1.0f, 0.55f};
    f32 cross_size = 0.12f;

    if (is_selected) {
      joint_color = ran::Vec4f32{1.0f, 0.85f, 0.2f, 0.98f};
      line_color = ran::Vec4f32{1.0f, 0.9f, 0.3f, 0.95f};
      cross_size = 0.25f;
    } else if (is_hovered) {
      joint_color = ran::Vec4f32{1.0f, 0.52f, 0.18f, 0.95f};
      line_color = ran::Vec4f32{1.0f, 0.60f, 0.25f, 0.85f};
      cross_size = 0.20f;
    }

    const ran::Vec3f32 b_pos = transform_point(b.world_position);
    draw_cross(b_pos, cross_size, joint_color);

    if (b.parent_index >= 0 && (size_t)b.parent_index < bones.size()) {
      const auto& parent = bones[b.parent_index];
      if (parent.visible) {
        const ran::Vec3f32 p_pos = transform_point(parent.world_position);
        draw_line(p_pos, b_pos, line_color);
      }
    }
  }
  return *this;
}

DebugDraw& DebugDraw::draw_point_light(const ran::Vec3f32& pos, const ran::Vec3f32& color,
                                       f32 radius, bool is_selected) noexcept {
  const ran::Vec4f32 marker_color{color.x, color.y, color.z, is_selected ? 1.0f : 0.85f};
  const f32 cross_size = is_selected ? 0.8f : 0.5f;
  draw_cross(pos, cross_size, marker_color);

  if (is_selected && radius > 0.0f) {
    static constexpr i32 segments = 24;
    const ran::Vec4f32 ring_color{color.x, color.y, color.z, 0.35f};
    for (i32 i = 0; i < segments; ++i) {
      const f32 a0 = (static_cast<f32>(i) / static_cast<f32>(segments)) * 6.2831853f;
      const f32 a1 = (static_cast<f32>(i + 1) / static_cast<f32>(segments)) * 6.2831853f;
      const f32 c0 = ran::cos(a0) * radius;
      const f32 s0 = ran::sin(a0) * radius;
      const f32 c1 = ran::cos(a1) * radius;
      const f32 s1 = ran::sin(a1) * radius;
      draw_line(ran::Vec3f32{pos.x + c0, pos.y, pos.z + s0},
                ran::Vec3f32{pos.x + c1, pos.y, pos.z + s1}, ring_color);
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
