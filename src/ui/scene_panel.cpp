#include "ui/ui_manager.hpp"

#include "scene/scene.hpp"

#include <imgui.h>

namespace mikugl {

bool UiManager::draw_scene_panel(Scene& scene, bool* open) {
  const DeferFn def = ImGui::End;
  bool request_model_modal = false;

  if (!ImGui::Begin("Scene Nodes", open)) {
    return false;
  }

  ImGui::SeparatorText("Models");
  if (ImGui::Button("+ Load Model...", ImVec2(-1.0f, 0.0f))) {
    request_model_modal = true;
  }

  auto models = scene.models();
  if (models.empty()) {
    ImGui::TextDisabled("No models loaded. Use '+ Load Model...' to add a PMD or PMX file.");
  } else {
    i32 model_to_remove = -1;
    for (u32 i = 0; i < models.size(); ++i) {
      auto& mdl = models[i];
      ImGui::PushID(static_cast<int>(i));

      ImGui::Checkbox("##mdl_vis", &mdl.visible);
      ImGui::SameLine();

      const bool is_selected = (scene.selected_node_type() == SceneNodeType::Model &&
                                scene.selected_model_index() == static_cast<i32>(i));
      const bool is_active_model = (scene.selected_model_index() == static_cast<i32>(i));

      ImGuiTreeNodeFlags node_flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
        ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowOverlap;
      if (is_selected) {
        node_flags |= ImGuiTreeNodeFlags_Selected;
      }

      const auto label =
        mdl.name + (is_active_model ? " [Active]" : "") + "###model_node_" + std::to_string(i);
      const bool node_open = ImGui::TreeNodeEx(label.c_str(), node_flags);
      if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        scene.select_model(static_cast<i32>(i));
      }

      ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 26.0f);
      if (ImGui::SmallButton("X")) {
        model_to_remove = static_cast<i32>(i);
      }

      if (node_open) {
        ImGui::TextDisabled("Vertices: %u | Indices: %u", mdl.gl_mesh.vertex_count(),
                            mdl.gl_mesh.index_count());
        if (ImGui::SmallButton("Show All Meshes")) {
          mdl.set_all_submeshes_visible(true);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Hide All Meshes")) {
          mdl.set_all_submeshes_visible(false);
        }

        if (ImGui::TreeNode("Submeshes")) {
          for (size_t sm_idx = 0; sm_idx < mdl.submeshes.size(); ++sm_idx) {
            auto& sm = mdl.submeshes[sm_idx];
            ImGui::PushID(static_cast<int>(sm_idx));
            ImGui::Checkbox("##sm_vis", &sm.visible);
            ImGui::SameLine();
            const auto sm_name = sm.name.empty() ? ("Submesh " + std::to_string(sm_idx)) : sm.name;
            ImGui::Text("[%zu] %s (%u idx)", sm_idx, sm_name.c_str(), sm.index_count);
            ImGui::PopID();
          }
          ImGui::TreePop();
        }
        ImGui::TreePop();
      }

      ImGui::PopID();
    }

    if (model_to_remove >= 0) {
      scene.remove_model(static_cast<u32>(model_to_remove));
    }
  }

  ImGui::Spacing();
  ImGui::SeparatorText("Environment & Camera");

  {
    const bool cam_sel = (scene.selected_node_type() == SceneNodeType::Camera);
    if (ImGui::Selectable("Viewport Camera", cam_sel)) {
      scene.select_node(SceneNodeType::Camera);
    }
  }

  {
    auto& dir_light = scene.lights().directional_light();
    ImGui::Checkbox("##dir_light_en", &dir_light.enabled);
    ImGui::SameLine();
    const bool dl_sel = (scene.selected_node_type() == SceneNodeType::DirectionalLight);
    if (ImGui::Selectable("Directional Light", dl_sel)) {
      scene.select_node(SceneNodeType::DirectionalLight);
    }
  }

  {
    bool grid_vis = scene.is_grid_visible();
    if (ImGui::Checkbox("##grid_vis", &grid_vis)) {
      scene.set_grid_visible(grid_vis);
    }
    ImGui::SameLine();
    const bool grid_sel = (scene.selected_node_type() == SceneNodeType::GroundGrid);
    if (ImGui::Selectable("Ground Grid", grid_sel)) {
      scene.select_node(SceneNodeType::GroundGrid);
    }
  }

  ImGui::Spacing();
  ImGui::SeparatorText("Point Lights");

  auto& light_mgr = scene.lights();
  const bool can_add_light = (light_mgr.point_light_count() < LightManager::MAX_POINT_LIGHTS);
  if (!can_add_light) {
    ImGui::BeginDisabled();
  }
  if (ImGui::Button("+ Add Point Light", ImVec2(-1.0f, 0.0f))) {
    const u32 new_idx = light_mgr.add_default_point_light();
    scene.select_node(SceneNodeType::PointLight, static_cast<i32>(new_idx));
  }
  if (!can_add_light) {
    ImGui::EndDisabled();
  }

  auto pt_lights = light_mgr.point_lights();
  if (pt_lights.empty()) {
    ImGui::TextDisabled("No point lights. Click '+ Add Point Light' to illuminate the scene.");
  } else {
    i32 light_to_remove = -1;
    for (u32 i = 0; i < pt_lights.size(); ++i) {
      auto& pl = pt_lights[i];
      ImGui::PushID(static_cast<int>(1000 + i));

      ImGui::Checkbox("##pl_en", &pl.enabled);
      ImGui::SameLine();

      const ImVec4 col{pl.color.x, pl.color.y, pl.color.z, 1.0f};
      ImGui::ColorButton("##pl_col", col,
                         ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
                         ImVec2(14.0f, 14.0f));
      ImGui::SameLine();

      const bool is_sel = (scene.selected_node_type() == SceneNodeType::PointLight &&
                           scene.selected_point_light_index() == static_cast<i32>(i));
      if (ImGui::Selectable(pl.name.c_str(), is_sel, ImGuiSelectableFlags_AllowOverlap)) {
        scene.select_node(SceneNodeType::PointLight, static_cast<i32>(i));
      }

      ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 26.0f);
      if (ImGui::SmallButton("X")) {
        light_to_remove = static_cast<i32>(i);
      }

      ImGui::PopID();
    }

    if (light_to_remove >= 0) {
      light_mgr.remove_point_light(static_cast<u32>(light_to_remove));
      if (scene.selected_node_type() == SceneNodeType::PointLight) {
        if (light_mgr.point_light_count() == 0) {
          scene.select_node(SceneNodeType::None);
        } else if (scene.selected_point_light_index() >=
                   static_cast<i32>(light_mgr.point_light_count())) {
          scene.select_node(SceneNodeType::PointLight,
                            static_cast<i32>(light_mgr.point_light_count()) - 1);
        }
      }
    }
  }

  return request_model_modal;
}

} // namespace mikugl
