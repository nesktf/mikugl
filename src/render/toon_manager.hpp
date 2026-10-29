#pragma once

#include "core.hpp"

#include <array>

namespace mikugl {

class ToonManager final {
private:
  struct create_t {};

public:
  static constexpr size_t shared_toon_count = 10;

public:
  static ToonManager create();

public:
  ToonManager(create_t, Span<const u32, shared_toon_count> textures) noexcept;
  ~ToonManager() noexcept;
  NTF_NO_COPY(ToonManager);
  ToonManager(ToonManager&& other) noexcept;
  ToonManager& operator=(ToonManager&& other) noexcept;

public:
  void bind_shared_toon(u8 index, u32 slot = 0) const noexcept;

public:
  u32 shared_toon_id(u8 index) const noexcept {
    return (index < shared_toon_count) ? _toon_textures[index] : 0;
  }

private:
  void _destroy() noexcept;

private:
  std::array<u32, shared_toon_count> _toon_textures;
};

} // namespace mikugl
