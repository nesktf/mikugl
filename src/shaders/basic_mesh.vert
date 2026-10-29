#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in uvec4 a_bone_indices;
layout(location = 4) in vec4 a_bone_weights;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

out vec3 v_world_normal;
out vec2 v_uv;
out vec3 v_world_pos;

void main() {
  vec4 world_pos = u_model * vec4(a_position, 1.0);
  v_world_pos = world_pos.xyz;
  v_world_normal = mat3(transpose(inverse(u_model))) * a_normal;
  v_uv = a_uv;
  gl_Position = u_proj * u_view * world_pos;
}
