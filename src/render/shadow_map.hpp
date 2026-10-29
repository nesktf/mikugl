#pragma once

#include "core.hpp"

#include <ranmath/ran.hpp>

namespace mikugl {

class ShadowMap final {
public:
  struct LightMat {
    ran::Mat4f32 view;
    ran::Mat4f32 proj;
  };

public:
  static constexpr u32 default_resolution = 2048;

public:
  ShadowMap() noexcept;
  ~ShadowMap() noexcept;
  NTF_NO_COPY(ShadowMap);
  ShadowMap(ShadowMap&& other) noexcept;
  ShadowMap& operator=(ShadowMap&& other) noexcept;

public:
  void init(u32 resolution = default_resolution);
  void destroy() noexcept;
  void bind_fbo() const noexcept;
  void unbind_fbo(i32 viewport_width, i32 viewport_height) const noexcept;
  void bind_texture(u32 slot) const noexcept;

  LightMat compute_light_matrices(const ran::Vec3f32& target, const ran::Vec3f32& light_dir,
                                  f32 box_extent = 25.0f) const noexcept;

public:
  u32 fbo() const noexcept { return _fbo; }

  u32 depth_texture() const noexcept { return _depth_texture; }

  u32 resolution() const noexcept { return _resolution; }

private:
  u32 _fbo;
  u32 _depth_texture;
  u32 _resolution;
};

} // namespace mikugl
