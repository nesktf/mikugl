#include "core.hpp"
#include "core/command_stack.hpp"
#include "model/async_loader.hpp"
#include "model/model_data.hpp"
#include "render/gl_mesh.hpp"
#include "render/grid.hpp"
#include "render/shader_program.hpp"
#include "render/shadow_map.hpp"
#include "render/texture_manager.hpp"
#include "render/toon_manager.hpp"
#include "scene/camera.hpp"
#include "ui/loading_indicator.hpp"
#include "ui/theme.hpp"
#include <embedded_shaders.hpp>

#include <SDL_video.h>
#include <glad/glad.h>

#include <SDL2/SDL.h>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

#include <cstdio>
#include <filesystem>

using namespace mikugl;
namespace stdfs = std::filesystem;

[[noreturn]] void mikugl::migu_panic_handler(const char* file, const char* func, int line,
                                             const char* msg) {
  static constexpr std::string_view srcdir =
#ifdef _WIN
    "mikugl\\src\\";
#else
    "mikugl/src/";
#endif
  std::string_view str{file};
  const auto src_dir_pos = str.find(srcdir);
  if (src_dir_pos != std::string_view::npos) {
    str = str.substr(src_dir_pos + srcdir.size());
  }
  // str should still be null terminated at this point
  fprintf(stderr, "PANIC: %s@%s:%d: %s\n", func, str.data(), line, msg);
  fprintf(stderr, "MIGU IS FUARKING DEAD, AND YOU KILLED HER\n");
  MIKU_ABORT();
  MIKU_UNREACHABLE();
}

namespace {

void init_imgui(SDL_Window* window, SDL_GLContext gl_context, f32 main_scale) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  MIKU_UNUSED(io);

  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  Theme::apply_dark_theme();

  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(main_scale);
  style.FontScaleDpi = main_scale;

  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init(nullptr);
}

std::string find_default_model_path() {
  const auto candidate_paths = std::to_array<std::string_view>({
    "models/default/Miku_Hatsune_Ver2.pmd",
#ifdef MIKU_DEFAULT_MODEL_DIR
    MIKU_DEFAULT_MODEL_DIR "/Miku_Hatsune_Ver2.pmd",
#endif
#ifdef MIKU_MMDGLTF_MODEL_DIR
    MIKU_MMDGLTF_MODEL_DIR "/default/Miku_Hatsune_Ver2.pmd",
#endif
    "lib/mmdgltf/models/default/Miku_Hatsune_Ver2.pmd",
  });

  for (const auto& path : candidate_paths) {
    if (stdfs::exists(path)) {
      return std::string(path);
    }
  }
  return std::string(candidate_paths.front());
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

  auto grid = Grid::create();
  if (!grid) {
    fprintf(stderr, "Failed to initialize grid renderer\n");
    return 1;
  }

  auto mmd_shader = ShaderProgram::create(shaders::k_mmd_model_vert, shaders::k_mmd_model_frag);
  if (!mmd_shader) {
    fprintf(stderr, "Failed to initialize MMD shader\n");
    return 1;
  }

  auto outline_shader = ShaderProgram::create(shaders::k_outline_vert, shaders::k_outline_frag);
  if (!outline_shader) {
    fprintf(stderr, "Failed to initialize outline shader\n");
    return 1;
  }

  auto shadow_shader =
    ShaderProgram::create(shaders::k_shadow_depth_vert, shaders::k_shadow_depth_frag);
  if (!shadow_shader) {
    fprintf(stderr, "Failed to initialize shadow shader\n");
    return 1;
  }

  ToonManager toon_manager;
  toon_manager.init();

  ShadowMap shadow_map;
  shadow_map.init();

  GlMesh gl_mesh;
  TextureManager texture_manager;
  texture_manager.init();

  Vec<ModelSubmesh> model_submeshes;
  Vec<ModelMaterial> model_materials;
  std::string loaded_model_name;

  AsyncLoader async_loader;
  const auto default_model_path = find_default_model_path();
  async_loader.request_load(default_model_path);

  MmdCamera camera{};
  CommandStack command_stack{};
  ran::Vec3f32 light_color{1.0f, 1.0f, 1.0f};
  ran::Vec3f32 light_dir{0.5f, 1.0f, 0.7f};

  bool should_exit = false;
  bool show_scene_panel = true;
  bool show_bone_panel = true;
  bool show_inspector_panel = true;
  bool show_timeline_panel = true;
  bool show_demo_window = false;

  bool is_right_dragging = false;
  bool is_middle_dragging = false;

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

      if (e.type == SDL_MOUSEBUTTONDOWN) {
        if (!io.WantCaptureMouse) {
          const bool shift = (SDL_GetModState() & KMOD_SHIFT) != 0;
          if (e.button.button == SDL_BUTTON_RIGHT) {
            is_right_dragging = true;
          } else if (e.button.button == SDL_BUTTON_MIDDLE ||
                     (shift && e.button.button == SDL_BUTTON_LEFT)) {
            is_middle_dragging = true;
          }
        }
      }

      if (e.type == SDL_MOUSEBUTTONUP) {
        if (e.button.button == SDL_BUTTON_RIGHT) {
          is_right_dragging = false;
        } else if (e.button.button == SDL_BUTTON_MIDDLE || e.button.button == SDL_BUTTON_LEFT) {
          is_middle_dragging = false;
        }
      }

      if (e.type == SDL_MOUSEMOTION) {
        if (is_right_dragging) {
          camera.orbit(static_cast<f32>(e.motion.xrel), static_cast<f32>(e.motion.yrel));
        } else if (is_middle_dragging) {
          camera.pan(static_cast<f32>(e.motion.xrel), static_cast<f32>(e.motion.yrel));
        }
      }

      if (e.type == SDL_MOUSEWHEEL) {
        if (!io.WantCaptureMouse) {
          camera.zoom(static_cast<f32>(e.wheel.y));
        }
      }

      if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_ESCAPE) {
          should_exit = true;
        }

        if (e.key.keysym.sym == SDLK_HOME) {
          camera.reset();
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

    if (auto result = async_loader.poll_result(); result.has_value()) {
      if (*result) {
        ModelData& data = **result;
        loaded_model_name = data.name.empty() ? "MMD Model" : std::move(data.name);
        gl_mesh.upload(data.vertices, data.indices);
        texture_manager.upload_textures(data.textures);
        model_submeshes = std::move(data.submeshes);
        model_materials = std::move(data.materials);
      } else {
        const auto path = async_loader.current_path();
        fprintf(stderr, "Failed to load model: %.*s\n", (int)path.size(), path.data());
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
        if (ImGui::MenuItem("Reset Camera", "Home")) {
          camera.reset();
        }
        ImGui::Separator();
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
        if (gl_mesh.is_valid()) {
          ImGui::Text("Model: %s", loaded_model_name.c_str());
          ImGui::Text("Vertices: %u, Indices: %u", gl_mesh.vertex_count(), gl_mesh.index_count());
          ImGui::Separator();
          ImGui::Text("Submeshes (%zu):", model_submeshes.size());
          for (size_t i = 0; i < model_submeshes.size(); ++i) {
            const auto& sm = model_submeshes[i];
            ImGui::BulletText("[%zu] %s (count: %u)", i, sm.name.c_str(), sm.index_count);
          }
        } else {
          ImGui::TextDisabled("No model loaded");
        }
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
        ImGui::Spacing();
        ImGui::SeparatorText("Directional Light");
        ImGui::ColorEdit3("Light Color", light_color.data());
        ImGui::DragFloat3("Light Direction", light_dir.data(), 0.01f, -1.0f, 1.0f);
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

    LoadingIndicator::draw(async_loader.is_loading(), async_loader.current_path());

    ImGui::Render();

    const f32 display_w = io.DisplaySize.x;
    const f32 display_h = io.DisplaySize.y;
    const f32 aspect = (display_h > 0.0f) ? (display_w / display_h) : 1.0f;
    const ran::Mat4f32 view = camera.compute_view_matrix();
    const ran::Mat4f32 proj = camera.compute_projection_matrix(aspect);

    const auto [light_view, light_proj] =
      shadow_map.compute_light_matrices(camera.target(), light_dir);
    const ran::Mat4f32 light_view_proj = light_proj * light_view;

    if (gl_mesh.is_valid()) {
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LESS);
      shadow_map.bind_fbo();
      shadow_shader->bind();
      shadow_shader->set_uniform_mat4("u_model", ran::Mat4f32::identity());
      shadow_shader->set_uniform_mat4("u_light_view", light_view);
      shadow_shader->set_uniform_mat4("u_light_proj", light_proj);
      gl_mesh.bind();
      for (const auto& sm : model_submeshes) {
        gl_mesh.draw_submesh(sm.index_offset, sm.index_count);
      }
      gl_mesh.unbind();
      shadow_map.unbind_fbo(static_cast<i32>(display_w), static_cast<i32>(display_h));
    }

    glViewport(0, 0, static_cast<i32>(display_w), static_cast<i32>(display_h));
    glClearColor(0.18f, 0.18f, 0.19f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shadow_map.bind_texture(3);
    grid->render(view, proj, light_view_proj, gl_mesh.is_valid());

    if (gl_mesh.is_valid()) {
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LESS);

      mmd_shader->bind();
      mmd_shader->set_uniform_mat4("u_model", ran::Mat4f32::identity());
      mmd_shader->set_uniform_mat4("u_view", view);
      mmd_shader->set_uniform_mat4("u_proj", proj);
      mmd_shader->set_uniform_mat4("u_light_view_proj", light_view_proj);
      mmd_shader->set_uniform_vec3("u_light_dir", light_dir);
      mmd_shader->set_uniform_vec3("u_light_color", light_color);
      mmd_shader->set_uniform_vec3("u_camera_pos", camera.compute_position());
      shadow_map.bind_texture(3);
      mmd_shader->set_uniform_int("u_has_shadow", 1);
      gl_mesh.bind();

      for (const auto& sm : model_submeshes) {
        if (sm.material_index < model_materials.size()) {
          const auto& mat = model_materials[sm.material_index];
          if (mat.is_double_sided) {
            glDisable(GL_CULL_FACE);
          } else {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
          }
          texture_manager.bind_texture(mat.texture_index, 0);
          mmd_shader->set_uniform_int("u_has_diffuse", mat.texture_index >= 0 ? 1 : 0);
          mmd_shader->set_uniform_vec4("u_diffuse_color", mat.diffuse);
          mmd_shader->set_uniform_vec3("u_specular_color", mat.specular);
          mmd_shader->set_uniform_vec3("u_ambient_color", mat.ambient);
          mmd_shader->set_uniform_float("u_shininess", mat.shininess);

          if (mat.sphere_texture_index >= 0 && mat.sphere_mode > 0) {
            texture_manager.bind_texture(mat.sphere_texture_index, 1);
            mmd_shader->set_uniform_int("u_has_sphere", 1);
            mmd_shader->set_uniform_int("u_sphere_mode", mat.sphere_mode);
          } else {
            mmd_shader->set_uniform_int("u_has_sphere", 0);
            mmd_shader->set_uniform_int("u_sphere_mode", 0);
          }

          if (mat.toon_sharing_flag == 1) {
            toon_manager.bind_shared_toon(static_cast<u8>(mat.toon_index), 2);
            mmd_shader->set_uniform_int("u_has_toon", 1);
          } else if (mat.toon_index >= 0) {
            texture_manager.bind_texture(mat.toon_index, 2);
            mmd_shader->set_uniform_int("u_has_toon", 1);
          } else {
            mmd_shader->set_uniform_int("u_has_toon", 0);
          }
        }
        gl_mesh.draw_submesh(sm.index_offset, sm.index_count);
      }
      gl_mesh.unbind();
      glDisable(GL_CULL_FACE);

      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LESS);
      glEnable(GL_CULL_FACE);
      glCullFace(GL_FRONT);

      outline_shader->bind();
      outline_shader->set_uniform_mat4("u_model", ran::Mat4f32::identity());
      outline_shader->set_uniform_mat4("u_view", view);
      outline_shader->set_uniform_mat4("u_proj", proj);
      outline_shader->set_uniform_vec2("u_viewport_size", ran::Vec2f32{display_w, display_h});
      gl_mesh.bind();

      for (const auto& sm : model_submeshes) {
        if (sm.material_index < model_materials.size()) {
          const auto& mat = model_materials[sm.material_index];
          if (mat.has_edge && mat.edge_size > 0.0f) {
            outline_shader->set_uniform_float("u_edge_size", mat.edge_size);
            outline_shader->set_uniform_vec4("u_edge_color", mat.edge_color);
            gl_mesh.draw_submesh(sm.index_offset, sm.index_count);
          }
        }
      }
      gl_mesh.unbind();
      glCullFace(GL_BACK);
      glDisable(GL_CULL_FACE);
      ShaderProgram::unbind();
    }

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(win);
  }
  printf("(in Mario voice) ByeBye!\n");
  return 0;
}
