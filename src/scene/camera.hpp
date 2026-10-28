#pragma once

#include "core.hpp"

#include <ranmath/ran.hpp>

namespace mikugl {

class MmdCamera final {
public:
  static constexpr f32 default_distance = 20.0f;
  static constexpr f32 default_fov = 90.0f;

public:
  MmdCamera() noexcept;
  explicit MmdCamera(const ran::Vec3f32& target, f32 distance = default_distance,
                     f32 fov_deg = default_fov) noexcept;

public:
  void orbit(f32 dx, f32 dy) noexcept;
  void pan(f32 dx, f32 dy) noexcept;
  void zoom(f32 delta) noexcept;
  void reset() noexcept;
  ran::Mat4f32 compute_view_matrix() const noexcept;
  ran::Mat4f32 compute_projection_matrix(f32 aspect_ratio) const noexcept;
  ran::Vec3f32 compute_position() const noexcept;

public:
  const ran::Vec3f32& target() const noexcept { return _target; }

  void set_target(const ran::Vec3f32& target) noexcept { _target = target; }

  f32 distance() const noexcept { return _distance; }

  void set_distance(f32 distance) noexcept;

  f32 yaw() const noexcept { return _yaw; }

  void set_yaw(f32 yaw) noexcept { _yaw = yaw; }

  f32 pitch() const noexcept { return _pitch; }

  void set_pitch(f32 pitch) noexcept;

  f32 fov() const noexcept { return _fov; }

  void set_fov(f32 fov_rad) noexcept { _fov = fov_rad; }

  f32 near_z() const noexcept { return _near_z; }

  f32 far_z() const noexcept { return _far_z; }

private:
  ran::Vec3f32 _target;
  f32 _distance;
  f32 _yaw;
  f32 _pitch;
  f32 _roll;
  f32 _fov;
  f32 _near_z;
  f32 _far_z;
};

} // namespace mikugl
