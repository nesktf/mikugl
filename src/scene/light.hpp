#pragma once

#include "core.hpp"

#include <ranmath/ran.hpp>

#include <string>

namespace mikugl {

class ShaderProgram;

class LightManager {
public:
  static constexpr u32 MAX_POINT_LIGHTS = 16;

public:
  struct DirectionalLight {
    ran::Vec3f32 direction{.5f, 1.f, .7f};
    ran::Vec3f32 color{1.f, 1.f, 1.f};
    bool enabled = true;
  };

  struct PointLight {
    std::string name = "Point Light";
    ran::Vec3f32 position{0.f, 12.f, 6.f};
    ran::Vec3f32 color{1.f, .92f, .8f};
    f32 radius = 25.f;
    f32 intensity = 1.5f;
    bool enabled = true;
  };

public:
  LightManager() noexcept;

public:
  u32 add_point_light(const PointLight& light);
  u32 add_default_point_light();
  void remove_point_light(u32 index);
  void clear_point_lights() noexcept;
  void upload_to_shader(const ShaderProgram& shader) const noexcept;

public:
  DirectionalLight& directional_light() noexcept { return _dir_light; }

  const DirectionalLight& directional_light() const noexcept { return _dir_light; }

  Span<PointLight> point_lights() noexcept { return _point_lights; }

  Span<const PointLight> point_lights() const noexcept { return _point_lights; }

  u32 point_light_count() const noexcept { return static_cast<u32>(_point_lights.size()); }

private:
  DirectionalLight _dir_light;
  Vec<PointLight> _point_lights;
  u32 _next_light_id;
};

} // namespace mikugl
