#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 3) in uvec4 a_bone_indices;
layout(location = 4) in vec4 a_bone_weights;

layout(std140, binding = 2) uniform BoneMatrices {
  mat4 u_bone_matrices[512];
};

uniform mat4 u_model;
uniform mat4 u_light_view;
uniform mat4 u_light_proj;
uniform int u_skinning_enabled;

void main() {
  vec4 local_pos = vec4(a_position, 1.0);

  if (u_skinning_enabled != 0) {
    mat4 skin_matrix =
        u_bone_matrices[a_bone_indices.x] * a_bone_weights.x +
        u_bone_matrices[a_bone_indices.y] * a_bone_weights.y +
        u_bone_matrices[a_bone_indices.z] * a_bone_weights.z +
        u_bone_matrices[a_bone_indices.w] * a_bone_weights.w;
    local_pos = skin_matrix * vec4(a_position, 1.0);
  }

  gl_Position = u_light_proj * u_light_view * u_model * local_pos;
}
