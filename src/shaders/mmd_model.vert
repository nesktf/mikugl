#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in uvec4 a_bone_indices;
layout(location = 4) in vec4 a_bone_weights;

layout(std140, binding = 2) uniform BoneMatrices {
  mat4 u_bone_matrices[512];
};

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;
uniform mat4 u_light_view_proj;
uniform int u_skinning_enabled;

out vec3 v_world_normal;
out vec3 v_view_normal;
out vec2 v_uv;
out vec3 v_world_pos;
out vec4 v_shadow_coord;

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

  vec4 world_pos = u_model * local_pos;
  v_world_pos = world_pos.xyz;

  mat3 normal_matrix = mat3(transpose(inverse(u_model)));
  v_world_normal = normalize(normal_matrix * local_normal);

  mat3 view_normal_matrix = mat3(u_view) * normal_matrix;
  v_view_normal = normalize(view_normal_matrix * local_normal);

  v_uv = a_uv;

  vec4 light_clip = u_light_view_proj * world_pos;
  v_shadow_coord = vec4(light_clip.xyz / light_clip.w * 0.5 + 0.5, light_clip.w);

  gl_Position = u_proj * u_view * world_pos;
}
