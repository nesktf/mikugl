#pragma once

#include "core.hpp"

#include <ranmath/ran.hpp>

namespace mikugl {

class Skeleton;

struct IkLink {
  u32 bone_index;
  bool has_limits;
  ran::Vec3f32 min_angle;
  ran::Vec3f32 max_angle;
  // bool is_knee;
};

struct IkChain {
  u32 ik_bone_index;
  u32 target_bone_index;
  i32 loop_count;
  f32 limit_angle;
  Vec<IkLink> links;
  bool enabled;
};

class IkSolver final {
public:
  static void solve(Skeleton& skeleton, Span<const IkChain> chains) noexcept;
  static void solve_chain(Skeleton& skeleton, const IkChain& chain) noexcept;
};

} // namespace mikugl
