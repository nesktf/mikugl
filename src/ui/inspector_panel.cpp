#include "ui/ui_manager.hpp"

#include "scene/scene.hpp"

#include <imgui.h>

#include <fmt/format.h>

#include <cstdio>
#include <string>

namespace mikugl {

namespace {

enum class CoordSpace : u8 {
  Local = 0,
  World = 1,
};

enum class RotMode : u8 {
  Euler = 0,
  Quaternion = 1,
};

ran::Vec3f32 quat_to_euler_deg(const ran::Quatf32& q) noexcept {
  const ran::Vec3f32 rad_euler = ran::to_euler(q);
  return ran::Vec3f32{ran::deg(rad_euler.x), ran::deg(rad_euler.y), ran::deg(rad_euler.z)};
}

ran::Quatf32 euler_deg_to_quat(const ran::Vec3f32& deg_euler) noexcept {
  const ran::Vec3f32 rad_euler{ran::rad(deg_euler.x), ran::rad(deg_euler.y),
                               ran::rad(deg_euler.z)};
  return ran::normalize(ran::to_quat(rad_euler));
}

bool draw_rotation_control(const char* label_prefix, ran::Quatf32& quat,
                           RotMode rot_mode) noexcept {
  bool changed = false;
  if (rot_mode == RotMode::Euler) {
    ran::Vec3f32 euler_deg = quat_to_euler_deg(quat);
    const auto label = fmt::format("{} (deg)", label_prefix);
    if (ImGui::DragFloat3(label.c_str(), euler_deg.data(), 0.5f, -180.0f, 180.0f, "%.1f")) {
      quat = euler_deg_to_quat(euler_deg);
      changed = true;
    }
  } else {
    f32 q_vals[4] = {quat.w, quat.x, quat.y, quat.z};
    const auto label = fmt::format("{} (wxyz)", label_prefix);
    if (ImGui::DragFloat4(label.c_str(), q_vals, 0.01f, -1.0f, 1.0f, "%.3f")) {
      quat = ran::normalize(ran::Quatf32{q_vals[0], q_vals[1], q_vals[2], q_vals[3]});
      changed = true;
    }
  }
  return changed;
}

void draw_bone_inspector(ModelInstance& model, RotMode& rot_mode, CoordSpace& coord_space) {
  if (!model.skeleton || model.selected_bone_idx < 0 ||
      static_cast<u32>(model.selected_bone_idx) >= model.skeleton->bone_count()) {
    return;
  }

  auto& skeleton = *model.skeleton;
  const u32 bone_idx = static_cast<u32>(model.selected_bone_idx);
  auto& bn = skeleton.node(bone_idx);

  ImGui::SeparatorText("Bone Transform");

  const auto bone_title = format_bone_label(bn, bone_idx);
  ImGui::Text("Bone: %s [idx: %u]", bone_title.c_str(), bone_idx);
  if (bn.parent_index >= 0 && static_cast<u32>(bn.parent_index) < skeleton.bone_count()) {
    const auto& parent = skeleton.node(static_cast<u32>(bn.parent_index));
    const auto& p_name = !parent.name_en.empty() ? parent.name_en : parent.name_jp;
    ImGui::TextDisabled("Parent: %s [idx: %d]", p_name.c_str(), bn.parent_index);
  } else {
    ImGui::TextDisabled("Parent: Root");
  }

  ImGui::TextUnformatted("Space:");
  ImGui::SameLine();
  if (ImGui::RadioButton("Local##bspace", coord_space == CoordSpace::Local)) {
    coord_space = CoordSpace::Local;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("World##bspace", coord_space == CoordSpace::World)) {
    coord_space = CoordSpace::World;
  }

  ImGui::TextUnformatted("Rotation Mode:");
  ImGui::SameLine();
  if (ImGui::RadioButton("Euler##brot", rot_mode == RotMode::Euler)) {
    rot_mode = RotMode::Euler;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Quat##brot", rot_mode == RotMode::Quaternion)) {
    rot_mode = RotMode::Quaternion;
  }

  ImGui::PushID("BoneTransformControls");
  bool pose_dirty = false;

  if (coord_space == CoordSpace::Local) {
    if (ImGui::DragFloat3("Position", bn.local_translation.data(), 0.02f, -500.0f, 500.0f,
                          "%.3f")) {
      pose_dirty = true;
    }
    if (draw_rotation_control("Rotation", bn.local_rotation, rot_mode)) {
      pose_dirty = true;
    }
  } else {
    ran::Vec3f32 world_pos = bn.world_position;
    if (ImGui::DragFloat3("Position (World)", world_pos.data(), 0.02f, -500.0f, 500.0f, "%.3f")) {
      skeleton.set_node_world_position(bone_idx, world_pos);
      pose_dirty = true;
    }
    ran::Quatf32 world_rot = bn.world_rotation;
    if (draw_rotation_control("Rotation (World)", world_rot, rot_mode)) {
      skeleton.set_node_world_rotation(bone_idx, world_rot);
      pose_dirty = true;
    }
  }

  if (ImGui::DragFloat3("Scale", bn.local_scale.data(), 0.01f, 0.01f, 100.0f, "%.3f")) {
    pose_dirty = true;
  }

  if (pose_dirty) {
    skeleton.update_pose();
  }

  if (ImGui::Button("Reset Bone")) {
    skeleton.reset_node_pose(bone_idx);
  }
  ImGui::SameLine();
  ImGui::Checkbox("Bone Visible", &bn.visible);
  ImGui::PopID();
}

void draw_model_inspector(ModelInstance& model, RotMode& rot_mode) {
  ImGui::SeparatorText("Model Transform");
  ImGui::Text("Name: %s", model.name.c_str());
  if (!model.path.empty()) {
    ImGui::TextDisabled("Path: %s", model.path.c_str());
  }
  ImGui::Checkbox("Model Visible", &model.visible);

  ImGui::PushID("ModelTransformControls");
  ImGui::DragFloat3("Position", model.position.data(), 0.05f, -1000.0f, 1000.0f, "%.2f");
  draw_rotation_control("Rotation", model.rotation, rot_mode);
  ImGui::DragFloat3("Scale", model.scale.data(), 0.01f, 0.01f, 100.0f, "%.2f");
  if (ImGui::Button("Reset Model Transform")) {
    model.reset_transform();
  }
  ImGui::PopID();
}

void draw_camera_inspector(MmdCamera& camera) {
  ImGui::SeparatorText("Viewport Camera");
  ran::Vec3f32 target = camera.target();
  if (ImGui::DragFloat3("Target", target.data(), 0.05f, -1000.0f, 1000.0f, "%.2f")) {
    camera.set_target(target);
  }
  f32 dist = camera.distance();
  if (ImGui::DragFloat("Distance", &dist, 0.1f, 0.5f, 2000.0f, "%.2f")) {
    camera.set_distance(dist);
  }
  f32 yaw_deg = ran::deg(camera.yaw());
  if (ImGui::DragFloat("Yaw (deg)", &yaw_deg, 0.5f, -360.0f, 360.0f, "%.1f")) {
    camera.set_yaw(ran::rad(yaw_deg));
  }
  f32 pitch_deg = ran::deg(camera.pitch());
  if (ImGui::DragFloat("Pitch (deg)", &pitch_deg, 0.5f, -88.0f, 88.0f, "%.1f")) {
    camera.set_pitch(ran::rad(pitch_deg));
  }
  f32 fov_deg = ran::deg(camera.fov());
  if (ImGui::SliderFloat("FOV (deg)", &fov_deg, 15.0f, 140.0f, "%.1f")) {
    camera.set_fov(ran::rad(fov_deg));
  }
  if (ImGui::Button("Reset Camera")) {
    camera.reset();
  }
}

void draw_directional_light_inspector(LightManager::DirectionalLight& dir_light) {
  ImGui::SeparatorText("Directional Light");
  ImGui::Checkbox("Enabled##dir_l", &dir_light.enabled);
  ImGui::ColorEdit3("Light Color", dir_light.color.data());
  ImGui::DragFloat3("Light Direction", dir_light.direction.data(), 0.01f, -1.0f, 1.0f, "%.2f");
  if (ImGui::Button("Reset Light")) {
    dir_light = LightManager::DirectionalLight{};
  }
}

void draw_point_light_inspector(LightManager::PointLight& pt_light) {
  ImGui::SeparatorText("Point Light");
  char name_buf[64];
  std::snprintf(name_buf, sizeof(name_buf), "%s", pt_light.name.c_str());
  if (ImGui::InputText("Name", name_buf, sizeof(name_buf))) {
    pt_light.name = name_buf;
  }
  ImGui::Checkbox("Enabled##pt_l", &pt_light.enabled);
  ImGui::DragFloat3("Position", pt_light.position.data(), 0.1f, -500.0f, 500.0f, "%.2f");
  ImGui::ColorEdit3("Color", pt_light.color.data());
  ImGui::DragFloat("Radius", &pt_light.radius, 0.2f, 0.1f, 500.0f, "%.1f");
  ImGui::DragFloat("Intensity", &pt_light.intensity, 0.05f, 0.0f, 50.0f, "%.2f");
}

} // namespace

void UiManager::draw_inspector_panel(Scene& scene, bool* open) {
  const DeferFn end = ImGui::End;
  if (!ImGui::Begin("Inspector", open)) {
    return;
  }

  static CoordSpace s_coord_space = CoordSpace::Local;
  static RotMode s_rot_mode = RotMode::Euler;

  const auto node_type = scene.selected_node_type();
  auto* active_model = scene.selected_model();

  bool drew_anything = false;
  if (node_type == SceneNodeType::Model && active_model) {
    draw_model_inspector(*active_model, s_rot_mode);
    drew_anything = true;
  } else if (node_type == SceneNodeType::Camera) {
    draw_camera_inspector(scene.camera());
    drew_anything = true;
  } else if (node_type == SceneNodeType::DirectionalLight) {
    draw_directional_light_inspector(scene.lights().directional_light());
    drew_anything = true;
  } else if (node_type == SceneNodeType::PointLight) {
    const i32 pl_idx = scene.selected_point_light_index();
    auto pt_lights = scene.lights().point_lights();
    if (pl_idx >= 0 && static_cast<size_t>(pl_idx) < pt_lights.size()) {
      draw_point_light_inspector(pt_lights[static_cast<size_t>(pl_idx)]);
      drew_anything = true;
    }
  } else if (node_type == SceneNodeType::GroundGrid) {
    ImGui::SeparatorText("Ground Grid");
    bool grid_vis = scene.is_grid_visible();
    if (ImGui::Checkbox("Grid Visible", &grid_vis)) {
      scene.set_grid_visible(grid_vis);
    }
    drew_anything = true;
  }

  if (active_model && active_model->skeleton && active_model->selected_bone_idx >= 0) {
    if (drew_anything) {
      ImGui::Spacing();
    }
    draw_bone_inspector(*active_model, s_rot_mode, s_coord_space);
    drew_anything = true;
  }

  if (!drew_anything) {
    ImGui::TextDisabled(
      "Select a scene node or bone to inspect and edit its transform or lighting properties.");
  }
}

} // namespace mikugl
