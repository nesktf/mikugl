#pragma once

#include "core.hpp"
#include "model/model_data.hpp"

namespace mikugl {

class TextureManager final {
public:
  TextureManager() noexcept;
  ~TextureManager() noexcept;
  NTF_NO_COPY(TextureManager);
  TextureManager(TextureManager&& other) noexcept;
  TextureManager& operator=(TextureManager&& other) noexcept;

public:
  void init();
  void upload_textures(Span<const TextureCpuBuffer> textures);
  void bind_texture(i32 texture_index, u32 slot = 0) const noexcept;
  void clear() noexcept;

public:
  u32 missing_texture_id() const noexcept { return _missing_texture; }

  size_t texture_count() const noexcept { return _textures.size(); }

private:
  u32 _missing_texture;
  Vec<u32> _textures;
};

} // namespace mikugl
