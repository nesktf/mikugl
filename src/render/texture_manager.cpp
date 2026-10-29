#include "render/texture_manager.hpp"

#include <glad/glad.h>

#include <utility>

namespace mikugl {

TextureManager::TextureManager() noexcept : _missing_texture(0), _textures() {}

TextureManager::~TextureManager() noexcept {
  clear();
  if (_missing_texture) {
    glDeleteTextures(1, &_missing_texture);
  }
}

TextureManager::TextureManager(TextureManager&& other) noexcept :
    _missing_texture(std::exchange(other._missing_texture, 0)),
    _textures(std::move(other._textures)) {}

TextureManager& TextureManager::operator=(TextureManager&& other) noexcept {
  if (this != &other) {
    clear();
    if (_missing_texture) {
      glDeleteTextures(1, &_missing_texture);
    }
    _missing_texture = std::exchange(other._missing_texture, 0);
    _textures = std::move(other._textures);
  }
  return *this;
}

void TextureManager::init() {
  if (_missing_texture) {
    return;
  }

  GLuint tex = 0;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               missing_texture.data());

  glBindTexture(GL_TEXTURE_2D, 0);
  _missing_texture = (u32)tex;
}

void TextureManager::upload_textures(Span<const TextureCpuBuffer> textures) {
  clear();

  if (!_missing_texture) {
    init();
  }

  _textures.resize(textures.size(), 0);

  for (size_t i = 0; i < textures.size(); ++i) {
    const TextureCpuBuffer& buf = textures[i];
    if (!buf.valid || buf.pixels.empty() || buf.width <= 0 || buf.height <= 0) {
      _textures[i] = 0;
      continue;
    }

    GLuint tex_id = 0;
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, buf.width, buf.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 buf.pixels.data());

    glGenerateMipmap(GL_TEXTURE_2D);

    _textures[i] = tex_id;
  }

  glBindTexture(GL_TEXTURE_2D, 0);
}

void TextureManager::bind_texture(i32 texture_index, u32 slot) const noexcept {
  glActiveTexture(GL_TEXTURE0 + slot);

  GLuint target = _missing_texture;
  if (texture_index >= 0 && (size_t)texture_index < _textures.size()) {
    const auto tex = _textures[texture_index];
    if (tex != 0) {
      target = (GLuint)tex;
    }
  }

  glBindTexture(GL_TEXTURE_2D, target);
}

void TextureManager::clear() noexcept {
  for (const GLuint tex : _textures) {
    if (tex != 0) {
      glDeleteTextures(1, &tex);
    }
  }
  _textures.clear();
}

} // namespace mikugl
