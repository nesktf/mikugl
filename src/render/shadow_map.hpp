#pragma once

#include "core.hpp"

#include <ranmath/ran.hpp>

namespace mikugl {

class ShadowMap final {
private:
  struct create_t {};

public:
  struct LightMat {
    ran::Mat4f32 view;
    ran::Mat4f32 proj;
  };

public:
  static constexpr u32 default_resolution = 2048;

public:
  static ShadowMap create(u32 resolution = default_resolution);

  static LightMat compute_light_matrices(const ran::Vec3f32& target, const ran::Vec3f32& light_dir,
                                         f32 box_extent = 25.0f) noexcept;

public:
  ShadowMap(create_t, u32 fbo, u32 depth, u32 resolution) noexcept;
  ~ShadowMap() noexcept;
  NTF_NO_COPY(ShadowMap);
  ShadowMap(ShadowMap&& other) noexcept;
  ShadowMap& operator=(ShadowMap&& other) noexcept;

public:
  void bind_fbo() const noexcept;
  void unbind_fbo(i32 viewport_width, i32 viewport_height) const noexcept;
  void bind_texture(u32 slot) const noexcept;

public:
  u32 fbo() const noexcept { return _fbo; }

  u32 depth_texture() const noexcept { return _depth_texture; }

  u32 resolution() const noexcept { return _resolution; }

private:
  void _destroy() noexcept;

private:
  u32 _fbo;
  u32 _depth_texture;
  u32 _resolution;
};

} // namespace mikugl
