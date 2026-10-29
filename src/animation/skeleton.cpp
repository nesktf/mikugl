#include "animation/skeleton.hpp"

#include <algorithm>
#include <utility>

namespace mikugl {

namespace {

struct SortedBones {
  Vec<Skeleton::RootArmature> armatures;
  Vec<u32> sorted_bones;
  Vec<u32> bone_remapping;
};

Optional<SortedBones> sort_bone_hierarchy(Span<const ModelBone> model_bones) {
  miku_assert(!model_bones.empty() && "Add some bones dummy");
  const u32 total_bones = (u32)model_bones.size();

  Vec<u32> roots;
  roots.reserve(total_bones);
  Vec<Vec<u32>> children(total_bones);

  for (u32 i = 0; i < total_bones; ++i) {
    const auto& mb = model_bones[i];
    if (mb.parent_index < 0 || mb.parent_index >= (i32)total_bones) {
      roots.push_back(i);
    } else {
      children[mb.parent_index].push_back(i);
    }
  }

  if (roots.empty()) {
    return std::nullopt;
  }

  // DFS preorder per root hierarchy
  // root, child 1, child 1 children.,., child 2, child 2 children...
  Vec<u32> sorted_order;
  sorted_order.reserve(total_bones);
  Vec<bool> visited(total_bones, false);
  Vec<Skeleton::RootArmature> armatures;
  armatures.reserve(roots.size());

  const auto dfs = [&](auto& self, u32 node_idx) -> void {
    visited[node_idx] = true;
    sorted_order.push_back(node_idx);
    for (const u32 child_idx : children[node_idx]) {
      if (!visited[child_idx]) {
        self(self, child_idx);
      }
    }
  };

  for (const u32 root_idx : roots) {
    if (visited[root_idx]) {
      continue;
    }
    const u32 start = (u32)sorted_order.size();
    dfs(dfs, root_idx);
    const u32 count = (u32)sorted_order.size() - start;
    armatures.emplace_back(start, count);
  }

  // remaining nodes
  for (size_t i = 0; i < total_bones; ++i) {
    if (visited[i]) {
      continue;
    }
    const u32 start = (u32)sorted_order.size();
    dfs(dfs, (u32)i);
    const u32 count = (u32)sorted_order.size() - start;
    armatures.emplace_back(start, count);
  }

  // old indices -> new sorted indices
  Vec<u32> bone_remapping(total_bones);
  for (u32 s = 0; s < total_bones; ++s) {
    bone_remapping[sorted_order[s]] = s;
  }

  return Optional<SortedBones>{std::in_place, std::move(armatures), std::move(sorted_order),
                               std::move(bone_remapping)};
}

} // namespace

BoneExpect<Skeleton> Skeleton::from_model_bones(Span<const ModelBone> model_bones) {
  auto parsed_bones = sort_bone_hierarchy(model_bones);
  if (!parsed_bones) {
    return Unexpected<BoneError>(BoneError::NoRootNodes);
  }
  auto& [armatures, sorted_bones, bone_remapping] = *parsed_bones;

  const u32 total_bones = (u32)model_bones.size();
  auto bones = make_unique_array<BoneNode>(total_bones);
  auto matrices = make_unique_array<ran::Mat4f32>(MAX_BONES, ran::Mat4f32::identity());

  for (u32 s = 0; s < total_bones; ++s) {
    const u32 orig_idx = sorted_bones[s];
    const auto& mb = model_bones[orig_idx];
    auto& bn = bones[s];
    bn.name_en = mb.name_en;
    bn.name_jp = mb.name_jp;
    bn.rest_position = mb.position;
    bn.transform_order = mb.transform_order;
    bn.flags = mb.flags;
    bn.connection_bone_index =
      (mb.connection_bone_index >= 0 && (u32)mb.connection_bone_index < total_bones)
        ? (i32)bone_remapping[mb.connection_bone_index]
        : mb.connection_bone_index;
    bn.local_translation = ran::Vec3f32{0.0f, 0.0f, 0.0f};
    bn.local_rotation = ran::Quatf32::identity();
    bn.local_scale = ran::Vec3f32{1.0f, 1.0f, 1.0f};
    bn.inv_bind_matrix = ran::translate(ran::Mat4f32::identity(), -bn.rest_position);
    bn.original_index = orig_idx;

    if (mb.parent_index < 0 || mb.parent_index >= (i32)total_bones) {
      bn.parent_index = -1;
    } else {
      const u32 parent = bone_remapping[mb.parent_index];
      miku_assert(parent < s && "Parent should precede child in hierarchy");
      bn.parent_index = (i32)parent;
    }
  }

  Skeleton skelly{create_t(), std::move(bones), std::move(matrices), std::move(armatures),
                  std::move(bone_remapping)};
  skelly.update_fk();
  return skelly;
}

Skeleton::Skeleton(create_t, UniqueArray<BoneNode>&& bones,
                   UniqueArray<ran::Mat4f32>&& skinning_mats, Vec<RootArmature>&& armatures,
                   Vec<u32>&& bone_remap) noexcept :
    _bones(std::move(bones)), _skinning_matrices(std::move(skinning_mats)),
    _armatures(std::move(armatures)), _bone_remap(std::move(bone_remap)), _ubo(0) {}

void Skeleton::_destroy_ubo() noexcept {
  if (_ubo) {
    GLuint u = _ubo;
    glDeleteBuffers(1, &u);
    _ubo = 0;
  }
}

Skeleton::~Skeleton() noexcept {
  _destroy_ubo();
}

Skeleton::Skeleton(Skeleton&& other) noexcept :
    _bones(std::move(other._bones)), _skinning_matrices(std::move(other._skinning_matrices)),
    _cache(std::move(other._cache)), _armatures(std::move(other._armatures)),
    _bone_remap(std::move(other._bone_remap)), _ubo(std::exchange(other._ubo, 0)) {}

Skeleton& Skeleton::operator=(Skeleton&& other) noexcept {
  if (this != &other) {
    _destroy_ubo();
    _bones = std::move(other._bones);
    _skinning_matrices = std::move(other._skinning_matrices);
    _cache = std::move(other._cache);
    _armatures = std::move(other._armatures);
    _bone_remap = std::move(other._bone_remap);
    _ubo = std::exchange(other._ubo, 0);
  }
  return *this;
}

void Skeleton::update_ubo() noexcept {
  miku_assert(!_bones.empty() && !_skinning_matrices.empty() && "No bones!!!!!!");
  if (!_ubo) {
    GLuint ubo = 0;
    glGenBuffers(1, &ubo);
    miku_assert(ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER,
                 static_cast<GLsizeiptr>(_skinning_matrices.size() * sizeof(ran::Mat4f32)),
                 _skinning_matrices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    _ubo = ubo;
  }

  glBindBuffer(GL_UNIFORM_BUFFER, _ubo);
  const size_t count = std::min(static_cast<size_t>(MAX_BONES), _skinning_matrices.size());
  glBufferSubData(GL_UNIFORM_BUFFER, 0, static_cast<GLsizeiptr>(count * sizeof(ran::Mat4f32)),
                  _skinning_matrices.data());
  glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Skeleton::bind_ubo(u32 binding_point) const noexcept {
  if (_ubo) {
    glBindBufferBase(GL_UNIFORM_BUFFER, binding_point, _ubo);
  }
}

void Skeleton::update_fk() noexcept {
  miku_assert(!_bones.empty() && !_skinning_matrices.empty() && "No bones!!!!!!");
  const size_t total_count = _bones.size();

  _cache.clear();
  _cache.resize(total_count * 2, ran::Mat4f32::identity());

  ran::Mat4f32* locals = _cache.data();
  ran::Mat4f32* models = _cache.data() + total_count;

  for (const auto& arm : _armatures) {
    const u32 root_idx = arm.start_index;
    auto& root = _bones[root_idx];
    miku_assert(root.parent_index == -1);

    root.world_position = root.rest_position + root.local_translation;
    root.world_rotation = root.local_rotation;

    locals[root_idx] = ran::translate(ran::Mat4f32::identity(), root.world_position) *
                       ran::to_mat4(root.world_rotation);
    models[root_idx] = locals[root_idx];
    root.world_matrix = models[root_idx];
    root.skinning_matrix = models[root_idx] * root.inv_bind_matrix;
    miku_assert(root.original_index < _skinning_matrices.size());
    _skinning_matrices[root.original_index] = root.skinning_matrix;

    const u32 last_idx = arm.start_index + arm.count;
    for (u32 i = root_idx + 1; i < last_idx; ++i) {
      auto& node = _bones[i];
      const u32 parent_idx = (u32)node.parent_index;
      miku_assert(parent_idx >= root_idx && parent_idx < i);
      const auto& parent = _bones[parent_idx];

      const auto rest_offset = node.rest_position - parent.rest_position;
      node.world_position =
        parent.world_position + (parent.world_rotation * (rest_offset + node.local_translation));
      node.world_rotation = parent.world_rotation * node.local_rotation;

      locals[i] = ran::translate(ran::Mat4f32::identity(), rest_offset + node.local_translation) *
                  ran::to_mat4(node.local_rotation);
      models[i] = models[parent_idx] * locals[i];
      node.world_matrix = models[i];
      node.skinning_matrix = models[i] * node.inv_bind_matrix;
      miku_assert(node.original_index < _skinning_matrices.size());
      _skinning_matrices[node.original_index] = node.skinning_matrix;
    }
  }
}

void Skeleton::reset_pose() noexcept {
  for (auto& bn : _bones) {
    bn.local_translation = ran::Vec3f32{0.0f, 0.0f, 0.0f};
    bn.local_rotation = ran::Quatf32::identity();
    bn.local_scale = ran::Vec3f32{1.0f, 1.0f, 1.0f};
  }
  update_fk();
}

void Skeleton::set_bone_rotation(u32 bone_index, const ran::Quatf32& rotation) noexcept {
  miku_assert(bone_index < (u32)_bone_remap.size());
  const u32 idx = _bone_remap[bone_index];
  miku_assert(idx < (u32)_bones.size());
  _bones[idx].local_rotation = rotation;
}

void Skeleton::set_bone_translation(u32 bone_index, const ran::Vec3f32& translation) noexcept {
  miku_assert(bone_index < (u32)_bone_remap.size());
  const u32 idx = _bone_remap[bone_index];
  miku_assert(idx < (u32)_bones.size());
  _bones[idx].local_translation = translation;
}

i32 Skeleton::find_bone_index(std::string_view name) const noexcept {
  for (size_t i = 0; i < _bones.size(); ++i) {
    if (_bones[i].name_en == name || _bones[i].name_jp == name) {
      return (u32)_bones[i].original_index;
    }
  }
  return -1;
}

const Skeleton::BoneNode& Skeleton::bone(u32 index) const noexcept {
  miku_assert(index < (u32)_bone_remap.size());
  const u32 idx = _bone_remap[index];
  miku_assert(idx < (u32)_bones.size());
  return _bones[idx];
}

Skeleton::BoneNode& Skeleton::bone(u32 index) noexcept {
  miku_assert(index < (u32)_bone_remap.size());
  const u32 idx = _bone_remap[index];
  miku_assert(idx < (u32)_bones.size());
  return _bones[idx];
}

} // namespace mikugl
