#version 450

layout(binding = 0) uniform sampler2D pause_texture;

layout(location = 0) in vec2 fs_tex_coord;


layout(location = 0) out vec4 final_color;

void main()
{
    final_color = texture(pause_texture, fs_tex_coord);
    //final_color.a = 1.0f;
}