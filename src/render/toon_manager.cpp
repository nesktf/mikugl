#include "render/toon_manager.hpp"

#include <glad/glad.h>

#include <fmt/format.h>
#include <stb_image.h>

#include <filesystem>

namespace stdfs = std::filesystem;

namespace mikugl {

namespace {

stdfs::path find_toon_file(std::string_view filename) {
  const std::array candidate_dirs = {
    std::string("data"),
#ifdef MIKU_DATA_DIR
    std::string(MIKU_DATA_DIR),
#endif
    std::string("../data"),
  };

  for (const auto& dir : candidate_dirs) {
    const auto full_path = stdfs::path(dir) / filename;
    if (stdfs::exists(full_path)) {
      return full_path;
    }
  }

  return "";
}

} // namespace

ToonManager ToonManager::create() {
  std::array<u32, shared_toon_count> textures{};
  for (size_t i = 0; i < shared_toon_count; ++i) {
    const auto filename = fmt::format("toon{:02d}.bmp", i + 1);
    const auto full_path = find_toon_file(filename);

    if (full_path.empty()) {
      miku_panic("Failed to find shared toon texture in data folder");
    }

    int w = 0;
    int h = 0;
    int comp = 0;
    static constexpr int req_comp = 4;
    u8* pixels = stbi_load(full_path.c_str(), &w, &h, &comp, req_comp);
    if (!pixels) {
      miku_panic("Failed to decode shared toon texture in data folder");
    }

    const DeferFn free_pixels([&]() { stbi_image_free(pixels); });

    GLuint tex_id = 0;
    glGenTextures(1, &tex_id);
    miku_assert(tex_id); // handle errors later
    glBindTexture(GL_TEXTURE_2D, tex_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    glBindTexture(GL_TEXTURE_2D, 0);
    textures[i] = (u32)tex_id;
  }
  return {create_t(), textures};
}

void ToonManager::_destroy() noexcept {
  for (auto& tex : _toon_textures) {
    if (tex != 0) {
      GLuint t = tex;
      glDeleteTextures(1, &t);
      tex = 0;
    }
  }
}

ToonManager::ToonManager(create_t, Span<const u32, shared_toon_count> textures) noexcept {
  std::memcpy(_toon_textures.data(), textures.data(), textures.size_bytes());
}

ToonManager::~ToonManager() noexcept {
  _destroy();
}

ToonManager::ToonManager(ToonManager&& other) noexcept : _toon_textures(other._toon_textures) {
  other._toon_textures.fill(0);
}

ToonManager& ToonManager::operator=(ToonManager&& other) noexcept {
  if (this != &other) {
    _destroy();
    _toon_textures = other._toon_textures;
    other._toon_textures.fill(0);
  }
  return *this;
}

void ToonManager::bind_shared_toon(u8 index, u32 slot) const noexcept {
  if (index >= shared_toon_count) {
    return;
  }

  const auto tex = (GLuint)_toon_textures[index];
  if (tex) {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, tex);
  }
}

} // namespace mikugl
