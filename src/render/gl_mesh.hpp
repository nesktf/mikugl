#pragma once

#include "core.hpp"
#include "model/model_data.hpp"

#include <glad/glad.h>

namespace mikugl {

class GlMesh final {
public:
  GlMesh() noexcept;
  ~GlMesh() noexcept;
  NTF_NO_COPY(GlMesh);
  GlMesh(GlMesh&& other) noexcept;
  GlMesh& operator=(GlMesh&& other) noexcept;

public:
  void upload(Span<const ModelVertex> vertices, Span<const u32> indices);
  void bind() const noexcept;
  void unbind() const noexcept;
  void draw_submesh(u32 index_offset, u32 index_count) const noexcept;
  void destroy() noexcept;

public:
  bool is_valid() const noexcept { return _vao != 0; }

  u32 vertex_count() const noexcept { return _vertex_count; }

  u32 index_count() const noexcept { return _index_count; }

private:
  GLuint _vao;
  GLuint _vbo;
  GLuint _ebo;
  u32 _vertex_count;
  u32 _index_count;
};

} // namespace mikugl
