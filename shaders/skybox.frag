#version 450


layout(binding = 3) uniform samplerCube skybox_texture;

layout(location = 1) in vec3 fs_texture_coordinate;

layout(location = 0) out vec4 final_color;

void main() {

    final_color = texture(skybox_texture, fs_texture_coordinate);
}
