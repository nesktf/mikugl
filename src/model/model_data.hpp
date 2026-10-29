#pragma once

#include "core.hpp"

#include <ranmath/ran.hpp>

#include <array>
#include <string>

namespace mikugl {

struct ModelVertex {
  ran::Vec3f32 position;
  ran::Vec3f32 normal;
  ran::Vec2f32 uv;
  std::array<u16, 4> bone_indices;
  std::array<f32, 4> bone_weights;
};

struct ModelSubmesh {
  std::string name;
  u32 material_index;
  u32 index_offset;
  u32 index_count;
};

struct ModelMaterial {
  std::string name;
  ran::Vec4f32 diffuse;
  ran::Vec3f32 specular;
  f32 shininess;
  ran::Vec3f32 ambient;
  i32 texture_index;
  i32 sphere_texture_index;
  i32 toon_index;
  u8 sphere_mode;
  bool is_double_sided;
  bool has_edge;
  f32 edge_size;
  ran::Vec4f32 edge_color;
};

struct TextureCpuBuffer {
  std::string path;
  i32 width;
  i32 height;
  i32 channels;
  UniqueArray<u8> pixels;
  bool valid;
};

struct ModelData {
  std::string name;
  Vec<ModelVertex> vertices;
  Vec<u32> indices;
  Vec<ModelSubmesh> submeshes;
  Vec<ModelMaterial> materials;
  Vec<TextureCpuBuffer> textures;
};

enum class ModelLoadError : u8 {
  FileNotFound,
  InvalidSignature,
  CorruptData,
  OutOfMemory,
  UnsupportedFormat,
  Unknown,
};

template<typename T>
using ModelExpect = Expected<T, ModelLoadError>;

} // namespace mikugl
