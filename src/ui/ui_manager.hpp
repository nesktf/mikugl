#pragma once

#include "core.hpp"

#include <ranmath/ran.hpp>

#include <filesystem>
#include <string>
#include <string_view>

#include <SDL2/SDL.h>

namespace mikugl {

namespace stdfs = std::filesystem;

class Scene;
class CommandStack;
class AsyncLoader;

class UiManager final {
public:
  struct FrameActions {
    bool request_screenshot = false;
    bool request_exit = false;
  };

private:
  static void draw_loading_indicator(bool is_loading, std::string_view path);
  static bool draw_scene_panel(Scene& scene, bool* open = nullptr);
  static void draw_morph_panel(Scene& scene, bool* open = nullptr);
  static void draw_bone_panel(Scene& scene, bool* open = nullptr);
  static void draw_inspector_panel(Scene& scene, bool* open = nullptr);

public:
  UiManager(const stdfs::path& exe_dir, SDL_Window* window, void* gl_context);

  ~UiManager() noexcept;
  NTF_NO_MOVE(UiManager);
  NTF_NO_COPY(UiManager);

public:
  void process_event(const SDL_Event& event) noexcept;
  FrameActions draw(Scene& scene, CommandStack& command_stack, AsyncLoader& async_loader);
  void render_draw_data() const noexcept;

  void open_load_model_modal() noexcept;
  void show_toast(std::string_view message, bool is_error = false) noexcept;

public:
  bool wants_capture_mouse() const noexcept;
  bool wants_capture_keyboard() const noexcept;
  ran::Vec2f32 display_size() const noexcept;

private:
  void _draw_file_browser_modal(AsyncLoader& async_loader);
  void _draw_help_modal();
  void _draw_toast() noexcept;

private:
  stdfs::path _exe_dir;
  stdfs::path _browser_dir;
  std::string _selected_file;
  std::string _toast_message;
  f32 _toast_timer;
  bool _toast_is_error;
  bool _open_file_modal;
  bool _open_help_modal;
  bool _show_scene_panel;
  bool _show_bone_panel;
  bool _show_morph_panel;
  bool _show_inspector_panel;
  bool _show_timeline_panel;
  bool _show_demo_window;
};

} // namespace mikugl
