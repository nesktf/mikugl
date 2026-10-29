#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 3) in uvec4 a_bone_indices;
layout(location = 4) in vec4 a_bone_weights;

layout(std140, binding = 2) uniform BoneMatrices {
  mat4 u_bone_matrices[512];
};

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;
uniform vec2 u_viewport_size;
uniform float u_edge_size;
uniform int u_skinning_enabled;

void main() {
  vec4 local_pos = vec4(a_position, 1.0);
  vec3 local_normal = a_normal;

  if (u_skinning_enabled != 0) {
    mat4 skin_matrix =
        u_bone_matrices[a_bone_indices.x] * a_bone_weights.x +
        u_bone_matrices[a_bone_indices.y] * a_bone_weights.y +
        u_bone_matrices[a_bone_indices.z] * a_bone_weights.z +
        u_bone_matrices[a_bone_indices.w] * a_bone_weights.w;
    local_pos = skin_matrix * vec4(a_position, 1.0);
    local_normal = mat3(skin_matrix) * a_normal;
  }

  vec4 clip_pos = u_proj * u_view * u_model * local_pos;

  mat3 normal_matrix = mat3(transpose(inverse(u_view * u_model)));
  vec3 view_normal = normalize(normal_matrix * local_normal);
  vec2 clip_normal = (u_proj * vec4(view_normal, 0.0)).xy;

  vec2 screen_normal = normalize(clip_normal * u_viewport_size);
  vec2 offset = screen_normal / u_viewport_size * (u_edge_size * 1.5) * clip_pos.w * 2.0;

  clip_pos.xy += offset;
  gl_Position = clip_pos;
}
