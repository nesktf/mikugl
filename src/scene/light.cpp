#include "scene/light.hpp"

#include "render/shader_program.hpp"

#include <fmt/format.h>

namespace mikugl {

LightManager::LightManager() noexcept : _dir_light(), _point_lights(), _next_light_id(1) {}

u32 LightManager::add_point_light(const PointLight& light) {
  if (_point_lights.size() >= MAX_POINT_LIGHTS) {
    return static_cast<u32>(_point_lights.size() - 1);
  }
  _point_lights.push_back(light);
  return static_cast<u32>(_point_lights.size() - 1);
}

u32 LightManager::add_default_point_light() {
  if (_point_lights.size() >= MAX_POINT_LIGHTS) {
    return static_cast<u32>(_point_lights.size() - 1);
  }
  PointLight pl{};
  pl.name = fmt::format("Point Light {}", _next_light_id++);
  const f32 offset_x = (static_cast<f32>(_point_lights.size() % 3) - 1.0f) * 5.0f;
  pl.position = ran::Vec3f32{offset_x, 12.0f, 6.0f};
  _point_lights.push_back(std::move(pl));
  return static_cast<u32>(_point_lights.size() - 1);
}

void LightManager::remove_point_light(u32 index) {
  if (index < _point_lights.size()) {
    _point_lights.erase(_point_lights.begin() + index);
  }
}

void LightManager::clear_point_lights() noexcept {
  _point_lights.clear();
}

void LightManager::upload_to_shader(const ShaderProgram& shader) const noexcept {
  shader.set_uniform_vec3("u_light_dir", _dir_light.direction);
  const auto active_dir_color =
    _dir_light.enabled ? _dir_light.color : ran::Vec3f32{0.0f, 0.0f, 0.0f};
  shader.set_uniform_vec3("u_light_color", active_dir_color);

  i32 active_count = 0;
  for (size_t i = 0; i < _point_lights.size() && active_count < static_cast<i32>(MAX_POINT_LIGHTS);
       ++i) {
    const auto& pl = _point_lights[i];
    if (!pl.enabled || pl.intensity <= 0.0f || pl.radius <= 0.0f) {
      continue;
    }
    const auto pos_str = fmt::format("u_point_light_pos[{}]", active_count);
    const auto color_str = fmt::format("u_point_light_color[{}]", active_count);
    const auto param_str = fmt::format("u_point_light_params[{}]", active_count);
    shader.set_uniform_vec3(pos_str, pl.position);
    shader.set_uniform_vec3(color_str, pl.color);
    shader.set_uniform_vec2(param_str, {pl.radius, pl.intensity});
    ++active_count;
  }
  shader.set_uniform_int("u_point_light_count", active_count);
}

} // namespace mikugl
