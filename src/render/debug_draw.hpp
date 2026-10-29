#pragma once

#include "animation/skeleton.hpp"
#include "core.hpp"
#include "render/shader_program.hpp"

#include <ranmath/ran.hpp>

namespace mikugl {

class DebugDraw final {
private:
  struct create_t {};

  struct DebugVertex {
    ran::Vec3f32 position;
    ran::Vec4f32 color;
  };

public:
  static ShaderExpect<DebugDraw> create();

public:
  DebugDraw(create_t, u32 vao, u32 vbo, ShaderProgram&& prog) noexcept;
  ~DebugDraw() noexcept;
  NTF_NO_COPY(DebugDraw);
  DebugDraw(DebugDraw&& other) noexcept;
  DebugDraw& operator=(DebugDraw&& other) noexcept;

public:
  DebugDraw& begin(const ran::Mat4f32& view_proj) noexcept;
  DebugDraw& draw_line(const ran::Vec3f32& p0, const ran::Vec3f32& p1,
                       const ran::Vec4f32& color) noexcept;
  DebugDraw& draw_cross(const ran::Vec3f32& pos, f32 size, const ran::Vec4f32& color) noexcept;
  DebugDraw& draw_skeleton(const Skeleton& skeleton, i32 selected_bone = -1) noexcept;
  void render() noexcept;

public:
  bool moved_from() const noexcept { return !_vao || !_vbo || _shader.moved_from(); }

private:
  void _destroy() noexcept;

private:
  ShaderProgram _shader;
  u32 _vao;
  u32 _vbo;
  ran::Mat4f32 _view_proj;
  Vec<DebugVertex> _vertices;
};

} // namespace mikugl
