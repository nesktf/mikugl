#include "render/shadow_map.hpp"

#include <glad/glad.h>

namespace mikugl {

ShadowMap::ShadowMap() noexcept : _fbo(0), _depth_texture(0), _resolution(0) {}

ShadowMap::~ShadowMap() noexcept {
  destroy();
}

ShadowMap::ShadowMap(ShadowMap&& other) noexcept :
    _fbo(std::exchange(other._fbo, 0)), _depth_texture(std::exchange(other._depth_texture, 0)),
    _resolution(std::exchange(other._resolution, 0)) {}

ShadowMap& ShadowMap::operator=(ShadowMap&& other) noexcept {
  if (this != &other) {
    destroy();
    _fbo = std::exchange(other._fbo, 0);
    _depth_texture = std::exchange(other._depth_texture, 0);
    _resolution = std::exchange(other._resolution, 0);
  }
  return *this;
}

void ShadowMap::destroy() noexcept {
  if (_depth_texture) {
    GLuint tex = _depth_texture;
    glDeleteTextures(1, &tex);
    _depth_texture = 0;
  }
  if (_fbo) {
    GLuint fbo = _fbo;
    glDeleteFramebuffers(1, &fbo);
    _fbo = 0;
  }
  _resolution = 0;
}

void ShadowMap::init(u32 resolution) {
  destroy();

  _resolution = resolution;

  GLuint fbo = 0;
  GLuint depth_texture = 0;
  glGenFramebuffers(1, &fbo);
  glGenTextures(1, &depth_texture);

  glBindTexture(GL_TEXTURE_2D, depth_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, static_cast<GLsizei>(_resolution),
               static_cast<GLsizei>(_resolution), 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

  static constexpr f32 border_color[]{1.0f, 1.0f, 1.0f, 1.0f};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_texture, 0);

  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);

  _fbo = (u32)fbo;
  _depth_texture = (u32)depth_texture;
}

void ShadowMap::bind_fbo() const noexcept {
  if (_fbo) {
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)_fbo);
    glViewport(0, 0, static_cast<GLsizei>(_resolution), static_cast<GLsizei>(_resolution));
    glClear(GL_DEPTH_BUFFER_BIT);
  }
}

void ShadowMap::unbind_fbo(i32 viewport_width, i32 viewport_height) const noexcept {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, viewport_width, viewport_height);
}

void ShadowMap::bind_texture(u32 slot) const noexcept {
  if (_depth_texture) {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, (GLuint)_depth_texture);
  }
}

auto ShadowMap::compute_light_matrices(const ran::Vec3f32& target, const ran::Vec3f32& light_dir,
                                       f32 box_extent) const noexcept -> LightMat {
  static constexpr f32 light_distance = 50.0f;
  const auto norm_dir = ran::normalize(light_dir);
  const auto light_pos = target + norm_dir * light_distance;
  ran::Vec3f32 up{0.0f, 1.0f, 0.0f};
  if (std::abs(norm_dir.y) > 0.99f) {
    up = ran::Vec3f32{0.0f, 0.0f, 1.0f};
  }

  return LightMat{
    .view = ran::lookat_rh(light_pos, target, up),
    .proj = ran::ortho_rh(-box_extent, box_extent, -box_extent, box_extent, 1.0f, 120.0f),
  };
}

} // namespace mikugl
