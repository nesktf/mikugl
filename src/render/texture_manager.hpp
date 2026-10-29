#pragma once

#include "core.hpp"
#include "model/model_data.hpp"

namespace mikugl {

class TextureManager final {
private:
  struct create_t {};

public:
  static TextureManager create();

public:
  TextureManager(create_t, u32 missing) noexcept;
  ~TextureManager() noexcept;
  NTF_NO_COPY(TextureManager);
  TextureManager(TextureManager&& other) noexcept;
  TextureManager& operator=(TextureManager&& other) noexcept;

public:
  void upload_textures(Span<const TextureCpuBuffer> textures);
  void bind_texture(i32 texture_index, u32 slot = 0) const noexcept;
  void clear() noexcept;

public:
  bool has_texture(i32 texture_index) const noexcept;

  u32 missing_texture_id() const noexcept { return _missing_texture; }

  size_t texture_count() const noexcept { return _textures.size(); }

private:
  void _destroy() noexcept;

private:
  u32 _missing_texture;
  Vec<u32> _textures;
};

} // namespace mikugl
