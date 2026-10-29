#include "render/texture_manager.hpp"

#include <glad/glad.h>

#include <utility>

namespace mikugl {

TextureManager TextureManager::create() {
  GLuint tex = 0;
  glGenTextures(1, &tex);
  miku_assert(tex); // handle errors later
  glBindTexture(GL_TEXTURE_2D, tex);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               missing_texture.data());

  glBindTexture(GL_TEXTURE_2D, 0);
  return {create_t(), (u32)tex};
}

void TextureManager::_destroy() noexcept {
  clear();
  if (_missing_texture) {
    GLuint t = _missing_texture;
    glDeleteTextures(1, &t);
  }
}

void TextureManager::clear() noexcept {
  if (!_textures.empty()) {
    glDeleteTextures((GLsizei)_textures.size(), _textures.data());
  }
  _textures.clear();
}

TextureManager::TextureManager(create_t, u32 missing) noexcept :
    _missing_texture(missing), _textures() {}

TextureManager::~TextureManager() noexcept {
  _destroy();
}

TextureManager::TextureManager(TextureManager&& other) noexcept :
    _missing_texture(std::exchange(other._missing_texture, 0)),
    _textures(std::move(other._textures)) {}

TextureManager& TextureManager::operator=(TextureManager&& other) noexcept {
  if (this != &other) {
    _missing_texture = std::exchange(other._missing_texture, 0);
    _textures = std::move(other._textures);
  }
  return *this;
}

void TextureManager::upload_textures(Span<const TextureCpuBuffer> textures) {
  clear();
  _textures.reserve(textures.size());

  for (size_t i = 0; i < textures.size(); ++i) {
    const TextureCpuBuffer& buf = textures[i];
    if (!buf.valid || buf.pixels.empty() || buf.width <= 0 || buf.height <= 0) {
      _textures[i] = 0;
      continue;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    miku_assert(tex); // handle errors later
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, buf.width, buf.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 buf.pixels.data());

    glGenerateMipmap(GL_TEXTURE_2D);

    _textures.emplace_back((u32)tex);
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

bool TextureManager::has_texture(i32 texture_index) const noexcept {
  return texture_index >= 0 && (size_t)texture_index < _textures.size() &&
         _textures[texture_index] != 0;
}

} // namespace mikugl
