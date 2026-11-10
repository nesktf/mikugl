#include "ui/ui_manager.hpp"

#include "scene/scene.hpp"

#include <fmt/format.h>
#include <imgui.h>

namespace mikugl {

namespace {

bool matches_bone_filter(std::string_view text, std::string_view query) noexcept {
  if (query.empty()) {
    return true;
  }
  const auto it =
    std::search(text.begin(), text.end(), query.begin(), query.end(), [](char ch1, char ch2) {
      return std::tolower(static_cast<unsigned char>(ch1)) ==
             std::tolower(static_cast<unsigned char>(ch2));
    });
  return it != text.end();
}

void draw_bone_tree_node(ModelInstance& model, Span<Skeleton::BoneNode> bones,
                         const Vec<Vec<u32>>& children, u32 bone_idx) {
  auto& b = bones[bone_idx];
  ImGui::PushID(static_cast<int>(bone_idx));

  ImGui::Checkbox("##bvis", &b.visible);
  ImGui::SameLine();

  const bool is_selected = (model.selected_bone_idx == static_cast<i32>(bone_idx));
  const bool is_hovered = (model.hovered_bone_idx == static_cast<i32>(bone_idx));
  const bool has_children = !children[bone_idx].empty();

  ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                             ImGuiTreeNodeFlags_OpenOnDoubleClick |
                             ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;
  if (is_selected) {
    flags |= ImGuiTreeNodeFlags_Selected;
  }
  if (!has_children) {
    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
  }

  if (is_hovered && !is_selected) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.60f, 0.25f, 1.0f));
  }

  const auto label = format_bone_label(b, bone_idx);
  const bool open = ImGui::TreeNodeEx(label.c_str(), flags);

  if (is_hovered && !is_selected) {
    ImGui::PopStyleColor();
  }

  if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
    model.selected_bone_idx = static_cast<i32>(bone_idx);
  }
  if (ImGui::IsItemHovered()) {
    model.hovered_bone_idx = static_cast<i32>(bone_idx);
  }

  if (has_children && open) {
    for (const u32 child_idx : children[bone_idx]) {
      draw_bone_tree_node(model, bones, children, child_idx);
    }
    ImGui::TreePop();
  }

  ImGui::PopID();
}

} // namespace

void UiManager::draw_bone_panel(Scene& scene, bool* open) {
  const DeferFn def = ImGui::End;

  if (!ImGui::Begin("Bone Hierarchy", open)) {
    return;
  }

  bool posing_mode = scene.is_posing_mode();
  if (ImGui::Checkbox("Posing Mode (Tab)", &posing_mode)) {
    scene.set_posing_mode(posing_mode);
  }
  ImGui::Separator();

  auto* model = scene.selected_model();
  if (!model) {
    ImGui::TextDisabled("No model selected. Select a model in Scene Nodes to view its bones.");
    return;
  }

  if (!model->skeleton) {
    ImGui::TextDisabled("Selected model '%s' has no skeleton.", model->name.c_str());
    return;
  }

  auto& skeleton = *model->skeleton;
  ImGui::Text("Model: %s (%u bones)", model->name.c_str(), skeleton.bone_count());

  bool ik_active = skeleton.is_ik_enabled();
  if (ImGui::Checkbox("Enable IK", &ik_active)) {
    skeleton.set_ik_enabled(ik_active);
    skeleton.update_pose();
  }
  ImGui::SameLine();
  if (ImGui::Button("Reset Pose")) {
    skeleton.reset_pose();
    if (model->morph_manager) {
      model->morph_manager->reset_all();
    }
  }

  if (ImGui::SmallButton("Show All Bones")) {
    skeleton.set_all_bones_visible(true);
  }
  ImGui::SameLine();
  if (ImGui::SmallButton("Hide All Bones")) {
    skeleton.set_all_bones_visible(false);
  }
  if (model->selected_bone_idx >= 0) {
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear Selection")) {
      model->selected_bone_idx = -1;
    }
  }

  static char search_buf[64] = "";
  ImGui::SetNextItemWidth(-1.0f);
  ImGui::InputTextWithHint("##BoneSearch", "Search bones (EN/JP)...", search_buf,
                           sizeof(search_buf));

  ImGui::Separator();
  if (ImGui::BeginChild("BoneTreeChild", ImVec2(0.0f, 0.0f), true)) {
    auto bones = skeleton.bones();
    const std::string_view query(search_buf);

    if (!query.empty()) {
      for (u32 i = 0; i < bones.size(); ++i) {
        auto& b = bones[i];
        const auto label = format_bone_label(b, i);
        if (!matches_bone_filter(label, query)) {
          continue;
        }
        ImGui::PushID(static_cast<int>(i));
        ImGui::Checkbox("##bvis_f", &b.visible);
        ImGui::SameLine();
        const bool is_sel = (model->selected_bone_idx == static_cast<i32>(i));
        if (ImGui::Selectable(label.c_str(), is_sel)) {
          model->selected_bone_idx = static_cast<i32>(i);
        }
        if (ImGui::IsItemHovered()) {
          model->hovered_bone_idx = static_cast<i32>(i);
        }
        ImGui::PopID();
      }
    } else {
      Vec<u32> roots;
      Vec<Vec<u32>> children(bones.size());
      for (u32 i = 0; i < bones.size(); ++i) {
        const i32 p = bones[i].parent_index;
        if (p < 0 || static_cast<size_t>(p) >= bones.size()) {
          roots.push_back(i);
        } else {
          children[static_cast<size_t>(p)].push_back(i);
        }
      }

      for (const u32 r : roots) {
        draw_bone_tree_node(*model, bones, children, r);
      }
    }
  }
  ImGui::EndChild();
}

} // namespace mikugl
