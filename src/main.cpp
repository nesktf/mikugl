#include "core.hpp"

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
  // context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  MIKU_UNUSED(io);
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

  // style
  ImGuiStyle& style = ImGui::GetStyle();
  ImGui::StyleColorsDark();
  style.FrameRounding = 3.0f;
  style.GrabRounding = 3.0f;

  // scaling
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

  SDL_GLContext gl = NULL;
  SDL_Window* win = SDL_CreateWindow("mikugl", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                     800, 600, SDL_WINDOW_OPENGL);
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

  bool should_exit = false;
  ImGuiIO& io = ImGui::GetIO();
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
      if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        should_exit = true;
      }
    }

    // render imgui
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();
    ImGui::Render();

    // render our things
    glViewport(0, 0, (i32)io.DisplaySize.x, (i32)io.DisplaySize.y);
    glClearColor(.2f, .2f, .2f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); // imgui last!

    SDL_GL_SwapWindow(win);
  }

  fprintf(stderr, "Byebye! (mario voice)\n");
  return 0;
}
