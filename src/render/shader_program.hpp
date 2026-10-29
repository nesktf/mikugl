#pragma once

#include "core.hpp"

#include <ranmath/ran.hpp>
#include <string_view>

namespace mikugl {

enum class ShaderError : u8 {
  VertexCompilationFailed,
  FragmentCompilationFailed,
  LinkingFailed,
};

template<typename T>
using ShaderExpect = Expected<T, ShaderError>;

class ShaderProgram final {
private:
  struct create_t {};

public:
  static ShaderExpect<ShaderProgram> create(std::string_view vertex_src,
                                            std::string_view fragment_src);

public:
  ShaderProgram(create_t, u32 program_id) noexcept;
  ~ShaderProgram() noexcept;
  NTF_NO_COPY(ShaderProgram);
  ShaderProgram(ShaderProgram&& other) noexcept;
  ShaderProgram& operator=(ShaderProgram&& other) noexcept;

public:
  u32 id() const noexcept { return _program_id; }

  bool moved_from() const noexcept { return _program_id != 0; }

public:
  void set_uniform_mat4(std::string_view name, const ran::Mat4f32& mat) const noexcept;
  void set_uniform_vec4(std::string_view name, const ran::Vec4f32& vec) const noexcept;
  void set_uniform_vec3(std::string_view name, const ran::Vec3f32& vec) const noexcept;
  void set_uniform_float(std::string_view name, f32 value) const noexcept;
  void set_uniform_int(std::string_view name, i32 value) const noexcept;
  void bind() const noexcept;
  static void unbind() noexcept;

private:
  u32 _program_id;
};

} // namespace mikugl
