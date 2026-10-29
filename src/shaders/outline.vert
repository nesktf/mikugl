#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;
uniform vec2 u_viewport_size;
uniform float u_edge_size;

void main() {
  vec4 clip_pos = u_proj * u_view * u_model * vec4(a_position, 1.0);

  mat3 normal_matrix = mat3(transpose(inverse(u_view * u_model)));
  vec3 view_normal = normalize(normal_matrix * a_normal);
  vec2 clip_normal = (u_proj * vec4(view_normal, 0.0)).xy;

  vec2 screen_normal = normalize(clip_normal * u_viewport_size);
  vec2 offset = screen_normal / u_viewport_size * (u_edge_size * 1.5) * clip_pos.w * 2.0;

  clip_pos.xy += offset;
  gl_Position = clip_pos;
}
