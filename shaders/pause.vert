#version 450

layout(location = 0) in vec4 quad_info;

layout(location = 0) out vec2 fs_tex_coord;

void main()
{
    gl_Position = vec4(quad_info.xy, 0.0, 1.0);
    fs_tex_coord = vec2(quad_info.zw);
}