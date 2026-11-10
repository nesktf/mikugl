#pragma once

#include "animation/morph.hpp"
#include "animation/skeleton.hpp"
#include "core.hpp"
#include "model/model_data.hpp"
#include "render/gl_mesh.hpp"
#include "render/texture_manager.hpp"
#include "scene/camera.hpp"
#include "scene/light.hpp"

#include <ranmath/ran.hpp>

#include <string>
#include <string_view>

namespace mikugl {

enum class SceneNodeType : u8 {
  None = 0,
  Model,
  Camera,
  DirectionalLight,
  PointLight,
  GroundGrid,
};

struct ModelInstance {
public:
  ModelInstance() noexcept;
  ~ModelInstance() noexcept = default;
  NTF_NO_COPY(ModelInstance);
  ModelInstance(ModelInstance&&) noexcept = default;
  ModelInstance& operator=(ModelInstance&&) noexcept = default;

public:
  ran::Mat4f32 compute_model_matrix() const noexcept;
  void reset_transform() noexcept;
  void set_all_submeshes_visible(bool vis) noexcept;

public:
  std::string name;
  std::string path;
  GlMesh gl_mesh;
  Optional<TextureManager> texture_manager;
  Optional<Skeleton> skeleton;
  Optional<MorphManager> morph_manager;
  Vec<ModelSubmesh> submeshes;
  Vec<ModelMaterial> materials;
  ran::Vec3f32 position;
  ran::Quatf32 rotation;
  ran::Vec3f32 scale;
  bool visible;
  i32 selected_bone_idx;
  i32 hovered_bone_idx;
};

class Scene final {
public:
  Scene() noexcept;

public:
  u32 add_model(ModelData&& data, std::string_view path, bool upload_gpu = true);
  void remove_model(u32 index);

  ModelInstance* selected_model() noexcept;
  const ModelInstance* selected_model() const noexcept;

  void select_model(i32 index) noexcept;
  void select_node(SceneNodeType type, i32 index = -1) noexcept;

  bool has_visible_models() const noexcept;

  i32 pick_bone_screen_space(f32 mouse_x, f32 mouse_y, f32 viewport_w, f32 viewport_h,
                             f32 max_pixel_dist = 14.0f) const noexcept;
  void set_posing_mode(bool enabled) noexcept;

public:
  Span<ModelInstance> models() noexcept { return _models; }

  Span<const ModelInstance> models() const noexcept { return _models; }

  MmdCamera& camera() noexcept { return _camera; }

  const MmdCamera& camera() const noexcept { return _camera; }

  LightManager& lights() noexcept { return _lights; }

  const LightManager& lights() const noexcept { return _lights; }

  bool is_grid_visible() const noexcept { return _grid_visible; }

  void set_grid_visible(bool visible) noexcept { _grid_visible = visible; }

  bool is_posing_mode() const noexcept { return _posing_mode; }

  SceneNodeType selected_node_type() const noexcept { return _selected_node_type; }

  i32 selected_model_index() const noexcept { return _selected_model_idx; }

  i32 selected_point_light_index() const noexcept { return _selected_point_light_idx; }

private:
  Vec<ModelInstance> _models;
  MmdCamera _camera;
  LightManager _lights;
  bool _grid_visible;
  bool _posing_mode;
  SceneNodeType _selected_node_type;
  i32 _selected_model_idx;
  i32 _selected_point_light_idx;
};

} // namespace mikugl
