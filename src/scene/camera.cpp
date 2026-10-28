#include "scene/camera.hpp"

#include <algorithm>
#include <cmath>

namespace mikugl {

namespace {

constexpr f32 s_orbit_sensitivity = 0.005f;
constexpr f32 s_pan_sensitivity = 0.0015f;
constexpr f32 s_zoom_sensitivity = 0.08f;
constexpr f32 s_min_distance = 0.5f;
constexpr f32 s_max_distance = 2000.0f;
constexpr f32 s_max_pitch = 1.55f; // ~88.8 degrees
constexpr f32 s_min_pitch = -1.55f;

constexpr ran::Vec3f32 s_default_target{0.0f, 5.0f, 0.0f};

} // namespace

MmdCamera::MmdCamera() noexcept :
    _target(s_default_target), _distance(default_distance), _yaw(0.0f), _pitch(ran::rad(-5.0f)),
    _roll(0.0f), _fov(ran::rad(default_fov)), _near_z(0.5f), _far_z(2000.0f) {}

MmdCamera::MmdCamera(const ran::Vec3f32& target, f32 distance, f32 fov_deg) noexcept :
    _target(target), _distance(ran::clamp(distance, s_min_distance, s_max_distance)), _yaw(0.0f),
    _pitch(ran::rad(-5.0f)), _roll(0.0f), _fov(ran::rad(fov_deg)), _near_z(0.5f), _far_z(2000.0f) {
}

void MmdCamera::orbit(f32 dx, f32 dy) noexcept {
  _yaw += dx * s_orbit_sensitivity;
  _pitch += dy * s_orbit_sensitivity;
  _pitch = ran::clamp(_pitch, s_min_pitch, s_max_pitch);
}

void MmdCamera::pan(f32 delta_x, f32 delta_y) noexcept {
  const f32 factor = _distance * s_pan_sensitivity;

  const f32 sin_y = ran::sin(_yaw);
  const f32 cos_y = ran::cos(_yaw);

  const ran::Vec3f32 right{cos_y, 0.0f, -sin_y};
  const ran::Vec3f32 up{0.0f, 1.0f, 0.0f};

  _target = _target - right * (delta_x * factor) + up * (delta_y * factor);
}

void MmdCamera::zoom(f32 delta) noexcept {
  _distance -= delta * (_distance * s_zoom_sensitivity);
  _distance = ran::clamp(_distance, s_min_distance, s_max_distance);
}

void MmdCamera::reset() noexcept {
  _target = s_default_target;
  _distance = default_distance;
  _yaw = 0.0f;
  _pitch = ran::rad(-5.0f);
  _roll = 0.0f;
  _fov = ran::rad(default_fov);
}

void MmdCamera::set_distance(f32 distance) noexcept {
  _distance = ran::clamp(distance, s_min_distance, s_max_distance);
}

void MmdCamera::set_pitch(f32 pitch) noexcept {
  _pitch = ran::clamp(pitch, s_min_pitch, s_max_pitch);
}

ran::Vec3f32 MmdCamera::compute_position() const noexcept {
  const f32 cos_pitch = ran::cos(_pitch);
  const f32 sin_pitch = ran::sin(_pitch);
  const f32 cos_yaw = ran::cos(_yaw);
  const f32 sin_yaw = ran::sin(_yaw);

  const ran::Vec3f32 dir{
    sin_yaw * cos_pitch,
    -sin_pitch,
    cos_yaw * cos_pitch,
  };

  return _target + dir * _distance;
}

ran::Mat4f32 MmdCamera::compute_view_matrix() const noexcept {
  const ran::Vec3f32 eye = compute_position();
  const ran::Vec3f32 up{0.0f, 1.0f, 0.0f};
  return ran::lookat_rh(eye, _target, up);
}

ran::Mat4f32 MmdCamera::compute_projection_matrix(f32 aspect_ratio) const noexcept {
  return ran::perspective(_fov, aspect_ratio, _near_z, _far_z);
}

} // namespace mikugl
