#include "animation/ik_solver.hpp"
#include "animation/skeleton.hpp"

namespace mikugl {

namespace {

ran::Quatf32 normalize_quat(const ran::Quatf32& q) noexcept {
  static constexpr f32 len_threshold = 1e-8f;
  const f32 d = ran::dot(q, q);
  if (d > len_threshold) {
    const f32 inv_len = 1.0f / ran::sqrt(d);
    return q * inv_len;
  }
  return ran::Quatf32::identity();
}

} // namespace

void IkSolver::solve(Skeleton& skeleton, Span<const IkChain> chains) noexcept {
  for (const auto& chain : chains) {
    solve_chain(skeleton, chain);
  }
}

void IkSolver::solve_chain(Skeleton& skeleton, const IkChain& chain) noexcept {
  if (!chain.enabled || chain.links.empty()) {
    return;
  }

  const auto update_chain_transforms = [&](size_t link_start_idx) {
    for (i32 i = static_cast<i32>(link_start_idx); i >= 0; --i) {
      auto& current = skeleton.node(chain.links[static_cast<size_t>(i)].bone_index);
      if (current.parent_index >= 0) {
        const auto& parent = skeleton.node(static_cast<u32>(current.parent_index));
        const auto rest_offset = current.rest_position - parent.rest_position;
        current.world_rotation = parent.world_rotation * current.local_rotation;
        current.world_position =
          parent.world_position +
          (parent.world_rotation * (rest_offset + current.local_translation));
      }
    }

    auto& target = skeleton.node(chain.target_bone_index);
    if (target.parent_index >= 0) {
      const auto& parent = skeleton.node(static_cast<u32>(target.parent_index));
      const auto rest_offset = target.rest_position - parent.rest_position;
      target.world_rotation = parent.world_rotation * target.local_rotation;
      target.world_position =
        parent.world_position + (parent.world_rotation * (rest_offset + target.local_translation));
    }
  };

  const auto& ik_bone = skeleton.node(chain.ik_bone_index);
  const auto goal_pos = ik_bone.world_position;
  const f32 max_step_angle =
    (chain.limit_angle > 0.0f) ? chain.limit_angle : (ran::pi<f32> / 4.0f);
  const i32 loops = (chain.loop_count > 0) ? chain.loop_count : 40;

  static constexpr f32 dist_threshold = 1e-8f;
  for (i32 iter = 0; iter < loops; ++iter) {
    const auto& target_node = skeleton.node(chain.target_bone_index);
    if (ran::length2(goal_pos - target_node.world_position) < dist_threshold) {
      break;
    }

    for (size_t link_idx = 0; link_idx < chain.links.size(); ++link_idx) {
      const auto& link = chain.links[link_idx];
      auto& link_node = skeleton.node(link.bone_index);

      const auto link_pos = link_node.world_position;
      const auto effector_pos = skeleton.node(chain.target_bone_index).world_position;
      const auto to_eff = effector_pos - link_pos;
      const auto to_goal = goal_pos - link_pos;

      static constexpr f32 min_len = 1e-5f;
      const f32 len_eff = ran::length(to_eff);
      const f32 len_goal = ran::length(to_goal);
      if (len_eff < min_len || len_goal < min_len) {
        continue;
      }

      const auto dir_eff = to_eff / len_eff;
      const auto dir_goal = to_goal / len_goal;

      const f32 cos_theta = ran::clamp(ran::dot(dir_eff, dir_goal), -1.0f, 1.0f);
      if (cos_theta > 0.999999f) {
        continue;
      }

      const auto axis_world = ran::cross(dir_eff, dir_goal);
      const f32 axis_len = ran::length(axis_world);
      if (axis_len < 1e-6f) {
        continue;
      }
      const auto norm_axis_world = axis_world / axis_len;
      const f32 raw_angle = ran::acos(cos_theta);

#if 0
      if (link.is_knee) {
        const ran::Vec3f32 axis_link = ran::conjugate(link_node.world_rotation) * norm_axis_world;
        const f32 sign = (axis_link.x >= 0.0f) ? 1.0f : -1.0f;
        const f32 rot_step = ran::clamp(raw_angle * sign, -max_step_angle, max_step_angle);
        const f32 cur_x = ran::pitch(link_node.local_rotation);
        f32 new_x = cur_x + rot_step;
        if (link.has_limits) {
          const f32 min_x = ran::min(link.min_angle.x, link.max_angle.x);
          const f32 max_x = ran::max(link.min_angle.x, link.max_angle.x);
          new_x = ran::clamp(new_x, min_x, max_x);
        } else {
          new_x = ran::clamp(new_x, 0.00872665f, ran::pi<f32>);
        }
        link_node.local_rotation = ran::to_quat(ran::Vec3f32{new_x, 0.0f, 0.0f});
      } else
#endif

      if (link.has_limits) {
        const ran::Vec3f32 axis_link =
          ran::normalize(ran::conjugate(link_node.world_rotation) * norm_axis_world);
        const f32 step_angle = ran::min(raw_angle, max_step_angle);
        const auto delta_q = ran::to_quat(step_angle, axis_link);
        const auto new_q = normalize_quat(link_node.local_rotation * delta_q);
        auto euler = ran::to_euler(new_q);
        euler.x = ran::clamp(euler.x, ran::min(link.min_angle.x, link.max_angle.x),
                             ran::max(link.min_angle.x, link.max_angle.x));
        euler.y = ran::clamp(euler.y, ran::min(link.min_angle.y, link.max_angle.y),
                             ran::max(link.min_angle.y, link.max_angle.y));
        euler.z = ran::clamp(euler.z, ran::min(link.min_angle.z, link.max_angle.z),
                             ran::max(link.min_angle.z, link.max_angle.z));
        link_node.local_rotation = ran::to_quat(euler);
      } else {
        const auto axis_link =
          ran::normalize(ran::conjugate(link_node.world_rotation) * norm_axis_world);
        const f32 step_angle = ran::min(raw_angle, max_step_angle);
        const auto delta_q = ran::to_quat(step_angle, axis_link);
        link_node.local_rotation = normalize_quat(link_node.local_rotation * delta_q);
      }

      update_chain_transforms(link_idx);
    }
  }
}

} // namespace mikugl
