#version 460 core

in vec3 v_world_normal;
in vec3 v_view_normal;
in vec2 v_uv;
in vec3 v_world_pos;
in vec4 v_shadow_coord;

layout(binding = 0) uniform sampler2D u_diffuse_texture;
layout(binding = 1) uniform sampler2D u_sphere_texture;
layout(binding = 2) uniform sampler2D u_toon_texture;
layout(binding = 3) uniform sampler2DShadow u_shadow_map;

uniform vec4 u_diffuse_color;
uniform vec3 u_specular_color;
uniform vec3 u_ambient_color;
uniform float u_shininess;

uniform int u_has_diffuse;
uniform int u_has_sphere;
uniform int u_sphere_mode; // 0=none, 1=multiply, 2=add
uniform int u_has_toon;
uniform int u_has_shadow;

uniform vec3 u_light_dir;
uniform vec3 u_light_color;
uniform vec3 u_camera_pos;

out vec4 frag_color;

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

void main() {
  vec4 tex_color = u_has_diffuse != 0 ? texture(u_diffuse_texture, v_uv) : vec4(1.0);
  vec4 base_color = tex_color * u_diffuse_color;

  if (u_has_sphere != 0 && u_sphere_mode > 0) {
    vec2 sphere_uv = v_view_normal.xy * 0.5 + 0.5;
    sphere_uv.y = 1.0 - sphere_uv.y;
    vec4 sphere_color = texture(u_sphere_texture, sphere_uv);
    if (u_sphere_mode == 1) {
      base_color.rgb *= sphere_color.rgb;
    } else if (u_sphere_mode == 2) {
      base_color.rgb += sphere_color.rgb;
    }
  }

  vec3 normal = normalize(v_world_normal);
  if (!gl_FrontFacing) {
    normal = -normal;
  }
  vec3 light_dir = normalize(u_light_dir);
  float n_dot_l = dot(normal, light_dir);

  vec3 toon_factor = vec3(1.0);
  if (u_has_toon != 0) {
    float toon_u = clamp(0.5 * n_dot_l + 0.5, 0.01, 0.99);
    toon_factor = texture(u_toon_texture, vec2(toon_u, 0.5)).rgb;
  } else {
    float half_lambert = clamp(0.5 * n_dot_l + 0.5, 0.0, 1.0);
    toon_factor = vec3(half_lambert);
  }

  float shadow = 1.0;
  if (u_has_shadow != 0) {
    shadow = compute_pcf_shadow(v_shadow_coord);
  }

  vec3 lighting = mix(toon_factor * 0.5, toon_factor, shadow) * u_light_color;
  lighting = max(lighting, u_ambient_color);

  vec3 view_dir = normalize(u_camera_pos - v_world_pos);
  vec3 half_dir = normalize(light_dir + view_dir);
  float spec_factor = pow(max(dot(normal, half_dir), 0.0), max(u_shininess, 1.0));
  vec3 specular = u_specular_color * u_light_color * spec_factor * shadow;

  vec3 final_rgb = base_color.rgb * lighting + specular;
  frag_color = vec4(final_rgb, base_color.a);
}
