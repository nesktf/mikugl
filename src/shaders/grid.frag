#version 460 core

layout(location = 0) in vec3 v_near_point;
layout(location = 1) in vec3 v_far_point;

layout(location = 0) out vec4 out_color;

uniform mat4 u_view;
uniform mat4 u_proj;
uniform mat4 u_light_view_proj;
layout(binding = 3) uniform sampler2DShadow u_shadow_map;
uniform int u_has_shadow;

float compute_pcf_shadow(vec4 shadow_coord) {
  if (shadow_coord.z > 1.0 || shadow_coord.x < 0.0 || shadow_coord.x > 1.0 ||
      shadow_coord.y < 0.0 || shadow_coord.y > 1.0) {
    return 1.0;
  }

  float shadow = 0.0;
  vec2 texel_size = 1.0 / textureSize(u_shadow_map, 0);
  float bias = 0.0015;

  for (int x = -1; x <= 1; ++x) {
    for (int y = -1; y <= 1; ++y) {
      vec3 lookup = vec3(shadow_coord.xy + vec2(x, y) * texel_size, shadow_coord.z - bias);
      shadow += texture(u_shadow_map, lookup);
    }
  }
  return shadow / 9.0;
}

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

  float above_factor = smoothstep(0.0, 0.5, v_near_point.y);

  vec4 light_clip = u_light_view_proj * vec4(frag_pos, 1.0);
  vec4 shadow_coord = vec4(light_clip.xyz / light_clip.w * 0.5 + 0.5, light_clip.w);
  float shadow = (u_has_shadow != 0 && above_factor > 0.0)
                 ? mix(1.0, compute_pcf_shadow(shadow_coord), above_factor)
                 : 1.0;

  color.rgb *= mix(0.45, 1.0, shadow);

  float shadow_receiver = (1.0 - shadow) * 0.4 * fading * above_factor;
  vec4 shadow_color = vec4(0.06, 0.06, 0.07, 1.0);
  color = mix(color, shadow_color, shadow_receiver / max(color.a + shadow_receiver, 0.001));
  color.a = max(color.a, shadow_receiver);

  if (color.a < 0.01) {
    discard;
  }

  out_color = color;
}
