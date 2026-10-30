#include "render/gl_mesh.hpp"

#include <cstddef>
#include <utility>

namespace mikugl {

GlMesh::GlMesh() noexcept : _vao(0), _vbo(0), _ebo(0), _vertex_count(0), _index_count(0) {}

GlMesh::~GlMesh() noexcept {
  destroy();
}

GlMesh::GlMesh(GlMesh&& other) noexcept :
    _vao(std::exchange(other._vao, 0)), _vbo(std::exchange(other._vbo, 0)),
    _ebo(std::exchange(other._ebo, 0)), _vertex_count(std::exchange(other._vertex_count, 0)),
    _index_count(std::exchange(other._index_count, 0)) {}

GlMesh& GlMesh::operator=(GlMesh&& other) noexcept {
  if (this != &other) {
    destroy();
    _vao = std::exchange(other._vao, 0);
    _vbo = std::exchange(other._vbo, 0);
    _ebo = std::exchange(other._ebo, 0);
    _vertex_count = std::exchange(other._vertex_count, 0);
    _index_count = std::exchange(other._index_count, 0);
  }
  return *this;
}

void GlMesh::destroy() noexcept {
  if (_ebo) {
    glDeleteBuffers(1, &_ebo);
    _ebo = 0;
  }
  if (_vbo) {
    glDeleteBuffers(1, &_vbo);
    _vbo = 0;
  }
  if (_vao) {
    glDeleteVertexArrays(1, &_vao);
    _vao = 0;
  }
  _vertex_count = 0;
  _index_count = 0;
}

void GlMesh::upload(Span<const ModelVertex> vertices, Span<const u32> indices) {
  destroy();

  if (vertices.empty() || indices.empty()) {
    return;
  }

  _vertex_count = static_cast<u32>(vertices.size());
  _index_count = static_cast<u32>(indices.size());

  glGenVertexArrays(1, &_vao);
  glGenBuffers(1, &_vbo);
  glGenBuffers(1, &_ebo);

  glBindVertexArray(_vao);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size_bytes()), vertices.data(),
               GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size_bytes()),
               indices.data(), GL_STATIC_DRAW);

  // position
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex),
                        reinterpret_cast<const void*>(offsetof(ModelVertex, position)));

  // normal
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex),
                        reinterpret_cast<const void*>(offsetof(ModelVertex, normal)));

  // UV
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ModelVertex),
                        reinterpret_cast<const void*>(offsetof(ModelVertex, uv)));

  // bone indices (u16)
  glEnableVertexAttribArray(3);
  glVertexAttribIPointer(3, 4, GL_UNSIGNED_SHORT, sizeof(ModelVertex),
                         reinterpret_cast<const void*>(offsetof(ModelVertex, bone_indices)));

  // bone weights (f32)
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(ModelVertex),
                        reinterpret_cast<const void*>(offsetof(ModelVertex, bone_weights)));

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GlMesh::update_vertices(Span<const ModelVertex> vertices) noexcept {
  if (!_vbo || vertices.empty()) {
    return;
  }
  miku_assert(vertices.size() == _vertex_count && "Vertex count mismatch in update_vertices");
  glBindBuffer(GL_ARRAY_BUFFER, _vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(vertices.size_bytes()),
                  vertices.data());
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GlMesh::bind() const noexcept {
  if (_vao) {
    glBindVertexArray(_vao);
  }
}

void GlMesh::unbind() const noexcept {
  glBindVertexArray(0);
}

void GlMesh::draw_submesh(u32 index_offset, u32 index_count) const noexcept {
  if (!_vao || index_count == 0) {
    return;
  }
  glDrawElements(
    GL_TRIANGLES, static_cast<GLsizei>(index_count), GL_UNSIGNED_INT,
    reinterpret_cast<const void*>(static_cast<uintptr_t>(index_offset) * sizeof(u32)));
}

} // namespace mikugl
