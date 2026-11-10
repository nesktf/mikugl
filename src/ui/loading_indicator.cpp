#include "ui/ui_manager.hpp"

#include <cmath>
#include <imgui.h>
#include <imgui_internal.h>

namespace mikugl {

namespace {

void draw_spinner(f32 radius, f32 thickness, ImU32 color) {
  auto* window = ImGui::GetCurrentWindow();
  if (window->SkipItems) {
    return;
  }

  const auto& g = *GImGui;
  const auto pos = window->DC.CursorPos;
  const ImVec2 size{radius * 2.0f, (radius + thickness) * 2.0f};
  const ImRect bb{pos, ImVec2(pos.x + size.x, pos.y + size.y)};
  ImGui::ItemSize(bb);
  if (!ImGui::ItemAdd(bb, window->GetID("spinner"))) {
    return;
  }

  static constexpr i32 num_segments = 30;
  static constexpr f32 a_min = 0.2f;
  static constexpr f32 a_max = 5.0f;
  const auto time = static_cast<f32>(g.Time);
  const f32 start = std::fmod(time * 6.0f, 6.2831853f);
  const f32 angle_span = a_min + (a_max - a_min) * (0.5f + 0.5f * std::sin(time * 3.0f));

  const ImVec2 centre{pos.x + radius, pos.y + radius + thickness};
  window->DrawList->PathClear();
  for (i32 i = 0; i < num_segments; ++i) {
    const f32 a = start + (static_cast<f32>(i) / static_cast<f32>(num_segments)) * angle_span;
    window->DrawList->PathLineTo(
      ImVec2(centre.x + std::cos(a) * radius, centre.y + std::sin(a) * radius));
  }
  window->DrawList->PathStroke(color, 0, thickness);
}

} // namespace

void UiManager::draw_loading_indicator(bool is_loading, std::string_view path) {
  if (!is_loading) {
    return;
  }

  const auto* viewport = ImGui::GetMainViewport();
  const auto center = viewport->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(380.0f, 0.0f));

  static constexpr ImGuiWindowFlags flags =
    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;

  const DeferFn end = ImGui::End;
  if (ImGui::Begin("##ModelLoadingIndicator", nullptr, flags)) {
    ImGui::Spacing();
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 36.0f) * 0.5f);
    draw_spinner(16.0f, 3.5f, IM_COL32(0, 180, 216, 255));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const char* title = "Loading Model...";
    const f32 title_width = ImGui::CalcTextSize(title).x;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - title_width) * 0.5f);
    ImGui::TextUnformatted(title);

    ImGui::Spacing();
    const std::string path_str(path);
    const f32 path_width = ImGui::CalcTextSize(path_str.c_str()).x;
    if (path_width < ImGui::GetWindowWidth() - 20.0f) {
      ImGui::SetCursorPosX((ImGui::GetWindowWidth() - path_width) * 0.5f);
    }
    ImGui::TextDisabled("%s", path_str.c_str());
    ImGui::Spacing();
  }
}

} // namespace mikugl
