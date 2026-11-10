#include "core/command_stack.hpp"
#include "io/screenshot.hpp"
#include "model/async_loader.hpp"
#include "render/debug_draw.hpp"
#include "render/grid.hpp"
#include "render/shader_program.hpp"
#include "render/shadow_map.hpp"
#include "render/toon_manager.hpp"
#include "scene/scene.hpp"
#include "ui/ui_manager.hpp"
#include <embedded_shaders.hpp>

#include <SDL2/SDL.h>
#include <SDL_video.h>
#include <glad/glad.h>

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

struct SDLContext {
  SDL_Window* win;
  SDL_GLContext gl;
};

Optional<SDLContext> init_sdl(u32 width, u32 height) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1) {
    fprintf(stderr, "Failed to load SDL2\n");
    return std::nullopt;
  }
  SDL_Window* win = nullptr;
  SDL_GLContext gl = nullptr;
  DeferFn on_err = [&]() {
    if (gl) {
      SDL_GL_DeleteContext(gl);
    }
    if (win) {
      SDL_DestroyWindow(win);
    }
    SDL_Quit();
  };

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  win = SDL_CreateWindow("mikugl", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height,
                         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  if (!win) {
    fprintf(stderr, "Failed to create SDL window\n");
    return std::nullopt;
  }

  gl = SDL_GL_CreateContext(win);
  if (!gl) {
    fprintf(stderr, "Failed to create gl context\n");
    return std::nullopt;
  }
  SDL_GL_MakeCurrent(win, gl);
  SDL_GL_SetSwapInterval(1);

  if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
    fprintf(stderr, "Failed to load GLAD\n");
    return std::nullopt;
  }

  on_err.disengage();
  return Optional<SDLContext>{std::in_place, win, gl};
}

} // namespace

int main(int argc, char* argv[]) {
  MIKU_UNUSED(argc);
  MIKU_UNUSED(argv);

  const auto exe_dir = []() {
#if defined(__linux__)
    std::error_code ec;
    const auto p = stdfs::canonical("/proc/self/exe", ec);
    if (!ec) {
      return p.parent_path();
    }
#elif defined(_WIN32)
    wchar_t buffer[MAX_PATH];
    if (GetModuleFileNameW(nullptr, buffer, MAX_PATH) > 0) {
      return stdfs::path(buffer).parent_path();
    }
#endif
    // TODO: check path in libnx
    return stdfs::current_path();
  }();

  const auto sdl = init_sdl(1280, 720);
  if (!sdl) {
    return 1;
  }
  const DeferFn win_defer = [&]() {
    SDL_GL_DeleteContext(sdl->gl);
    SDL_DestroyWindow(sdl->win);
    SDL_Quit();
  };

  UiManager ui_manager(exe_dir, sdl->win, sdl->gl);

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

  auto debug_draw = DebugDraw::create();
  if (!debug_draw) {
    fprintf(stderr, "Failed to initialize debug draw renderer\n");
    return 1;
  }

  auto toon_manager = ToonManager::create();
  auto shadow_map = ShadowMap::create();

  Scene scene{};
  CommandStack command_stack{};
  AsyncLoader async_loader;

  const auto default_model_path = exe_dir / "model" / "default" / "Miku_Hatsune_Ver2.pmd";
  if (!stdfs::exists(default_model_path)) {
    fprintf(stderr, "Migu not found at '%s'\n", default_model_path.c_str());
  } else {
    async_loader.request_load(default_model_path.c_str());
  }

  bool should_exit = false;
  bool is_right_dragging = false;
  bool is_middle_dragging = false;
  bool hotkey_screenshot = false;

  while (!should_exit) {
    const ran::Vec2f32 disp_size = ui_manager.display_size();

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      ui_manager.process_event(e);
      if (e.type == SDL_QUIT) {
        should_exit = true;
      }
      if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE &&
          e.window.windowID == SDL_GetWindowID(sdl->win)) {
        should_exit = true;
      }

      if (e.type == SDL_MOUSEBUTTONDOWN) {
        if (!ui_manager.wants_capture_mouse()) {
          const bool shift = (SDL_GetModState() & KMOD_SHIFT) != 0;
          if (e.button.button == SDL_BUTTON_RIGHT) {
            is_right_dragging = true;
          } else if (e.button.button == SDL_BUTTON_MIDDLE ||
                     (shift && e.button.button == SDL_BUTTON_LEFT)) {
            is_middle_dragging = true;
          } else if (e.button.button == SDL_BUTTON_LEFT && !shift && scene.is_posing_mode()) {
            if (auto* mdl = scene.selected_model()) {
              mdl->selected_bone_idx = scene.pick_bone_screen_space(static_cast<f32>(e.button.x),
                                                                    static_cast<f32>(e.button.y),
                                                                    disp_size.x, disp_size.y);
            }
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
          scene.camera().orbit(static_cast<f32>(e.motion.xrel), static_cast<f32>(e.motion.yrel));
        } else if (is_middle_dragging) {
          scene.camera().pan(static_cast<f32>(e.motion.xrel), static_cast<f32>(e.motion.yrel));
        } else if (!ui_manager.wants_capture_mouse() && scene.is_posing_mode()) {
          if (auto* mdl = scene.selected_model()) {
            mdl->hovered_bone_idx =
              scene.pick_bone_screen_space(static_cast<f32>(e.motion.x),
                                           static_cast<f32>(e.motion.y), disp_size.x, disp_size.y);
          }
        }
      }

      if (e.type == SDL_MOUSEWHEEL) {
        if (!ui_manager.wants_capture_mouse()) {
          scene.camera().zoom(static_cast<f32>(e.wheel.y));
        }
      }

      if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_F2) {
          hotkey_screenshot = true;
        } else if (!ui_manager.wants_capture_keyboard()) {
          if (e.key.keysym.sym == SDLK_ESCAPE) {
            should_exit = true;
          } else if (e.key.keysym.sym == SDLK_HOME) {
            scene.camera().reset();
          } else if (e.key.keysym.sym == SDLK_TAB) {
            scene.set_posing_mode(!scene.is_posing_mode());
          }

          const bool ctrl = (SDL_GetModState() & KMOD_CTRL) != 0;
          const bool shift = (SDL_GetModState() & KMOD_SHIFT) != 0;
          if (ctrl && e.key.keysym.sym == SDLK_o) {
            ui_manager.open_load_model_modal();
          } else if (ctrl && !shift && e.key.keysym.sym == SDLK_z) {
            const auto res = command_stack.undo();
            MIKU_UNUSED(res);
          } else if ((ctrl && e.key.keysym.sym == SDLK_y) ||
                     (ctrl && shift && e.key.keysym.sym == SDLK_z)) {
            const auto res = command_stack.redo();
            MIKU_UNUSED(res);
          }
        }
      }
    }

    auto result = async_loader.poll_result();
    if (result) {
      scene.add_model(std::move(*result), async_loader.current_path());
    } else if (result.error() != ModelLoadError::NotYetLoaded) {
      const auto path = async_loader.current_path();
      fprintf(stderr, "Failed to load model: %.*s\n", static_cast<int>(path.size()), path.data());
      ui_manager.show_toast("Failed to load model", true);
    }

    const auto ui_actions = ui_manager.draw(scene, command_stack, async_loader);
    if (ui_actions.request_exit) {
      should_exit = true;
    }

    const ran::Vec2f32 cur_disp = ui_manager.display_size();
    const f32 display_w = cur_disp.x;
    const f32 display_h = cur_disp.y;
    const f32 aspect = (display_h > 0.0f) ? (display_w / display_h) : 1.0f;
    const ran::Mat4f32 view = scene.camera().compute_view_matrix();
    const ran::Mat4f32 proj = scene.camera().compute_projection_matrix(aspect);

    const auto& dir_light = scene.lights().directional_light();
    const auto [light_view, light_proj] =
      shadow_map.compute_light_matrices(scene.camera().target(), dir_light.direction);
    const ran::Mat4f32 light_view_proj = light_proj * light_view;

    for (auto& mdl : scene.models()) {
      if (!mdl.visible || !mdl.gl_mesh.is_valid()) {
        continue;
      }
      if (mdl.skeleton) {
        mdl.skeleton->update_ubo();
      }
      if (mdl.morph_manager && mdl.morph_manager->evaluate()) {
        mdl.gl_mesh.update_vertices(mdl.morph_manager->deformed_vertices());
      }
    }

    const bool has_shadow_casters = scene.has_visible_models() && dir_light.enabled;
    if (has_shadow_casters) {
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LESS);
      shadow_map.bind_fbo();
      shadow_shader->bind();
      shadow_shader->set_uniform_mat4("u_light_view", light_view);
      shadow_shader->set_uniform_mat4("u_light_proj", light_proj);

      for (const auto& mdl : scene.models()) {
        if (!mdl.visible || !mdl.gl_mesh.is_valid()) {
          continue;
        }
        shadow_shader->set_uniform_mat4("u_model", mdl.compute_model_matrix());
        shadow_shader->set_uniform_int("u_skinning_enabled",
                                       static_cast<i32>(mdl.skeleton.has_value()));
        if (mdl.skeleton) {
          mdl.skeleton->bind_ubo(2);
        }
        mdl.gl_mesh.bind();
        for (const auto& sm : mdl.submeshes) {
          if (sm.visible) {
            mdl.gl_mesh.draw_submesh(sm.index_offset, sm.index_count);
          }
        }
        mdl.gl_mesh.unbind();
      }
      shadow_map.unbind_fbo(static_cast<i32>(display_w), static_cast<i32>(display_h));
    }

    glViewport(0, 0, static_cast<i32>(display_w), static_cast<i32>(display_h));
    glClearColor(0.18f, 0.18f, 0.19f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (scene.is_grid_visible()) {
      shadow_map.bind_texture(3);
      grid->render(view, proj, light_view_proj, has_shadow_casters);
    }

    if (scene.has_visible_models()) {
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LESS);

      mmd_shader->bind();
      mmd_shader->set_uniform_mat4("u_view", view);
      mmd_shader->set_uniform_mat4("u_proj", proj);
      mmd_shader->set_uniform_mat4("u_light_view_proj", light_view_proj);
      mmd_shader->set_uniform_vec3("u_camera_pos", scene.camera().compute_position());
      scene.lights().upload_to_shader(*mmd_shader);
      shadow_map.bind_texture(3);
      mmd_shader->set_uniform_int("u_has_shadow", has_shadow_casters ? 1 : 0);

      for (const auto& mdl : scene.models()) {
        if (!mdl.visible || !mdl.gl_mesh.is_valid()) {
          continue;
        }
        mmd_shader->set_uniform_mat4("u_model", mdl.compute_model_matrix());
        mmd_shader->set_uniform_int("u_skinning_enabled",
                                    static_cast<i32>(mdl.skeleton.has_value()));
        if (mdl.skeleton) {
          mdl.skeleton->bind_ubo(2);
        }
        mdl.gl_mesh.bind();

        for (const auto& sm : mdl.submeshes) {
          if (!sm.visible) {
            continue;
          }
          if (sm.material_index < mdl.materials.size()) {
            const auto& mat = mdl.materials[sm.material_index];
            if (mat.is_double_sided) {
              glDisable(GL_CULL_FACE);
            } else {
              glEnable(GL_CULL_FACE);
              glCullFace(GL_BACK);
            }
            if (mdl.texture_manager) {
              mdl.texture_manager->bind_texture(mat.texture_index, 0);
            }
            mmd_shader->set_uniform_int("u_has_diffuse", mat.texture_index >= 0 ? 1 : 0);
            mmd_shader->set_uniform_vec4("u_diffuse_color", mat.diffuse);
            mmd_shader->set_uniform_vec3("u_specular_color", mat.specular);
            mmd_shader->set_uniform_vec3("u_ambient_color", mat.ambient);
            mmd_shader->set_uniform_float("u_shininess", mat.shininess);

            if (mat.sphere_texture_index >= 0 && mat.sphere_mode > 0 && mdl.texture_manager) {
              mdl.texture_manager->bind_texture(mat.sphere_texture_index, 1);
              mmd_shader->set_uniform_int("u_has_sphere", 1);
              mmd_shader->set_uniform_int("u_sphere_mode", mat.sphere_mode);
            } else {
              mmd_shader->set_uniform_int("u_has_sphere", 0);
              mmd_shader->set_uniform_int("u_sphere_mode", 0);
            }

            if (mat.toon_sharing_flag == 1) {
              toon_manager.bind_shared_toon(static_cast<u8>(mat.toon_index), 2);
              mmd_shader->set_uniform_int("u_has_toon", 1);
            } else if (mat.toon_index >= 0 && mdl.texture_manager) {
              mdl.texture_manager->bind_texture(mat.toon_index, 2);
              mmd_shader->set_uniform_int("u_has_toon", 1);
            } else {
              mmd_shader->set_uniform_int("u_has_toon", 0);
            }
          }
          mdl.gl_mesh.draw_submesh(sm.index_offset, sm.index_count);
        }
        mdl.gl_mesh.unbind();
      }
      glDisable(GL_CULL_FACE);

      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LESS);
      glEnable(GL_CULL_FACE);
      glCullFace(GL_FRONT);

      outline_shader->bind();
      outline_shader->set_uniform_mat4("u_view", view);
      outline_shader->set_uniform_mat4("u_proj", proj);
      outline_shader->set_uniform_vec2("u_viewport_size", ran::Vec2f32{display_w, display_h});

      for (const auto& mdl : scene.models()) {
        if (!mdl.visible || !mdl.gl_mesh.is_valid()) {
          continue;
        }
        outline_shader->set_uniform_mat4("u_model", mdl.compute_model_matrix());
        outline_shader->set_uniform_int("u_skinning_enabled",
                                        static_cast<i32>(mdl.skeleton.has_value()));
        if (mdl.skeleton) {
          mdl.skeleton->bind_ubo(2);
        }
        mdl.gl_mesh.bind();

        for (const auto& sm : mdl.submeshes) {
          if (!sm.visible) {
            continue;
          }
          if (sm.material_index < mdl.materials.size()) {
            const auto& mat = mdl.materials[sm.material_index];
            if (mat.has_edge && mat.edge_size > 0.0f) {
              outline_shader->set_uniform_float("u_edge_size", mat.edge_size);
              outline_shader->set_uniform_vec4("u_edge_color", mat.edge_color);
              mdl.gl_mesh.draw_submesh(sm.index_offset, sm.index_count);
            }
          }
        }
        mdl.gl_mesh.unbind();
      }
      glCullFace(GL_BACK);
      glDisable(GL_CULL_FACE);
      ShaderProgram::unbind();
    }

    if (hotkey_screenshot || ui_actions.request_screenshot) {
      hotkey_screenshot = false;
      const auto shot_res = Screenshot::capture_to_screenshots_dir(
        exe_dir, static_cast<i32>(display_w), static_cast<i32>(display_h));
      if (shot_res) {
        printf("Screenshot saved: %s\n", shot_res->c_str());
        ui_manager.show_toast("Screenshot saved: " + stdfs::path(*shot_res).filename().string());
      } else {
        fprintf(stderr, "Failed to save screenshot\n");
        ui_manager.show_toast("Failed to save screenshot", true);
      }
    }

    {
      debug_draw->begin(proj * view);
      if (scene.is_posing_mode()) {
        if (const auto* active_mdl = scene.selected_model()) {
          if (active_mdl->visible && active_mdl->skeleton) {
            debug_draw->draw_skeleton(*active_mdl->skeleton, active_mdl->selected_bone_idx,
                                      active_mdl->hovered_bone_idx,
                                      active_mdl->compute_model_matrix());
          }
        }
      }

      const auto pt_lights = scene.lights().point_lights();
      for (u32 i = 0; i < pt_lights.size(); ++i) {
        const auto& pl = pt_lights[i];
        const bool is_sel = (scene.selected_node_type() == SceneNodeType::PointLight &&
                             scene.selected_point_light_index() == static_cast<i32>(i));
        if (pl.enabled && (is_sel || scene.is_posing_mode())) {
          debug_draw->draw_point_light(pl.position, pl.color, pl.radius, is_sel);
        }
      }
      debug_draw->render();
    }

    ui_manager.render_draw_data();
    SDL_GL_SwapWindow(sdl->win);
  }
  printf("(in Mario voice) ByeBye!\n");
  return 0;
}
