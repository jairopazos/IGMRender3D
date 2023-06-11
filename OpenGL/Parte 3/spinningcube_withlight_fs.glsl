#version 130

struct Material {
  sampler2D diffuse;
  vec3 specular;
  float shininess;
};

struct Light {
  vec3 position;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

out vec4 frag_col;

in vec3 frag_3Dpos;
in vec3 vs_normal;
in vec2 vs_tex_coord;

uniform Material material;
uniform Light light, second_light;
uniform vec3 view_pos, second_view_pos;

void main() {
  // Ambient
  vec3 ambient = light.ambient * vec3(texture(material.diffuse, vs_tex_coord));

  vec3 light_dir = normalize(light.position - frag_3Dpos);
  
  // Diffuse
  float diff = max(dot(vs_normal, light_dir), 0.0);
  vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, vs_tex_coord));

  // Specular
  vec3 view_dir = normalize(view_pos - frag_3Dpos);
  vec3 reflect_dir = reflect(-light_dir, vs_normal);
  float spec = pow(max(dot(view_dir, reflect_dir), 0.0), material.shininess);
  vec3 specular = light.specular * spec * material.specular;


  // Ambient second light
  vec3 second_ambient = second_light.ambient * vec3(texture(material.diffuse, vs_tex_coord));

  vec3 second_light_dir = normalize(second_light.position - frag_3Dpos);
  
  // Diffuse second light
  float second_diff = max(dot(vs_normal, second_light_dir), 0.0);
  vec3 second_diffuse = second_light.diffuse * second_diff * vec3(texture(material.diffuse, vs_tex_coord));

  // Specular
  vec3 second_view_dir = normalize(second_view_pos - frag_3Dpos);
  vec3 second_reflect_dir = reflect(-second_light_dir, vs_normal);
  float second_spec = pow(max(dot(second_view_dir, second_reflect_dir), 0.0), material.shininess);
  vec3 second_specular = second_light.specular * second_spec * material.specular;

  vec3 result = ambient + diffuse + specular + second_ambient + second_diffuse + second_specular;
  frag_col = vec4(result, 1.0);
}
