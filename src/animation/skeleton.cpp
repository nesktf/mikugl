#include "animation/skeleton.hpp"

#include <utility>

#include <fmt/format.h>

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

Vec<IkChain> generate_ik_chains(Span<const ModelBone> model_bones,
                                Span<const u32> bone_remapping) {
  const u32 total_bones = (u32)model_bones.size();
  Vec<IkChain> chains;
  for (u32 idx = 0; idx < total_bones; ++idx) {
    const auto& mb = model_bones[idx];
    if (mb.ik_target_index < 0 || mb.ik_links.empty()) {
      continue;
    }
    miku_assert((u32)mb.ik_target_index < total_bones);

    Vec<IkLink> links;
    links.reserve(mb.ik_links.size());
    for (const auto& ml : mb.ik_links) {
      if (ml.bone_index < 0) {
        continue;
      }
      miku_assert((u32)ml.bone_index < total_bones);

      IkLink link{};
      link.bone_index = bone_remapping[ml.bone_index];
      link.has_limits = ml.has_limits;
      link.min_angle = ml.min_angle;
      link.max_angle = ml.max_angle;
#if 0
      const auto& link_mb = model_bones[ml.bone_index];
      link.is_knee =
        ml.has_limits && (link_mb.name_jp.find("\x82\xd0\x82\xb4") != std::string::npos ||
                          link_mb.name_jp.find("膝") != std::string::npos ||
                          link_mb.name_en.find("knee") != std::string::npos ||
                          link_mb.name_en.find("Knee") != std::string::npos ||
                          (ml.min_angle.y == 0.0f && ml.max_angle.y == 0.0f &&
                           ml.min_angle.z == 0.0f && ml.max_angle.z == 0.0f));

#endif

      // hack to invert the coordinate system of the knee iks
      // (pmx iks expect directx coords apparently)
      // TODO: check if this is applicable in general
      const f32 abs_min = ran::abs(ml.min_angle.x);
      const f32 abs_max = ran::abs(ml.max_angle.x);
      link.min_angle.x = ran::min(abs_min, abs_max);
      link.max_angle.x = ran::max(abs_min, abs_max);
      if (link.max_angle.x < 0.1f) {
        link.max_angle.x = ran::pi<f32>;
      }
      if (link.min_angle.x < 0.00872665f) {
        link.min_angle.x = 0.00872665f;
      }

      links.push_back(link);
    }
    if (links.empty()) {
      continue;
    }
    IkChain chain{};
    chain.ik_bone_index = bone_remapping[idx];
    chain.target_bone_index = bone_remapping[mb.ik_target_index];
    chain.loop_count = (mb.ik_loop_count > 0) ? mb.ik_loop_count : 40;
    chain.limit_angle = (mb.ik_limit_angle > 0.0f) ? mb.ik_limit_angle : (ran::pi<f32> / 4.0f);
    chain.links = std::move(links);
    chain.enabled = true;
    chains.push_back(std::move(chain));
  }
  return chains;
}

} // namespace

BoneExpect<Skeleton> Skeleton::from_model_bones(Span<const ModelBone> model_bones) {
  auto parsed_bones = sort_bone_hierarchy(model_bones);
  if (!parsed_bones) {
    return Unexpected<BoneError>(BoneError::NoRootNodes);
  }
  auto& [armatures, sorted_bones, bone_remapping] = *parsed_bones;

  const u32 total_bones = (u32)model_bones.size();
  auto ik_chains = generate_ik_chains(model_bones, bone_remapping);

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
    bn.visible = true;

    if (mb.parent_index < 0 || mb.parent_index >= (i32)total_bones) {
      bn.parent_index = -1;
    } else {
      const u32 parent = bone_remapping[mb.parent_index];
      miku_assert(parent < s && "Parent should precede child in hierarchy");
      bn.parent_index = (i32)parent;
    }
  }

  Skeleton skelly{create_t(),           std::move(bones),          std::move(matrices),
                  std::move(armatures), std::move(bone_remapping), std::move(ik_chains)};
  skelly.update_pose();
  return skelly;
}

Skeleton::Skeleton(create_t, UniqueArray<BoneNode>&& bones,
                   UniqueArray<ran::Mat4f32>&& skinning_mats, Vec<RootArmature>&& armatures,
                   Vec<u32>&& bone_remap, Vec<IkChain>&& ik_chains) noexcept :
    _bones(std::move(bones)), _skinning_matrices(std::move(skinning_mats)),
    _armatures(std::move(armatures)), _bone_remap(std::move(bone_remap)),
    _ik_chains(std::move(ik_chains)), _ik_enabled(true), _ubo(0) {}

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
    _bone_remap(std::move(other._bone_remap)), _ik_chains(std::move(other._ik_chains)),
    _ik_enabled(other._ik_enabled), _ubo(std::exchange(other._ubo, 0)) {}

Skeleton& Skeleton::operator=(Skeleton&& other) noexcept {
  if (this != &other) {
    _destroy_ubo();
    _bones = std::move(other._bones);
    _skinning_matrices = std::move(other._skinning_matrices);
    _cache = std::move(other._cache);
    _armatures = std::move(other._armatures);
    _bone_remap = std::move(other._bone_remap);
    _ik_chains = std::move(other._ik_chains);
    _ik_enabled = other._ik_enabled;
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
  const size_t count = ran::min(static_cast<size_t>(MAX_BONES), _skinning_matrices.size());
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

    locals[root_idx] = ran::scale(ran::translate(ran::Mat4f32::identity(), root.world_position) *
                                    ran::to_mat4(root.world_rotation),
                                  root.local_scale);
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
      locals[i] =
        ran::scale(ran::translate(ran::Mat4f32::identity(), rest_offset + node.local_translation) *
                     ran::to_mat4(node.local_rotation),
                   node.local_scale);
      models[i] = models[parent_idx] * locals[i];
      node.world_position = ran::Vec3f32{models[i].x4, models[i].y4, models[i].z4};
      node.world_rotation = parent.world_rotation * node.local_rotation;
      node.world_matrix = models[i];
      node.skinning_matrix = models[i] * node.inv_bind_matrix;
      miku_assert(node.original_index < _skinning_matrices.size());
      _skinning_matrices[node.original_index] = node.skinning_matrix;
    }
  }
}

void Skeleton::update_pose() noexcept {
  update_fk();
  if (_ik_enabled && !_ik_chains.empty()) {
    IkSolver::solve(*this, _ik_chains);
    update_fk();
  }
}

void Skeleton::solve_ik() noexcept {
  if (_ik_enabled && !_ik_chains.empty()) {
    IkSolver::solve(*this, _ik_chains);
  }
}

void Skeleton::reset_pose() noexcept {
  for (auto& bn : _bones) {
    bn.local_translation = ran::Vec3f32{0.0f, 0.0f, 0.0f};
    bn.local_rotation = ran::Quatf32::identity();
    bn.local_scale = ran::Vec3f32{1.0f, 1.0f, 1.0f};
  }
  update_pose();
}

void Skeleton::set_bone_rotation(u32 bone_index, const ran::Quatf32& rotation) {
  miku_assert(bone_index < (u32)_bone_remap.size());
  const u32 idx = _bone_remap[bone_index];
  miku_assert(idx < (u32)_bones.size());
  _bones[idx].local_rotation = rotation;
}

void Skeleton::set_bone_translation(u32 bone_index, const ran::Vec3f32& translation) {
  miku_assert(bone_index < (u32)_bone_remap.size());
  const u32 idx = _bone_remap[bone_index];
  miku_assert(idx < (u32)_bones.size());
  _bones[idx].local_translation = translation;
}

void Skeleton::set_bone_scale(u32 bone_index, const ran::Vec3f32& scale) {
  miku_assert(bone_index < (u32)_bone_remap.size());
  const u32 idx = _bone_remap[bone_index];
  miku_assert(idx < (u32)_bones.size());
  _bones[idx].local_scale = scale;
}

void Skeleton::reset_node_pose(u32 sorted_index) noexcept {
  if (sorted_index >= (u32)_bones.size()) {
    return;
  }
  auto& bn = _bones[sorted_index];
  bn.local_translation = ran::Vec3f32{0.0f, 0.0f, 0.0f};
  bn.local_rotation = ran::Quatf32::identity();
  bn.local_scale = ran::Vec3f32{1.0f, 1.0f, 1.0f};
  update_pose();
}

void Skeleton::set_all_bones_visible(bool visible) noexcept {
  for (auto& bn : _bones) {
    bn.visible = visible;
  }
}

void Skeleton::set_node_world_position(u32 sorted_index, const ran::Vec3f32& world_pos) noexcept {
  if (sorted_index >= (u32)_bones.size()) {
    return;
  }
  auto& bn = _bones[sorted_index];
  if (bn.parent_index < 0 || (u32)bn.parent_index >= (u32)_bones.size()) {
    bn.local_translation = world_pos - bn.rest_position;
  } else {
    const auto& parent = _bones[(u32)bn.parent_index];
    const ran::Mat4f32 inv_parent = ran::inverse(parent.world_matrix);
    const ran::Vec4f32 local_p =
      inv_parent * ran::Vec4f32{world_pos.x, world_pos.y, world_pos.z, 1.0f};
    const ran::Vec3f32 rest_offset = bn.rest_position - parent.rest_position;
    bn.local_translation = ran::Vec3f32{local_p.x, local_p.y, local_p.z} - rest_offset;
  }
}

void Skeleton::set_node_world_rotation(u32 sorted_index, const ran::Quatf32& world_rot) noexcept {
  if (sorted_index >= (u32)_bones.size()) {
    return;
  }
  auto& bn = _bones[sorted_index];
  if (bn.parent_index < 0 || (u32)bn.parent_index >= (u32)_bones.size()) {
    bn.local_rotation = ran::normalize(world_rot);
  } else {
    const auto& parent = _bones[(u32)bn.parent_index];
    bn.local_rotation = ran::normalize(ran::inverse(parent.world_rotation) * world_rot);
  }
}

i32 Skeleton::find_bone_index(std::string_view name) const noexcept {
  for (size_t i = 0; i < _bones.size(); ++i) {
    if (_bones[i].name_en == name || _bones[i].name_jp == name) {
      return (u32)_bones[i].original_index;
    }
  }
  return -1;
}

const Skeleton::BoneNode& Skeleton::bone(u32 index) const {
  miku_assert(index < (u32)_bone_remap.size());
  const u32 idx = _bone_remap[index];
  miku_assert(idx < (u32)_bones.size());
  return _bones[idx];
}

Skeleton::BoneNode& Skeleton::bone(u32 index) {
  miku_assert(index < (u32)_bone_remap.size());
  const u32 idx = _bone_remap[index];
  miku_assert(idx < (u32)_bones.size());
  return _bones[idx];
}

const Skeleton::BoneNode& Skeleton::node(u32 sorted_index) const {
  miku_assert((size_t)sorted_index < _bones.size());
  return _bones[sorted_index];
}

Skeleton::BoneNode& Skeleton::node(u32 sorted_index) {
  miku_assert((size_t)sorted_index < _bones.size());
  return _bones[sorted_index];
}

std::string format_bone_label(const Skeleton::BoneNode& b, u32 index) {
  if (!b.name_en.empty() && !b.name_jp.empty() && b.name_en != b.name_jp) {
    return fmt::format("{} ({})", b.name_en, b.name_jp);
  }
  if (!b.name_en.empty()) {
    return b.name_en;
  }
  if (!b.name_jp.empty()) {
    return b.name_jp;
  }
  return fmt::format("Bone_{}", index);
}

} // namespace mikugl
