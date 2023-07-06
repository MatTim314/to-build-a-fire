#version 450

layout(binding = 0, std140) uniform Camera {
    mat4 projection;
    mat4 view;
    vec3 position;
}
camera;

layout(binding = 1, std140) uniform Light {
    vec4 position;
    vec4 ambient_color;
    vec4 diffuse_color;
    vec4 specular_color;
}
light;

layout(binding = 2, std140) uniform Object {
    mat4 model_matrix;

    vec4 ambient_color;
    vec4 diffuse_color;
    vec4 specular_color;
}
object;

layout(binding = 4, std140) uniform Moon {
    vec4 position;
    vec4 ambient_color;
    vec4 diffuse_color;
    vec4 specular_color;
}
moon;

layout(binding = 5, std140) uniform Fire {
    vec4 position;
    vec4 ambient_color;
    vec4 diffuse_color;
    vec4 specular_color;
}
fire;

layout(location = 3) uniform bool has_texture = false;

layout(binding = 3) uniform sampler2D albedo_texture;
layout(location = 4) uniform float light_ubo_intensity;
layout(location = 5) uniform bool selected;
layout(location = 6) uniform bool fire_lit;
layout(location = 7) uniform float fire_radius;
layout(location = 8) uniform bool flashlight_lit;

layout(location = 0) in vec3 fs_position;
layout(location = 1) in vec3 fs_normal;
layout(location = 2) in vec2 fs_texture_coordinate;

layout(location = 0) out vec4 final_color;

void main() {
    //Flashlight
    float cut_off = -0.923403;
    //float theta = dot(lightDir, normalize(-light.direction));


    vec3 light_vector = light.position.xyz - fs_position;
    float distance_from_light = length(light_vector);
    float distance_factor = (-distance_from_light/3)+1;
    //vec3 light_vector = light.position.xyz;
    vec3 L = normalize(light_vector);
    vec3 L_moon = normalize(moon.position.xyz - fs_position.xyz);
    vec3 L_fire = normalize(fire.position.xyz - fs_position.xyz);
    float fire_distance = length(fire.position.xyz - fs_position.xyz);
    float fire_coeficient = -pow(fire_distance,2) / pow(fire_radius, 2) + 1;

    vec3 N = normalize(fs_normal);
    vec3 E = normalize(camera.position - fs_position);
    vec3 H = normalize(L + E);

    float NdotL = max(dot(N, L), 0.0);
    float NdotL_moon = max(dot(N,  L_moon), 0.0);
    float NdotL_fire = max(dot(N, L_fire), 0.0);
    float NdotH = max(dot(N, H), 0.0001);
     
    vec3 ambient = object.ambient_color.rgb * light.ambient_color.rgb * moon.ambient_color.rgb * (has_texture ? texture(albedo_texture, fs_texture_coordinate).rgb : vec3(1.0));
    vec3 diffuse = object.diffuse_color.rgb * (has_texture ? texture(albedo_texture, fs_texture_coordinate).rgb : vec3(1.0)) *
                   light.diffuse_color.rgb;
    vec3 specular = object.specular_color.rgb * light.specular_color.rgb;

    vec3 moon_diffuse = object.diffuse_color.rgb * (has_texture ? texture(albedo_texture, fs_texture_coordinate).rgb : vec3(1.0)) *
                   moon.diffuse_color.rgb;
    vec3 fire_diffuse = object.diffuse_color.rgb * (has_texture ? texture(albedo_texture, fs_texture_coordinate).rgb : vec3(1.0)) *
                   fire.diffuse_color.rgb * fire_coeficient;

    vec3 color = ambient.rgb + NdotL * diffuse.rgb + NdotL_moon * moon_diffuse.rgb + pow(NdotH, object.specular_color.w) * specular;
    //Inside cabin
    if (fs_position.x > 11.9151 && fs_position.x < 12.8703 && fs_position.z < -10.7092 && fs_position.z > -11.843){
        color = ambient.rgb + NdotL * diffuse.rgb + pow(NdotH, object.specular_color.w) * specular;
        if (fire_lit){
            color = ambient.rgb + NdotL * diffuse.rgb + NdotL_fire * fire_diffuse.rgb + pow(NdotH, object.specular_color.w) * specular;
        }
        
    }
    // Return distance factor
    color = color * distance_factor * light_ubo_intensity;
    //color = color * distance_factor * light_ubo_intensity;
    //vec3 color = light.ambient_color.rbg;
    // if (light.position.w == 1.0) {
    //     color /= (dot(light_vector, light_vector));
    // }

    // color = color / (color + 1.0);       // tone mapping
    // color = pow(color, vec3(1.0 / 2.2)); // gamma correction
    if (selected){
        vec3 greenish_hue = color.rgb;
        final_color = vec4(color.r + 0.10, color.g +0.20, color.b + 0.10, 1.0);
    
    }else{
        final_color = vec4(color, 1.0);
    }
    
}
