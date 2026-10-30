#pragma once

#include "core.hpp"
#include "model/model_data.hpp"

#include <string_view>

namespace mikugl {

class MorphManager final {
public:
  MorphManager() noexcept = default;
  MorphManager(Span<const ModelVertex> base_vertices, Vec<ModelMorph> morphs) noexcept;

  ~MorphManager() noexcept = default;
  NTF_NO_COPY(MorphManager);
  MorphManager(MorphManager&& other) noexcept = default;
  MorphManager& operator=(MorphManager&& other) noexcept = default;

public:
  Span<const ModelMorph> morphs() const noexcept { return _morphs; }

  Span<const f32> weights() const noexcept { return _weights; }

  u32 morph_count() const noexcept { return static_cast<u32>(_morphs.size()); }

  f32 weight(u32 morph_index) const noexcept;

  void set_weight(u32 morph_index, f32 weight) noexcept;

  void reset_all() noexcept;

  void reset_category(MorphCategory category) noexcept;

  i32 find_morph_index(std::string_view name) const noexcept;

  bool is_dirty() const noexcept { return _dirty; }

  bool evaluate() noexcept;

  Span<const ModelVertex> deformed_vertices() const noexcept { return _deformed_vertices; }

  Span<const ModelVertex> base_vertices() const noexcept { return _base_vertices; }

private:
  Vec<ModelVertex> _base_vertices;
  Vec<ModelVertex> _deformed_vertices;
  Vec<ModelMorph> _morphs;
  Vec<f32> _weights;
  Vec<f32> _effective_weights;
  bool _dirty = false;
};

} // namespace mikugl
