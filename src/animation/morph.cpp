#include "animation/morph.hpp"

namespace mikugl {

namespace {
constexpr f32 weight_min = 1e-6f;

void apply_group_morph(const ModelMorph& morph, f32 weight, Vec<f32>& effective_weights,
                       Span<const ModelMorph> all_morphs, i32 depth) noexcept {
  if (depth > 8 || ran::abs(weight) < weight_min) {
    return;
  }
  for (const auto& go : morph.group_offsets) {
    miku_assert(go.morph_index >= 0 && (size_t)go.morph_index < all_morphs.size());
    const f32 sub_weight = weight * go.factor;
    const auto& target = all_morphs[go.morph_index];
    if (target.type == MorphType::Group) {
      apply_group_morph(target, sub_weight, effective_weights, all_morphs, depth + 1);
    } else {
      effective_weights[go.morph_index] += sub_weight;
    }
  }
}

} // namespace

MorphManager::MorphManager(Span<const ModelVertex> base_vertices, Vec<ModelMorph> morphs) noexcept
    :
    _base_vertices(base_vertices.begin(), base_vertices.end()),
    _deformed_vertices(base_vertices.begin(), base_vertices.end()), _morphs(std::move(morphs)),
    _weights(_morphs.size(), 0.0f), _dirty(true) {}

f32 MorphManager::weight(u32 morph_index) const noexcept {
  miku_assert(morph_index < _weights.size());
  return _weights[morph_index];
}

void MorphManager::set_weight(u32 morph_index, f32 weight) noexcept {
  miku_assert(morph_index < _weights.size());
  if (ran::abs(_weights[morph_index] - weight) > weight_min) {
    _weights[morph_index] = weight;
    _dirty = true;
  }
}

void MorphManager::reset_all() noexcept {
  for (auto& w : _weights) {
    if (ran::abs(w) > weight_min) {
      w = 0.0f;
      _dirty = true;
    }
  }
}

void MorphManager::reset_category(MorphCategory category) noexcept {
  for (size_t i = 0; i < _morphs.size(); ++i) {
    if (_morphs[i].category == category && ran::abs(_weights[i]) > weight_min) {
      _weights[i] = 0.0f;
      _dirty = true;
    }
  }
}

i32 MorphManager::find_morph_index(std::string_view name) const noexcept {
  for (size_t i = 0; i < _morphs.size(); ++i) {
    if (_morphs[i].name_jp == name || _morphs[i].name_en == name) {
      return (i32)i;
    }
  }
  return -1;
}

bool MorphManager::evaluate() noexcept {
  if (!_dirty) {
    return false;
  }

  _effective_weights.clear();
  _effective_weights.resize(_morphs.size(), 0.f);
  for (size_t i = 0; i < _morphs.size(); ++i) {
    if (_morphs[i].type != MorphType::Group) {
      _effective_weights[i] += _weights[i];
    }
  }

  for (size_t i = 0; i < _morphs.size(); ++i) {
    if (_morphs[i].type == MorphType::Group && ran::abs(_weights[i]) > weight_min) {
      apply_group_morph(_morphs[i], _weights[i], _effective_weights, _morphs, 0);
    }
  }

  _deformed_vertices = _base_vertices;

  for (size_t i = 0; i < _morphs.size(); ++i) {
    const auto& m = _morphs[i];
    if (m.type == MorphType::Vertex) {
      const f32 w = _effective_weights[i];
      if (ran::abs(w) > weight_min) {
        for (const auto& vo : m.vertex_offsets) {
          if (vo.vertex_index < _deformed_vertices.size()) {
            _deformed_vertices[vo.vertex_index].position += vo.position_offset * w;
          }
        }
      }
    }
  }

  _dirty = false;
  return true;
}

} // namespace mikugl
