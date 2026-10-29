#version 460 core

in vec3 v_world_normal;
in vec2 v_uv;
in vec3 v_world_pos;

uniform sampler2D u_diffuse_texture;
uniform vec4 u_diffuse_color;
uniform int u_has_texture;

out vec4 frag_color;

void main() {
  vec4 tex_color = texture(u_diffuse_texture, v_uv);
  vec4 base_color = u_has_texture != 0 ? (tex_color * u_diffuse_color) : u_diffuse_color;

  vec3 light_dir = normalize(vec3(0.5, 1.0, 0.7));
  vec3 normal = normalize(v_world_normal);
  if (!gl_FrontFacing) {
    normal = -normal;
  }

  float n_dot_l = max(dot(normal, light_dir), 0.0);
  float ambient = 0.45;
  float lighting = ambient + (1.0 - ambient) * n_dot_l;

  frag_color = vec4(base_color.rgb * lighting, base_color.a);
}
