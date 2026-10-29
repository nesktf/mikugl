#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in uvec4 a_bone_indices;
layout(location = 4) in vec4 a_bone_weights;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;
uniform mat4 u_light_view_proj;

out vec3 v_world_normal;
out vec3 v_view_normal;
out vec2 v_uv;
out vec3 v_world_pos;
out vec4 v_shadow_coord;

void main() {
  vec4 world_pos = u_model * vec4(a_position, 1.0);
  v_world_pos = world_pos.xyz;

  mat3 normal_matrix = mat3(transpose(inverse(u_model)));
  v_world_normal = normalize(normal_matrix * a_normal);

  mat3 view_normal_matrix = mat3(u_view) * normal_matrix;
  v_view_normal = normalize(view_normal_matrix * a_normal);

  v_uv = a_uv;

  vec4 light_clip = u_light_view_proj * world_pos;
  v_shadow_coord = vec4(light_clip.xyz / light_clip.w * 0.5 + 0.5, light_clip.w);

  gl_Position = u_proj * u_view * world_pos;
}
