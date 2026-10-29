#pragma once

#include "core.hpp"
#include "model/model_data.hpp"

#include <glad/glad.h>
#include <ranmath/ran.hpp>

#include <string>
#include <string_view>

namespace mikugl {

enum class BoneError {
  NoRootNodes,
};

template<typename T>
using BoneExpect = Expected<T, BoneError>;

class Skeleton final {
private:
  struct create_t {};

public:
  struct BoneNode {
    std::string name_en;
    std::string name_jp;
    ran::Vec3f32 rest_position;
    i32 parent_index;
    i32 transform_order;
    u16 flags;
    i32 connection_bone_index;
    ran::Vec3f32 local_translation;
    ran::Quatf32 local_rotation;
    ran::Vec3f32 local_scale;
    ran::Vec3f32 world_position;
    ran::Quatf32 world_rotation;
    ran::Mat4f32 world_matrix;
    ran::Mat4f32 inv_bind_matrix;
    ran::Mat4f32 skinning_matrix;
    u32 original_index;
  };

  struct RootArmature {
    u32 start_index;
    u32 count;
  };

public:
  static constexpr u32 MAX_BONES = 512;

public:
  static BoneExpect<Skeleton> from_model_bones(Span<const ModelBone> bones);

public:
  Skeleton(create_t, UniqueArray<BoneNode>&& bones, UniqueArray<ran::Mat4f32>&& skinning_mats,
           Vec<RootArmature>&& armatures, Vec<u32>&& bone_remap) noexcept;
  ~Skeleton() noexcept;
  NTF_NO_COPY(Skeleton);
  Skeleton(Skeleton&& other) noexcept;
  Skeleton& operator=(Skeleton&& other) noexcept;

public:
  void update_fk() noexcept;
  void reset_pose() noexcept;

  void set_bone_rotation(u32 bone_index, const ran::Quatf32& rotation) noexcept;
  void set_bone_translation(u32 bone_index, const ran::Vec3f32& translation) noexcept;

  i32 find_bone_index(std::string_view name) const noexcept;
  const BoneNode& bone(u32 index) const noexcept;
  BoneNode& bone(u32 index) noexcept;
  void update_ubo() noexcept;
  void bind_ubo(u32 binding_point) const noexcept;

public:
  u32 bone_count() const noexcept { return static_cast<u32>(_bones.size()); }

  Span<const BoneNode> bones() const noexcept { return _bones; }

  Span<const ran::Mat4f32> skinning_matrices() const noexcept { return _skinning_matrices; }

  Span<const RootArmature> armatures() const noexcept { return _armatures; }

private:
  void _destroy_ubo() noexcept;

private:
  UniqueArray<BoneNode> _bones;
  UniqueArray<ran::Mat4f32> _skinning_matrices;
  Vec<ran::Mat4f32> _cache;
  Vec<RootArmature> _armatures;
  Vec<u32> _bone_remap;
  u32 _ubo;
};

} // namespace mikugl
