#include "scene/scene.hpp"

#include <filesystem>

namespace mikugl {

namespace {

struct ScreenPoint {
  ran::Vec2f32 pos;
  f32 depth;
  bool valid;
};

ScreenPoint project_to_screen(const ran::Vec3f32& world_pos, const ran::Mat4f32& mvp, f32 vp_w,
                              f32 vp_h) noexcept {
  const auto clip = mvp * ran::Vec4f32{world_pos.x, world_pos.y, world_pos.z, 1.0f};
  if (clip.w <= 0.0001f) {
    return {ran::Vec2f32{0.0f, 0.0f}, 0.0f, false};
  }
  const f32 inv_w = 1.0f / clip.w;
  const f32 ndc_x = clip.x * inv_w;
  const f32 ndc_y = clip.y * inv_w;
  const f32 ndc_z = clip.z * inv_w;
  if (ndc_z < -1.0f || ndc_z > 1.0f) {
    return {ran::Vec2f32{0.0f, 0.0f}, 0.0f, false};
  }

  const f32 sx = (ndc_x * 0.5f + 0.5f) * vp_w;
  const f32 sy = (1.0f - (ndc_y * 0.5f + 0.5f)) * vp_h;
  return {ran::Vec2f32{sx, sy}, ndc_z, true};
}

f32 point_to_segment_dist(const ran::Vec2f32& p, const ran::Vec2f32& a,
                          const ran::Vec2f32& b) noexcept {
  const ran::Vec2f32 ab = b - a;
  const f32 len_sq = ran::dot(ab, ab);
  if (len_sq <= 1e-6f) {
    return ran::length(p - a);
  }
  const f32 t = ran::clamp(ran::dot(p - a, ab) / len_sq, 0.0f, 1.0f);
  const ran::Vec2f32 proj = a + ab * t;
  return ran::length(p - proj);
}

} // namespace

ModelInstance::ModelInstance() noexcept :
    name(), path(), gl_mesh(), texture_manager(std::nullopt), skeleton(std::nullopt),
    morph_manager(std::nullopt), submeshes(), materials(), position(0.0f, 0.0f, 0.0f),
    rotation(ran::Quatf32::identity()), scale(1.0f, 1.0f, 1.0f), visible(true),
    selected_bone_idx(-1), hovered_bone_idx(-1) {}

ran::Mat4f32 ModelInstance::compute_model_matrix() const noexcept {
  return ran::scale(ran::translate(ran::Mat4f32::identity(), position) * ran::to_mat4(rotation),
                    scale);
}

void ModelInstance::reset_transform() noexcept {
  position = ran::Vec3f32{0.0f, 0.0f, 0.0f};
  rotation = ran::Quatf32::identity();
  scale = ran::Vec3f32{1.0f, 1.0f, 1.0f};
}

void ModelInstance::set_all_submeshes_visible(bool vis) noexcept {
  for (auto& sm : submeshes) {
    sm.visible = vis;
  }
}

Scene::Scene() noexcept :
    _models(), _camera(), _lights(), _grid_visible(true), _posing_mode(false),
    _selected_node_type(SceneNodeType::None), _selected_model_idx(-1),
    _selected_point_light_idx(-1) {}

u32 Scene::add_model(ModelData&& data, std::string_view path, bool upload_gpu) {
  ModelInstance instance{};
  instance.path = std::string(path);
  if (!data.name.empty()) {
    instance.name = std::move(data.name);
  } else if (!path.empty()) {
    instance.name = std::filesystem::path(path).stem().string();
  } else {
    instance.name = "MMD Model " + std::to_string(_models.size() + 1);
  }

  if (!data.bones.empty()) {
    auto skel_res = Skeleton::from_model_bones(data.bones);
    if (skel_res) {
      instance.skeleton = std::move(*skel_res);
    }
  }

  instance.morph_manager = MorphManager(data.vertices, std::move(data.morphs));
  if (upload_gpu) {
    instance.gl_mesh.upload(data.vertices, data.indices);
    instance.texture_manager = TextureManager::create();
    instance.texture_manager->upload_textures(data.textures);
  }
  instance.submeshes = std::move(data.submeshes);
  instance.materials = std::move(data.materials);

  _models.push_back(std::move(instance));
  const i32 new_idx = static_cast<i32>(_models.size() - 1);
  select_model(new_idx);
  return static_cast<u32>(new_idx);
}

void Scene::remove_model(u32 index) {
  if (index >= _models.size()) {
    return;
  }
  _models.erase(_models.begin() + index);
  if (_models.empty()) {
    _selected_model_idx = -1;
    if (_selected_node_type == SceneNodeType::Model) {
      _selected_node_type = SceneNodeType::None;
    }
  } else if (_selected_model_idx >= static_cast<i32>(_models.size())) {
    _selected_model_idx = static_cast<i32>(_models.size()) - 1;
  }
}

ModelInstance* Scene::selected_model() noexcept {
  if (_selected_model_idx >= 0 && static_cast<size_t>(_selected_model_idx) < _models.size()) {
    return &_models[static_cast<size_t>(_selected_model_idx)];
  }
  return nullptr;
}

const ModelInstance* Scene::selected_model() const noexcept {
  if (_selected_model_idx >= 0 && static_cast<size_t>(_selected_model_idx) < _models.size()) {
    return &_models[static_cast<size_t>(_selected_model_idx)];
  }
  return nullptr;
}

void Scene::select_model(i32 index) noexcept {
  if (index >= 0 && static_cast<size_t>(index) < _models.size()) {
    _selected_model_idx = index;
    _selected_node_type = SceneNodeType::Model;
  } else {
    _selected_model_idx = -1;
    if (_selected_node_type == SceneNodeType::Model) {
      _selected_node_type = SceneNodeType::None;
    }
  }
}

void Scene::select_node(SceneNodeType type, i32 index) noexcept {
  _selected_node_type = type;
  if (type == SceneNodeType::Model) {
    if (index >= 0 && static_cast<size_t>(index) < _models.size()) {
      _selected_model_idx = index;
    }
  } else if (type == SceneNodeType::PointLight) {
    if (index >= 0 && static_cast<size_t>(index) < _lights.point_light_count()) {
      _selected_point_light_idx = index;
    }
  }
}

bool Scene::has_visible_models() const noexcept {
  for (const auto& m : _models) {
    if (m.visible && m.gl_mesh.is_valid()) {
      return true;
    }
  }
  return false;
}

i32 Scene::pick_bone_screen_space(f32 mouse_x, f32 mouse_y, f32 viewport_w, f32 viewport_h,
                                  f32 max_pixel_dist) const noexcept {
  if (!_posing_mode || viewport_w <= 0.0f || viewport_h <= 0.0f) {
    return -1;
  }
  const auto* model = selected_model();
  if (!model || !model->visible || !model->skeleton) {
    return -1;
  }

  const f32 aspect = viewport_w / viewport_h;
  const auto view = _camera.compute_view_matrix();
  const auto proj = _camera.compute_projection_matrix(aspect);
  const auto model_mat = model->compute_model_matrix();
  const auto mvp = proj * view * model_mat;

  const ran::Vec2f32 mouse_pt{mouse_x, mouse_y};
  const auto bones = model->skeleton->bones();

  i32 best_joint = -1;
  f32 best_joint_dist = max_pixel_dist;

  for (size_t i = 0; i < bones.size(); ++i) {
    const auto& b = bones[i];
    if (!b.visible) {
      continue;
    }
    const auto sp = project_to_screen(b.world_position, mvp, viewport_w, viewport_h);
    if (!sp.valid) {
      continue;
    }
    const f32 d = ran::length(mouse_pt - sp.pos);
    if (d < best_joint_dist) {
      best_joint_dist = d;
      best_joint = static_cast<i32>(i);
    }
  }

  if (best_joint >= 0) {
    return best_joint;
  }

  i32 best_seg = -1;
  f32 best_seg_dist = max_pixel_dist * 0.65f;
  for (size_t i = 0; i < bones.size(); ++i) {
    const auto& b = bones[i];
    if (!b.visible || b.parent_index < 0 || static_cast<size_t>(b.parent_index) >= bones.size()) {
      continue;
    }
    const auto& parent = bones[static_cast<size_t>(b.parent_index)];
    if (!parent.visible) {
      continue;
    }
    const auto sp_child = project_to_screen(b.world_position, mvp, viewport_w, viewport_h);
    const auto sp_parent = project_to_screen(parent.world_position, mvp, viewport_w, viewport_h);
    if (!sp_child.valid || !sp_parent.valid) {
      continue;
    }
    const f32 d = point_to_segment_dist(mouse_pt, sp_parent.pos, sp_child.pos);
    if (d < best_seg_dist) {
      best_seg_dist = d;
      best_seg = static_cast<i32>(b.parent_index);
    }
  }

  return best_seg;
}

void Scene::set_posing_mode(bool enabled) noexcept {
  _posing_mode = enabled;
  if (!_posing_mode) {
    for (auto& m : _models) {
      m.hovered_bone_idx = -1;
    }
  }
}

} // namespace mikugl
