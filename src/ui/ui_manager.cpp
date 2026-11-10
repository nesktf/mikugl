#include "ui/ui_manager.hpp"

#include "core/command_stack.hpp"
#include "model/async_loader.hpp"
#include "scene/scene.hpp"

#include <SDL2/SDL.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

#include <algorithm>
#include <cctype>
#include <cstdio>

namespace mikugl {

namespace {

bool has_model_extension(const stdfs::path& p) noexcept {
  std::string ext = p.extension().string();
  for (char& c : ext) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return ext == ".pmd" || ext == ".pmx";
}

void apply_dark_theme() {
  ImGuiStyle& style = ImGui::GetStyle();
  ImVec4* colors = style.Colors;

  style.WindowRounding = 4.0f;
  style.ChildRounding = 3.0f;
  style.FrameRounding = 3.0f;
  style.PopupRounding = 4.0f;
  style.ScrollbarRounding = 4.0f;
  style.GrabRounding = 3.0f;
  style.TabRounding = 4.0f;

  style.WindowBorderSize = 1.0f;
  style.ChildBorderSize = 1.0f;
  style.PopupBorderSize = 1.0f;
  style.FrameBorderSize = 0.0f;
  style.TabBorderSize = 0.0f;

  style.WindowPadding = ImVec2(8.0f, 8.0f);
  style.FramePadding = ImVec2(6.0f, 4.0f);
  style.ItemSpacing = ImVec2(8.0f, 6.0f);
  style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
  style.IndentSpacing = 20.0f;
  style.ScrollbarSize = 13.0f;
  style.GrabMinSize = 10.0f;

  colors[ImGuiCol_Text] = ImVec4(0.92f, 0.92f, 0.94f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.55f, 0.58f, 1.00f);
  colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.14f, 0.14f, 0.15f, 1.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.16f, 0.16f, 0.18f, 0.98f);
  colors[ImGuiCol_Border] = ImVec4(0.24f, 0.24f, 0.27f, 1.00f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.24f, 0.27f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.11f, 0.75f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.15f, 1.00f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.12f, 0.12f, 0.13f, 0.60f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.35f, 0.40f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.42f, 0.42f, 0.48f, 1.00f);
  colors[ImGuiCol_CheckMark] = ImVec4(0.38f, 0.72f, 0.96f, 1.00f);
  colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.65f, 0.88f, 1.00f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.45f, 0.75f, 0.98f, 1.00f);
  colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.23f, 1.00f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.27f, 0.27f, 0.31f, 1.00f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.33f, 0.33f, 0.38f, 1.00f);
  colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.25f, 1.00f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.28f, 0.33f, 1.00f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.34f, 0.34f, 0.40f, 1.00f);
  colors[ImGuiCol_Separator] = ImVec4(0.24f, 0.24f, 0.27f, 1.00f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.35f, 0.35f, 0.40f, 1.00f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(0.45f, 0.45f, 0.52f, 1.00f);
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.24f, 0.24f, 0.27f, 0.50f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.35f, 0.35f, 0.40f, 0.75f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.45f, 0.45f, 0.52f, 0.90f);
  colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.26f, 0.30f, 1.00f);
  colors[ImGuiCol_TabSelected] = ImVec4(0.22f, 0.22f, 0.26f, 1.00f);
  colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.38f, 0.72f, 0.96f, 1.00f);
  colors[ImGuiCol_TabDimmed] = ImVec4(0.14f, 0.14f, 0.15f, 1.00f);
  colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
  colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.28f, 0.50f, 0.70f, 1.00f);
  colors[ImGuiCol_DockingPreview] = ImVec4(0.35f, 0.65f, 0.88f, 0.40f);
  colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
  colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
  colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
  colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
  colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
  colors[ImGuiCol_TableHeaderBg] = ImVec4(0.17f, 0.17f, 0.19f, 1.00f);
  colors[ImGuiCol_TableBorderStrong] = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
  colors[ImGuiCol_TableBorderLight] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
  colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.02f);
  colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
  colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
  colors[ImGuiCol_NavHighlight] = ImVec4(0.35f, 0.65f, 0.88f, 1.00f);
  colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
  colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
}

} // namespace

UiManager::UiManager(const stdfs::path& exe_dir, SDL_Window* window, void* gl_context) :
    _exe_dir(exe_dir), _browser_dir(exe_dir), _selected_file(), _toast_message(),
    _toast_timer(0.0f), _toast_is_error(false), _open_file_modal(false), _open_help_modal(false),
    _show_scene_panel(true), _show_bone_panel(true), _show_morph_panel(true),
    _show_inspector_panel(true), _show_timeline_panel(true), _show_demo_window(false) {
  if (stdfs::exists(exe_dir / "models")) {
    _browser_dir = exe_dir / "models";
  } else if (stdfs::exists(exe_dir / "model")) {
    _browser_dir = exe_dir / "model";
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();

  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  apply_dark_theme();

  const f32 main_scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(main_scale);

  const auto find_font = [&](std::string_view font_name) -> std::string {
    const auto path = exe_dir / "data" / "fonts" / font_name;
    return stdfs::exists(path) ? path.string() : "";
  };

  const f32 font_size = 16.0f * (main_scale > 0.0f ? main_scale : 1.0f);

  if (const auto base_font = find_font("DroidSans.ttf"); !base_font.empty()) {
    io.Fonts->AddFontFromFileTTF(base_font.c_str(), font_size);
  } else {
    fprintf(stderr, "Base font not found: %s\n", base_font.c_str());
    io.Fonts->AddFontDefault();
  }

  if (const auto jp_font = find_font("DroidSansJapanese.ttf"); !jp_font.empty()) {
    ImFontConfig config;
    config.MergeMode = true;
    io.Fonts->AddFontFromFileTTF(jp_font.c_str(), font_size, &config,
                                 io.Fonts->GetGlyphRangesJapanese());
  } else {
    fprintf(stderr, "JP Font not found: %s\n", jp_font.c_str());
  }

  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init(nullptr);
}

UiManager::~UiManager() noexcept {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
}

void UiManager::process_event(const SDL_Event& event) noexcept {
  ImGui_ImplSDL2_ProcessEvent(&event);
}

bool UiManager::wants_capture_mouse() const noexcept {
  return ImGui::GetIO().WantCaptureMouse;
}

bool UiManager::wants_capture_keyboard() const noexcept {
  return ImGui::GetIO().WantCaptureKeyboard;
}

ran::Vec2f32 UiManager::display_size() const noexcept {
  const auto& sz = ImGui::GetIO().DisplaySize;
  return ran::Vec2f32{sz.x, sz.y};
}

void UiManager::open_load_model_modal() noexcept {
  _open_file_modal = true;
}

void UiManager::show_toast(std::string_view message, bool is_error) noexcept {
  _toast_message = std::string(message);
  _toast_is_error = is_error;
  _toast_timer = 3.5f;
}

auto UiManager::draw(Scene& scene, CommandStack& command_stack, AsyncLoader& async_loader)
  -> FrameActions {
  FrameActions actions{};
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  const auto dockspace_id = ImGui::GetID("MikuGLDockSpace");
  const auto* main_viewport = ImGui::GetMainViewport();
  ImGui::DockSpaceOverViewport(dockspace_id, main_viewport,
                               ImGuiDockNodeFlags_PassthruCentralNode);

  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Load Model...", "Ctrl+O")) {
        _open_file_modal = true;
      }
      const bool has_selected_model = (scene.selected_model() != nullptr);
      if (ImGui::MenuItem("Reset Pose", nullptr, false, has_selected_model)) {
        if (auto* mdl = scene.selected_model()) {
          if (mdl->skeleton) {
            mdl->skeleton->reset_pose();
          }
          if (mdl->morph_manager) {
            mdl->morph_manager->reset_all();
          }
        }
      }
      if (ImGui::MenuItem("Take Screenshot", "F2")) {
        actions.request_screenshot = true;
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Exit", "Alt+F4")) {
        actions.request_exit = true;
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::MenuItem("Undo", "Ctrl+Z", false, command_stack.can_undo())) {
        const auto res = command_stack.undo();
        MIKU_UNUSED(res);
      }
      if (ImGui::MenuItem("Redo", "Ctrl+Y", false, command_stack.can_redo())) {
        const auto res = command_stack.redo();
        MIKU_UNUSED(res);
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Clear History")) {
        command_stack.clear();
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
      bool posing_mode = scene.is_posing_mode();
      if (ImGui::MenuItem("Posing Mode", "Tab", &posing_mode)) {
        scene.set_posing_mode(posing_mode);
      }
      if (ImGui::MenuItem("Reset Camera", "Home")) {
        scene.camera().reset();
      }
      ImGui::Separator();
      ImGui::MenuItem("Scene Nodes", nullptr, &_show_scene_panel);
      ImGui::MenuItem("Bone Hierarchy", nullptr, &_show_bone_panel);
      ImGui::MenuItem("Facial Expressions", nullptr, &_show_morph_panel);
      ImGui::MenuItem("Inspector", nullptr, &_show_inspector_panel);
      ImGui::MenuItem("Timeline", nullptr, &_show_timeline_panel);
      ImGui::Separator();
      ImGui::MenuItem("ImGui Demo Window", nullptr, &_show_demo_window);
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
      if (ImGui::MenuItem("Controls & Shortcuts")) {
        _open_help_modal = true;
      }
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }

  if (_show_scene_panel) {
    if (draw_scene_panel(scene, &_show_scene_panel)) {
      _open_file_modal = true;
    }
  }

  if (_show_bone_panel) {
    draw_bone_panel(scene, &_show_bone_panel);
  }

  if (_show_inspector_panel) {
    draw_inspector_panel(scene, &_show_inspector_panel);
  }

  if (_show_morph_panel) {
    draw_morph_panel(scene, &_show_morph_panel);
  }

  if (_show_timeline_panel) {
    if (ImGui::Begin("Timeline", &_show_timeline_panel)) {
      ImGui::TextDisabled("Animation timeline");
      // TODO: Playback controls and keyframe tracks
    }
    ImGui::End();
  }

  if (_show_demo_window) {
    ImGui::ShowDemoWindow(&_show_demo_window);
  }

  _draw_file_browser_modal(async_loader);
  _draw_help_modal();
  _draw_toast();

  draw_loading_indicator(async_loader.is_loading(), async_loader.current_path());

  ImGui::Render();
  return actions;
}

void UiManager::render_draw_data() const noexcept {
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UiManager::_draw_file_browser_modal(AsyncLoader& async_loader) {
  if (_open_file_modal) {
    ImGui::OpenPopup("Load PMD/PMX Model");
    _open_file_modal = false;
  }

  const auto* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(560.0f, 400.0f), ImGuiCond_Appearing);

  if (ImGui::BeginPopupModal("Load PMD/PMX Model", nullptr, ImGuiWindowFlags_None)) {
    if (ImGui::Button("^ Up")) {
      if (_browser_dir.has_parent_path()) {
        _browser_dir = _browser_dir.parent_path();
        _selected_file.clear();
      }
    }
    ImGui::SameLine();

    char dir_buf[512];
    std::snprintf(dir_buf, sizeof(dir_buf), "%s", _browser_dir.string().c_str());
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::InputText("##DirPath", dir_buf, sizeof(dir_buf),
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
      std::error_code ec;
      const stdfs::path candidate(dir_buf);
      if (stdfs::is_directory(candidate, ec)) {
        _browser_dir = candidate;
        _selected_file.clear();
      } else if (stdfs::is_regular_file(candidate, ec) && has_model_extension(candidate)) {
        async_loader.request_load(candidate.string());
        ImGui::CloseCurrentPopup();
      }
    }

    ImGui::Separator();

    if (ImGui::BeginChild("FileBrowserList", ImVec2(0.0f, -38.0f), true)) {
      std::error_code ec;
      Vec<stdfs::path> dirs;
      Vec<stdfs::path> files;

      for (const auto& entry : stdfs::directory_iterator(_browser_dir, ec)) {
        if (entry.is_directory(ec)) {
          dirs.push_back(entry.path());
        } else if (entry.is_regular_file(ec) && has_model_extension(entry.path())) {
          files.push_back(entry.path());
        }
      }

      std::sort(dirs.begin(), dirs.end());
      std::sort(files.begin(), files.end());

      for (const auto& d : dirs) {
        const std::string label = "[DIR] " + d.filename().string();
        if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
          if (ImGui::IsMouseDoubleClicked(0)) {
            _browser_dir = d;
            _selected_file.clear();
          }
        }
      }

      for (const auto& f : files) {
        const std::string full_str = f.string();
        const std::string label = f.filename().string();
        const bool is_sel = (_selected_file == full_str);
        if (ImGui::Selectable(label.c_str(), is_sel, ImGuiSelectableFlags_AllowDoubleClick)) {
          _selected_file = full_str;
          if (ImGui::IsMouseDoubleClicked(0)) {
            async_loader.request_load(_selected_file);
            ImGui::CloseCurrentPopup();
          }
        }
      }

      if (dirs.empty() && files.empty()) {
        ImGui::TextDisabled("No subdirectories or .pmd/.pmx models in this folder.");
      }
    }
    ImGui::EndChild();

    const bool can_load = !_selected_file.empty();
    if (!can_load) {
      ImGui::BeginDisabled();
    }
    if (ImGui::Button("Load Selected", ImVec2(130.0f, 0.0f))) {
      async_loader.request_load(_selected_file);
      ImGui::CloseCurrentPopup();
    }
    if (!can_load) {
      ImGui::EndDisabled();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(100.0f, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

void UiManager::_draw_help_modal() {
  if (_open_help_modal) {
    ImGui::OpenPopup("Controls & Shortcuts");
    _open_help_modal = false;
  }

  const auto* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

  if (ImGui::BeginPopupModal("Controls & Shortcuts", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::SeparatorText("Viewport Navigation");
    ImGui::BulletText("Right Mouse Drag: Orbit camera around target");
    ImGui::BulletText("Middle Mouse Drag (or Shift+Left Drag): Pan camera");
    ImGui::BulletText("Mouse Wheel: Zoom in / out");
    ImGui::BulletText("Home: Reset camera framing");

    ImGui::SeparatorText("Posing & Scene");
    ImGui::BulletText("Tab: Toggle Posing Mode (show/pick bones in viewport)");
    ImGui::BulletText("Left Click (in Posing Mode): Select hovered bone");
    ImGui::BulletText("Ctrl+O: Open Load Model dialog");
    ImGui::BulletText("F2: Save clean viewport screenshot to screenshots/");
    ImGui::BulletText("Ctrl+Z / Ctrl+Y: Undo / Redo");

    ImGui::Spacing();
    if (ImGui::Button("Close", ImVec2(120.0f, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void UiManager::_draw_toast() noexcept {
  if (_toast_timer <= 0.0f || _toast_message.empty()) {
    return;
  }
  const auto& io = ImGui::GetIO();
  _toast_timer -= io.DeltaTime;

  const auto* viewport = ImGui::GetMainViewport();
  const ImVec2 pos{viewport->WorkPos.x + viewport->WorkSize.x - 16.0f,
                   viewport->WorkPos.y + viewport->WorkSize.y - 16.0f};
  ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(1.0f, 1.0f));

  static constexpr ImGuiWindowFlags flags =
    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
    ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

  if (ImGui::Begin("##StatusToast", nullptr, flags)) {
    if (_toast_is_error) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.45f, 1.0f));
    } else {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.92f, 0.65f, 1.0f));
    }
    ImGui::TextUnformatted(_toast_message.c_str());
    ImGui::PopStyleColor();
  }
  ImGui::End();
}

} // namespace mikugl
