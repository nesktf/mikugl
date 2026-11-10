#include "ui/ui_manager.hpp"

#include "scene/scene.hpp"

#include <fmt/format.h>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <string>

namespace mikugl {

namespace {

struct CategoryDef {
  const char* label;
  MorphCategory category;
};

constexpr std::array<CategoryDef, 4> categories = {{
  {"Eyebrows", MorphCategory::Eyebrow},
  {"Eyes", MorphCategory::Eye},
  {"Mouth", MorphCategory::Mouth},
  {"Other", MorphCategory::Other},
}};

bool matches_filter(std::string_view text, std::string_view query) noexcept {
  if (query.empty()) {
    return true;
  }
  auto it =
    std::search(text.begin(), text.end(), query.begin(), query.end(), [](char ch1, char ch2) {
      return std::tolower(static_cast<unsigned char>(ch1)) ==
             std::tolower(static_cast<unsigned char>(ch2));
    });
  return it != text.end();
}

void draw_morph_category(MorphManager& morph_manager, MorphCategory category,
                         std::string_view filter) noexcept {
  if (ImGui::Button("Reset Category")) {
    morph_manager.reset_category(category);
  }
  ImGui::Separator();

  const DeferFn end_child = ImGui::EndChild;
  if (ImGui::BeginChild("MorphList", ImVec2(0, 0), false)) {
    const auto morphs = morph_manager.morphs();
    for (u32 i = 0; i < morphs.size(); ++i) {
      const auto& m = morphs[i];
      if (m.category != category) {
        continue;
      }

      std::string display_name = m.name_jp;
      if (!m.name_en.empty()) {
        if (!display_name.empty()) {
          display_name += " (" + m.name_en + ")";
        } else {
          display_name = m.name_en;
        }
      }
      if (display_name.empty()) {
        display_name = fmt::format("Morph_{}", i);
      }

      if (!matches_filter(display_name, filter)) {
        continue;
      }

      ImGui::PushID(static_cast<int>(i));

      f32 w = morph_manager.weight(i);
      const bool is_active = (ran::abs(w) > 1e-4f);

      if (ImGui::SmallButton("R")) {
        morph_manager.set_weight(i, 0.0f);
      }
      ImGui::SameLine();

      if (is_active) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.85f, 1.0f, 1.0f));
      }

      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.45f);
      if (ImGui::SliderFloat("##weight", &w, 0.0f, 1.0f, "%.2f")) {
        morph_manager.set_weight(i, w);
      }
      ImGui::SameLine();
      ImGui::TextUnformatted(display_name.c_str());

      if (is_active) {
        ImGui::PopStyleColor();
      }

      ImGui::PopID();
    }
  }
}

void do_draw(MorphManager& morph_manager, bool* p_open) {
  const DeferFn end = ImGui::End;
  if (!ImGui::Begin("Facial Expressions", p_open)) {
    return;
  }

  static char filter_buf[64] = "";
  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 90.0f);
  ImGui::InputTextWithHint("##MorphFilter", "Search expressions...", filter_buf,
                           sizeof(filter_buf));
  ImGui::SameLine();
  if (ImGui::Button("Reset All", ImVec2(80.0f, 0))) {
    morph_manager.reset_all();
  }

  ImGui::Spacing();

  if (ImGui::BeginTabBar("MorphCategoryTabs", ImGuiTabBarFlags_None)) {
    for (const auto& cat : categories) {
      if (ImGui::BeginTabItem(cat.label)) {
        draw_morph_category(morph_manager, cat.category, filter_buf);
        ImGui::EndTabItem();
      }
    }
    ImGui::EndTabBar();
  }
}

} // namespace

void UiManager::draw_morph_panel(Scene& scene, bool* p_open) {
  auto* model = scene.selected_model();
  if (model && model->morph_manager) {
    do_draw(*model->morph_manager, p_open);
    return;
  }

  if (ImGui::Begin("Facial Expressions", p_open)) {
    ImGui::TextDisabled("No model selected. Select a model in Scene Nodes to edit expressions.");
  }
  ImGui::End();
}

} // namespace mikugl
