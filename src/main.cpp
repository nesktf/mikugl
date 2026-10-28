#include "core.hpp"
#include "core/command_stack.hpp"
#include "ui/theme.hpp"

#include <SDL_video.h>
#include <glad/glad.h>

#include <SDL2/SDL.h>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

#include <cstdio>

using namespace mikugl;

namespace {

void init_imgui(SDL_Window* window, SDL_GLContext gl_context, f32 main_scale) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  MIKU_UNUSED(io);

  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  apply_imgui_dark_theme();

  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(main_scale);
  style.FontScaleDpi = main_scale;

  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init(nullptr);
}

} // namespace

int main(int argc, char* argv[]) {
  MIKU_UNUSED(argc);
  MIKU_UNUSED(argv);

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1) {
    fprintf(stderr, "Failed to load SDL2\n");
    return 1;
  }
  const DeferFn sdl_defer = []() {
    SDL_Quit();
  };

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  const f32 main_scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);

  SDL_GLContext gl = nullptr;
  SDL_Window* win = SDL_CreateWindow("mikugl", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                     1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  if (!win) {
    fprintf(stderr, "Failed to create SDL window\n");
    return 1;
  }
  const DeferFn win_defer = [&]() {
    SDL_DestroyWindow(win);
    if (gl) {
      SDL_GL_DeleteContext(gl);
    }
  };

  gl = SDL_GL_CreateContext(win);
  if (!gl) {
    fprintf(stderr, "Failed to create gl context\n");
    return 1;
  }
  SDL_GL_MakeCurrent(win, gl);
  SDL_GL_SetSwapInterval(1);

  if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
    fprintf(stderr, "Failed to load GLAD\n");
    return 1;
  }

  init_imgui(win, gl, main_scale);

  CommandStack command_stack{};
  bool should_exit = false;
  bool show_scene_panel = true;
  bool show_bone_panel = true;
  bool show_inspector_panel = true;
  bool show_timeline_panel = true;
  bool show_demo_window = false;

  const ImGuiIO& io = ImGui::GetIO();
  while (!should_exit) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      ImGui_ImplSDL2_ProcessEvent(&e);
      if (e.type == SDL_QUIT) {
        should_exit = true;
      }
      if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE &&
          e.window.windowID == SDL_GetWindowID(win)) {
        should_exit = true;
      }
      if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_ESCAPE) {
          should_exit = true;
        }

        const bool ctrl = (SDL_GetModState() & KMOD_CTRL) != 0;
        const bool shift = (SDL_GetModState() & KMOD_SHIFT) != 0;
        if (ctrl && !shift && e.key.keysym.sym == SDLK_z) {
          const auto res = command_stack.undo();
          MIKU_UNUSED(res);
        } else if ((ctrl && e.key.keysym.sym == SDLK_y) ||
                   (ctrl && shift && e.key.keysym.sym == SDLK_z)) {
          const auto res = command_stack.redo();
          MIKU_UNUSED(res);
        }
      }
    }

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
          // TODO: Load PMD/PMX model
        }
        if (ImGui::MenuItem("Export PNG...", "Ctrl+E")) {
          // TODO: Export PNG screenshot
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Exit", "Alt+F4")) {
          should_exit = true;
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
        ImGui::MenuItem("Scene Nodes", nullptr, &show_scene_panel);
        ImGui::MenuItem("Bone Hierarchy", nullptr, &show_bone_panel);
        ImGui::MenuItem("Inspector", nullptr, &show_inspector_panel);
        ImGui::MenuItem("Timeline", nullptr, &show_timeline_panel);
        ImGui::Separator();
        ImGui::MenuItem("ImGui Demo Window", nullptr, &show_demo_window);
        ImGui::EndMenu();
      }

      ImGui::EndMainMenuBar();
    }

    if (show_scene_panel) {
      if (ImGui::Begin("Scene Nodes", &show_scene_panel)) {
        ImGui::TextDisabled("Scene hierarchy");
        // TODO: Populate scene nodes (Model instance, Camera, Light, Grid)
      }
      ImGui::End();
    }

    if (show_bone_panel) {
      if (ImGui::Begin("Bone Hierarchy", &show_bone_panel)) {
        ImGui::TextDisabled("Bone list");
        // TODO: Populate bone tree and visibility toggles
      }
      ImGui::End();
    }

    if (show_inspector_panel) {
      if (ImGui::Begin("Inspector", &show_inspector_panel)) {
        ImGui::TextDisabled("Transform inspector");
        // TODO: Show selected node / bone transform controls
      }
      ImGui::End();
    }

    if (show_timeline_panel) {
      if (ImGui::Begin("Timeline", &show_timeline_panel)) {
        ImGui::TextDisabled("Animation timeline");
        // TODO: Playback controls and keyframe tracks
      }
      ImGui::End();
    }

    if (show_demo_window) {
      ImGui::ShowDemoWindow(&show_demo_window);
    }

    ImGui::Render();

    glViewport(0, 0, static_cast<i32>(io.DisplaySize.x), static_cast<i32>(io.DisplaySize.y));
    glClearColor(0.18f, 0.18f, 0.19f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // TODO: Render 3D ground grid and camera viewport to backbuffer
    // TODO: Render MMD models to backbuffer

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(win);
  }

  return 0;
}
