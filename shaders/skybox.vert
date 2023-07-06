#version 450

layout(binding = 0, std140) uniform Camera {
	mat4 projection;
	mat4 view;
	vec3 position;
} camera;


layout(binding = 2, std140) uniform Object {
	mat4 model_matrix;
	vec4 ambient_color;
	vec4 diffuse_color;
	vec4 specular_color;
} object;

layout(location = 0) in vec3 position;

layout(location = 1) out vec3 fs_texture_coordinate;

void main()
{
	fs_texture_coordinate = position;
    gl_Position = camera.projection * camera.view * object.model_matrix * vec4(position, 1.0);
}
