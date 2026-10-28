#include "render/shader_program.hpp"

#include <glad/glad.h>

#include <cstdio>
#include <string>

namespace mikugl {

ShaderExpect<ShaderProgram> ShaderProgram::create(std::string_view vertex_src,
                                                  std::string_view fragment_src) {
  const auto compile_stage = [](u32 stage_type, std::string_view src,
                                ShaderError err) -> ShaderExpect<u32> {
    const auto shader = glCreateShader(stage_type);
    const char* src_ptr = src.data();
    const auto src_len = static_cast<i32>(src.size());
    glShaderSource(shader, 1, &src_ptr, &src_len);
    glCompileShader(shader);

    i32 success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
      char log[1024];
      glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
      fprintf(stderr, "Shader compilation error: %s\n", log);
      glDeleteShader(shader);
      return Unexpected<ShaderError>(err);
    }

    return (u32)shader;
  };

  const auto vert_shader =
    compile_stage(GL_VERTEX_SHADER, vertex_src, ShaderError::VertexCompilationFailed);
  if (!vert_shader) {
    return Unexpected<ShaderError>(vert_shader.error());
  }
  const DeferFn vert_defer = [&]() {
    glDeleteShader(*vert_shader);
  };

  const auto frag_shader =
    compile_stage(GL_FRAGMENT_SHADER, fragment_src, ShaderError::FragmentCompilationFailed);
  if (!frag_shader) {
    return Unexpected<ShaderError>(frag_shader.error());
  }
  const DeferFn frag_defer = [&]() {
    glDeleteShader(*frag_shader);
  };

  const u32 program = glCreateProgram();
  glAttachShader(program, *vert_shader);
  glAttachShader(program, *frag_shader);
  glLinkProgram(program);
  glDetachShader(program, *vert_shader);
  glDetachShader(program, *frag_shader);

  i32 success = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    char log[1024];
    glGetProgramInfoLog(program, sizeof(log), nullptr, log);
    fprintf(stderr, "Shader link error: %s\n", log);
    glDeleteProgram(program);
    return Unexpected<ShaderError>(ShaderError::LinkingFailed);
  }

  return {ntf::in_place, create_t(), program};
}

ShaderProgram::ShaderProgram(create_t, u32 program_id) noexcept : _program_id(program_id) {}

ShaderProgram::~ShaderProgram() noexcept {
  if (_program_id != 0) {
    glDeleteProgram(_program_id);
    _program_id = 0;
  }
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept : _program_id(other._program_id) {
  other._program_id = 0;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
  if (this != &other) {
    if (_program_id != 0) {
      glDeleteProgram(_program_id);
    }
    _program_id = other._program_id;
    other._program_id = 0;
  }
  return *this;
}

void ShaderProgram::bind() const noexcept {
  glUseProgram(_program_id);
}

void ShaderProgram::unbind() noexcept {
  glUseProgram(0);
}

void ShaderProgram::set_uniform_mat4(std::string_view name,
                                     const ran::Mat4f32& mat) const noexcept {
  const std::string name_str(name);
  const i32 loc = glGetUniformLocation(_program_id, name_str.c_str());
  if (loc >= 0) {
    glUniformMatrix4fv(loc, 1, GL_FALSE, mat.data());
  }
}

void ShaderProgram::set_uniform_vec3(std::string_view name,
                                     const ran::Vec3f32& vec) const noexcept {
  const std::string name_str(name);
  const i32 loc = glGetUniformLocation(_program_id, name_str.c_str());
  if (loc >= 0) {
    glUniform3fv(loc, 1, vec.data());
  }
}

void ShaderProgram::set_uniform_float(std::string_view name, f32 value) const noexcept {
  const std::string name_str(name);
  const i32 loc = glGetUniformLocation(_program_id, name_str.c_str());
  if (loc >= 0) {
    glUniform1f(loc, value);
  }
}

void ShaderProgram::set_uniform_int(std::string_view name, i32 value) const noexcept {
  const std::string name_str(name);
  const i32 loc = glGetUniformLocation(_program_id, name_str.c_str());
  if (loc >= 0) {
    glUniform1i(loc, value);
  }
}

} // namespace mikugl
