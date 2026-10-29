#version 460 core

layout (location = 0) out vec3 v_near_point;
layout (location = 1) out vec3 v_far_point;

uniform mat4 u_view;
uniform mat4 u_proj;

vec3 unproject_point(float x, float y, float z, mat4 inv_view, mat4 inv_proj) {
  vec4 unproj = inv_view * inv_proj * vec4(x, y, z, 1.0);
  return unproj.xyz / unproj.w;
}

void main() {
  const vec2 p[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0,  1.0)
  );

  vec2 pos = p[gl_VertexID];
  mat4 inv_view = inverse(u_view);
  mat4 inv_proj = inverse(u_proj);

  v_near_point = unproject_point(pos.x, pos.y, 0.0, inv_view, inv_proj);
  v_far_point  = unproject_point(pos.x, pos.y, 1.0, inv_view, inv_proj);

  gl_Position = vec4(pos, 0.0, 1.0);
}
