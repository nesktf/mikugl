#version 460 core

layout (location = 0) in vec3 v_near_point;
layout (location = 1) in vec3 v_far_point;

layout (location = 0) out vec4 out_color;

uniform mat4 u_view;
uniform mat4 u_proj;

vec4 compute_grid(vec3 frag_pos, float scale, bool draw_axes) {
  vec2 coord = frag_pos.xz * scale;
  vec2 derivative = fwidth(coord);
  vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
  float line = min(grid.x, grid.y);
  float minimum_z = min(derivative.y, 1.0);
  float minimum_x = min(derivative.x, 1.0);

  vec4 color = vec4(0.32, 0.32, 0.35, 1.0 - min(line, 1.0));

  if (draw_axes) {
    if (frag_pos.x > -0.1 * minimum_x && frag_pos.x < 0.1 * minimum_x) {
      color = vec4(0.28, 0.52, 0.92, 1.0);
    }
    if (frag_pos.z > -0.1 * minimum_z && frag_pos.z < 0.1 * minimum_z) {
      color = vec4(0.92, 0.32, 0.32, 1.0);
    }
  }

  return color;
}

float compute_depth(vec3 frag_pos) {
  vec4 clip = u_proj * u_view * vec4(frag_pos, 1.0);
  return (clip.z / clip.w) * 0.5 + 0.5;
}

void main() {
  float t = -v_near_point.y / (v_far_point.y - v_near_point.y);
  if (t <= 0.0) {
    discard;
  }

  vec3 frag_pos = v_near_point + t * (v_far_point - v_near_point);
  gl_FragDepth = compute_depth(frag_pos);

  float linear_depth = length(frag_pos - v_near_point);
  float fading = max(0.0, (1.0 - linear_depth / 400.0));

  vec4 minor_grid = compute_grid(frag_pos, .5, false);
  vec4 major_grid = compute_grid(frag_pos, 1.0, true);

  vec4 color = mix(minor_grid, major_grid, major_grid.a);
  color.a *= fading;

  if (color.a < 0.01) {
    discard;
  }

  out_color = color;
}
