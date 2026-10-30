#include "model/model_loader.hpp"

#include <mmdgltf/mmdgltf.hpp>
#include <stb_image.h>

namespace mikugl {

Expected<ModelData, ModelLoadError> ModelLoader::load_file(std::string_view path) {
  const auto map_mmdgltf_status = [](mmdgltf_Status status) -> ModelLoadError {
    switch (status) {
      case MMDGLTF_ERROR_IO:
        return ModelLoadError::FileNotFound;
      case MMDGLTF_ERROR_INVALID_SIGNATURE:
        return ModelLoadError::InvalidSignature;
      case MMDGLTF_ERROR_CORRUPT_DATA:
        return ModelLoadError::CorruptData;
      case MMDGLTF_ERROR_OUT_OF_MEMORY:
        return ModelLoadError::OutOfMemory;
      case MMDGLTF_ERROR_UNSUPPORTED_VERSION:
        return ModelLoadError::UnsupportedFormat;
      default:
        return ModelLoadError::Unknown;
    }
  };

  const auto pmx = mmdgltf::PmxModel::load_file(path.data(), path.size());
  if (!pmx) {
    return Unexpected(map_mmdgltf_status(pmx.code()));
  }

  mmdgltf_ModelConfig config{};
  mmdgltf_model_config_default(&config);
  config.coordinate_system = MMDGLTF_COORDS_GLTF_RIGHT_HANDED;
  config.load_flags = MMDGLTF_LOAD_ALL;
  config.strict_textures = 0;

  const auto model = mmdgltf::Model::create(*pmx, &config);
  if (!model) {
    return Unexpected(map_mmdgltf_status(model.code()));
  }

  const auto vbufs = model->vertex_buffers();
  if (!vbufs) {
    return Unexpected(ModelLoadError::CorruptData);
  }

  const auto indices = model->indices();
  if (!indices) {
    return Unexpected(ModelLoadError::CorruptData);
  }

  ModelExpect<ModelData> result{ntf::in_place};
  const auto info = pmx->model_info().value_or(mmdgltf::PmxModelInfo{});
  if (info.name_en && info.name_en[0] != '\0') {
    result->name = info.name_en;
  } else if (info.name_jp && info.name_jp[0] != '\0') {
    result->name = info.name_jp;
  }

  const size_t vcount = vbufs->vertex_count;
  result->vertices.resize(vcount);

  for (size_t i = 0; i < vcount; ++i) {
    ModelVertex& v = result->vertices[i];
    v.position = ran::Vec3f32{
      vbufs->positions[i * 3 + 0],
      vbufs->positions[i * 3 + 1],
      vbufs->positions[i * 3 + 2],
    };
    v.normal = ran::Vec3f32{
      vbufs->normals[i * 3 + 0],
      vbufs->normals[i * 3 + 1],
      vbufs->normals[i * 3 + 2],
    };
    v.uv = ran::Vec2f32{
      vbufs->uvs[i * 2 + 0],
      vbufs->uvs[i * 2 + 1],
    };

    if (vbufs->joints) {
      v.bone_indices[0] = vbufs->joints[i * 4 + 0];
      v.bone_indices[1] = vbufs->joints[i * 4 + 1];
      v.bone_indices[2] = vbufs->joints[i * 4 + 2];
      v.bone_indices[3] = vbufs->joints[i * 4 + 3];
    } else {
      v.bone_indices.fill(0);
    }

    if (vbufs->weights) {
      v.bone_weights[0] = vbufs->weights[i * 4 + 0];
      v.bone_weights[1] = vbufs->weights[i * 4 + 1];
      v.bone_weights[2] = vbufs->weights[i * 4 + 2];
      v.bone_weights[3] = vbufs->weights[i * 4 + 3];
    } else {
      v.bone_weights = {1.0f, 0.0f, 0.0f, 0.0f};
    }
  }

  if (!indices->empty()) {
    result->indices.insert(result->indices.begin(), indices->begin(), indices->end());
  }

  if (const auto materials = model->materials(); materials && !materials->empty()) {
    result->materials.reserve(materials->size());
    result->submeshes.reserve(materials->size());

    u32 i = 0;
    u32 current_offset = 0;
    for (const auto& src_mat : *materials) {
      ModelMaterial dst_mat{};
      if (src_mat.name_en && src_mat.name_en[0] != '\0') {
        dst_mat.name = src_mat.name_en;
      } else if (src_mat.name_jp) {
        dst_mat.name = src_mat.name_jp;
      }

      dst_mat.diffuse = ran::Vec4f32{
        src_mat.diffuse.r,
        src_mat.diffuse.g,
        src_mat.diffuse.b,
        src_mat.diffuse.a,
      };
      dst_mat.specular = ran::Vec3f32{
        src_mat.specular.r,
        src_mat.specular.g,
        src_mat.specular.b,
      };
      dst_mat.shininess = src_mat.shininess;
      dst_mat.ambient = ran::Vec3f32{
        src_mat.ambient.r,
        src_mat.ambient.g,
        src_mat.ambient.b,
      };
      dst_mat.texture_index = src_mat.texture_index;
      dst_mat.sphere_texture_index = src_mat.sphere_texture_index;
      dst_mat.toon_index = src_mat.toon_index;
      dst_mat.toon_sharing_flag = src_mat.toon_sharing_flag;
      dst_mat.sphere_mode = static_cast<u8>(src_mat.sphere_mode);
      dst_mat.is_double_sided = (src_mat.draw_flags & MMDGLTF_MATERIAL_DOUBLE_SIDED) != 0;
      dst_mat.has_edge = (src_mat.draw_flags & MMDGLTF_MATERIAL_EDGE) != 0;
      dst_mat.edge_size = src_mat.edge_size;
      dst_mat.edge_color = ran::Vec4f32{
        src_mat.edge_color.r,
        src_mat.edge_color.g,
        src_mat.edge_color.b,
        src_mat.edge_color.a,
      };

      result->materials.push_back(std::move(dst_mat));

      ModelSubmesh submesh{};
      submesh.name = result->materials.back().name;
      submesh.material_index = i++;
      submesh.index_offset = current_offset;
      submesh.index_count = src_mat.index_count;
      current_offset += src_mat.index_count;

      result->submeshes.push_back(std::move(submesh));
    }
  }

  if (const auto images = model->images(); images && !images->empty()) {
    result->textures.reserve(images->size());
    for (const auto& img : *images) {
      TextureCpuBuffer tex{};
      if (img.path) {
        tex.path = img.path;
      }

      if (img.data && img.size > 0) {
        int w = 0;
        int h = 0;
        int comp = 0;
        static constexpr int req_comp = 4;
        u8* decoded =
          stbi_load_from_memory(img.data, static_cast<int>(img.size), &w, &h, &comp, req_comp);

        if (decoded) {
          tex.width = w;
          tex.height = h;
          tex.channels = req_comp;
          const auto byte_count = static_cast<size_t>(w * h * req_comp);
          tex.pixels = make_unique_array<u8>(uninitialized, byte_count);
          std::memcpy(tex.pixels.data(), decoded, byte_count);
          stbi_image_free(decoded);
          tex.valid = true;
        } else {
          tex.valid = false;
        }
      } else {
        tex.valid = false;
      }

      result->textures.push_back(std::move(tex));
    }
  }

  if (const auto bones = model->bones(); bones && !bones->empty()) {
    result->bones.reserve(bones->size());
    for (const auto& b : *bones) {
      ModelBone mb{};
      if (b.name_en && b.name_en[0] != '\0') {
        mb.name_en = b.name_en;
      }
      if (b.name_jp && b.name_jp[0] != '\0') {
        mb.name_jp = b.name_jp;
      }
      mb.position = ran::Vec3f32{b.position.x, b.position.y, b.position.z};
      mb.parent_index = b.parent_index;
      mb.transform_order = b.transform_order;
      mb.flags = b.flags;
      mb.offset = ran::Vec3f32{b.offset.x, b.offset.y, b.offset.z};
      mb.connection_bone_index = b.connection_bone_index;
      mb.ik_target_index = b.ik_target_index;
      mb.ik_loop_count = b.ik_loop_count;
      mb.ik_limit_angle = b.ik_limit_angle;
      if (b.ik_links) {
        miku_assert(b.ik_link_count > 0);
        mb.ik_links.reserve(b.ik_link_count);
        for (u32 k = 0; k < b.ik_link_count; ++k) {
          const auto& link = b.ik_links[k];
          ModelIkLink ml{};
          ml.bone_index = link.bone_index;
          ml.has_limits = (link.has_limits != 0);
          ml.min_angle = ran::Vec3f32{link.min_angle.x, link.min_angle.y, link.min_angle.z};
          ml.max_angle = ran::Vec3f32{link.max_angle.x, link.max_angle.y, link.max_angle.z};
          mb.ik_links.push_back(ml);
        }
      }
      result->bones.push_back(std::move(mb));
    }
  }

  return result;
}

} // namespace mikugl
