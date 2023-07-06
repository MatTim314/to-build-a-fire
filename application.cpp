#include "application.hpp"

#include <memory>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <fstream>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <fmod_studio.hpp>

using std::make_shared;

FMOD::Studio::System *fmod_system = NULL;

Environment environment = FOREST;
Surface surface = SNOW;

size_t Item::nextID = 0;

bool fire_lit = false;
const auto cave_pos = glm::vec3(-13.38, 0.21, -9.99001);
const auto cabin_pos = glm::vec3(12.39, 0.21, -11.52);
const auto fire_pos = glm::vec3(12.6481, 0.141635, -11.585);
const float distance_to_branch = 0.2f;
const float player_height = 0.09f;

bool DEBUG = false;

// sounds
namespace
{
    void fmod_init()
    {
        FMOD::Studio::System::create(&fmod_system);
        fmod_system->initialize(1024, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, NULL);

        FMOD::Studio::Bank *master_bank = NULL;
        FMOD::Studio::Bank *strings_bank = NULL;
        FMOD::Studio::Bank *fx_bank = NULL;
        FMOD::Studio::Bank *character_bank = NULL;
        FMOD::Studio::Bank *ambience_bank = NULL;
        FMOD::Studio::Bank *music_bank = NULL;

        auto fmod_loaded = fmod_system->loadBankFile("./FMOD_banks/Desktop/Master.bank", FMOD_STUDIO_LOAD_BANK_NORMAL, &master_bank);
        fmod_system->loadBankFile("./FMOD_banks/Desktop/Master.strings.bank", FMOD_STUDIO_LOAD_BANK_NORMAL, &strings_bank);
        fmod_system->loadBankFile("./FMOD_banks/Desktop/FX.bank", FMOD_STUDIO_LOAD_BANK_NORMAL, &fx_bank);
        fmod_system->loadBankFile("./FMOD_banks/Desktop/Character.bank", FMOD_STUDIO_LOAD_BANK_NORMAL, &character_bank);
        fmod_system->loadBankFile("./FMOD_banks/Desktop/Ambience.bank", FMOD_STUDIO_LOAD_BANK_NORMAL, &ambience_bank);
        fmod_system->loadBankFile("./FMOD_banks/Desktop/Music.bank", FMOD_STUDIO_LOAD_BANK_NORMAL, &music_bank);
        if (DEBUG)
        {
            if (fmod_loaded == FMOD_OK)
            {
                std::cout << "FMOD master bank loaded."
                          << "\n";
            }
            else
            {
                std::cout << "Failed to load FMOD master bank."
                          << "\n";
            }
        }
    }

    void start_default_sounds()
    {
        // start default ambience
        FMOD::Studio::EventDescription *event_desc = NULL;
        FMOD::Studio::EventInstance *event_inst = NULL;
        fmod_system->getEvent("event:/Ambient/ForestAmbience", &event_desc);
        event_desc->createInstance(&event_inst);
        // fmod_system->setParameterByName("Wind", 1.0f);
        event_inst->start();
        // start breathing
        fmod_system->getEvent("event:/Character/Breathing", &event_desc);
        event_desc->createInstance(&event_inst);
        // event_inst->setParameterByName("BreathIntensity", 0.1f);
        event_inst->start();
        // default snapshot
        fmod_system->getEvent("snapshot:/Forest", &event_desc);
        event_desc->createInstance(&event_inst);
        event_inst->start();
        // music
        fmod_system->getEvent("event:/Music/CabinMusic", &event_desc);
        event_desc->createInstance(&event_inst);
        event_inst->start();
        fmod_system->getEvent("event:/Music/CaveMusic", &event_desc);
        event_desc->createInstance(&event_inst);
        event_inst->start();
        // fx
        fmod_system->getEvent("event:/FX/Fire", &event_desc);
        event_desc->createInstance(&event_inst);
        FMOD_3D_ATTRIBUTES attributes = {{0}};
        attributes.position = {fire_pos.x, fire_pos.y, fire_pos.z};
        attributes.forward.z = 1.0f;
        attributes.up.y = 1.0f;
        event_inst->set3DAttributes(&attributes);
    }

    void step_sound()
    {
        FMOD::Studio::EventDescription *event_desc = NULL;
        fmod_system->getEvent("event:/Character/Steps", &event_desc);
        FMOD::Studio::EventInstance *event_inst = NULL;
        event_desc->createInstance(&event_inst);
        event_inst->setParameterByName("Surface", static_cast<float>(surface));
        event_inst->start();
    }

    void set_fmod_parameter(const char *event, const char *parameter, float value)
    {
        FMOD::Studio::EventDescription *event_desc = NULL;
        fmod_system->getEvent(event, &event_desc);
        FMOD::Studio::EventInstance *instances[10];
        int count;
        event_desc->getInstanceList(instances, 10, &count);
        for (int i = 0; i < count; i++)
        {
            instances[i]->setParameterByName(parameter, value);
            // std::cout << parameter << " set to " << value << "\n";
        }
    }

    void start_fmod_instances(const char *event)
    {
        FMOD::Studio::EventDescription *event_desc = NULL;
        fmod_system->getEvent(event, &event_desc);
        FMOD::Studio::EventInstance *instances[10];
        int count;
        event_desc->getInstanceList(instances, 10, &count);
        for (int i = 0; i < count; i++)
            instances[i]->start();
    }

    void fmod_3d_pos(const char *event, glm::vec3 vec)
    {
        FMOD::Studio::EventDescription *event_desc = NULL;
        fmod_system->getEvent(event, &event_desc);
        FMOD::Studio::EventInstance *instances[1];
        int count;
        event_desc->getInstanceList(instances, 1, &count);
        FMOD_3D_ATTRIBUTES attributes = {{0}};
        attributes.forward.z = 1.0f;
        attributes.up.y = 1.0f;
        attributes.position = {vec.x, vec.y, vec.z};
        std::cout << instances[0]->set3DAttributes(&attributes) << std::endl;
    }

    void stop_fmod_event(const char *event)
    {
        FMOD::Studio::EventDescription *event_desc = NULL;
        fmod_system->getEvent(event, &event_desc);
        FMOD::Studio::EventInstance *instances[10];
        event_desc->getInstanceList(instances, 10, 0);
        for (auto i : instances)
        {
            i->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
        }
    }

    void change_sound_environment(Environment new_env)
    {
        FMOD::Studio::EventDescription *event_desc = NULL;
        FMOD::Studio::EventInstance *event_inst = NULL;

        switch (new_env)
        {
        case CABIN:
            set_fmod_parameter("event:/Ambient/ForestAmbience", "Inside", 1.0f);
            // snapshot
            fmod_system->getEvent("snapshot:/Cabin", &event_desc);
            event_desc->createInstance(&event_inst);
            event_inst->start();
            // ambience
            fmod_system->getEvent("event:/Ambient/CabinAmbience", &event_desc);
            event_desc->createInstance(&event_inst);
            event_inst->start();
            surface = WOOD;
            break;
        case CAVE:
            set_fmod_parameter("event:/Ambient/ForestAmbience", "Inside", 0.9f);
            // snapshot
            fmod_system->getEvent("snapshot:/Cave", &event_desc);
            event_desc->createInstance(&event_inst);
            event_inst->start();
            // ambience
            fmod_system->getEvent("event:/Ambient/CaveAmbience", &event_desc);
            event_desc->createInstance(&event_inst);
            event_inst->start();
            surface = STONE;
            break;
        case FOREST:
            set_fmod_parameter("event:/Ambient/ForestAmbience", "Inside", 0.0f);
            // snapshot
            fmod_system->getEvent("snapshot:/Forest", &event_desc);
            event_desc->createInstance(&event_inst);
            event_inst->start();
            surface = SNOW;
            stop_fmod_event("event:/Ambient/CaveAmbience");
            stop_fmod_event("event:/Ambient/CabinAmbience");
            break;
        }
        environment = new_env;
    }

    bool is_inside_forbiden_zone(glm::vec2 position)
    {
        if ((position.x < 14.033 && position.x > 11.2185) && (position.y < -9.377 && position.y > -13.3761))
        {
            return true;
        } // cabin
        if ((position.x < -2.75 && position.x > -21.7185) && (position.y < -0.777 && position.y > -16.661))
        {
            return true;
        }
        return false;
    }
    void play_sound()
    {
        fmod_system->update();
    }
}



Player::Player()
{
    position = glm::vec3(0.0f);
    model_matrix = glm::mat4(1.0f);
};

void Player::update_position(float x, float y)
{
    position.x += x;
    position.y += y;
    model_matrix = glm::translate(model_matrix, position) * model_matrix;
};

void Item::translate_matrix(glm::vec3 vector, std::vector<ObjectUBO> &ubo)
{
    model_matrix = glm::translate(model_matrix, vector);
    ubo.at(object_ubos_index).model_matrix = model_matrix;
};

void Item::scale_matrix(glm::vec3 vector, std::vector<ObjectUBO> &ubo)
{
    model_matrix = glm::scale(model_matrix, vector);
    ubo.at(object_ubos_index).model_matrix = model_matrix;
};

// two coordinates and diameter of objects for calculating collisions of cylindrical objects

// Collisions
std::vector<std::tuple<float, float, float>> collision_vec_cylindrical = {};
std::vector<std::tuple<float, float, float, float>> collision_vec_rectangular = {};

// returns true if collision occured
bool is_inside_cylinder(glm::vec3 player_pos, float obj_x, float obj_z, float radius)
{
    glm::vec2 player = {player_pos.x, player_pos.z};
    glm::vec2 object = {obj_x, obj_z};

    return glm::length(object - player) < radius;
}
bool is_inside_rectangular(glm::vec3 player_pos, float obj_ax, float obj_az, float obj_bx, float obj_bz)
{
    glm::vec2 player = {player_pos.x, player_pos.z};
    glm::vec2 object_aa = {obj_ax, obj_az};
    glm::vec2 object_bb = {obj_bx, obj_bz};

    bool is_inside_x = object_aa.x < player.x && player.x < object_bb.x;
    bool is_inside_z = object_aa.y < player.y && player.y < object_bb.y;
    return is_inside_x && is_inside_z;
}

bool check_collisions(glm::vec3 player_position, glm::vec3 move_direction)
{
    for (auto i : collision_vec_cylindrical)
    {
        auto [x, z, r] = i;
        if (is_inside_cylinder(player_position + move_direction, x, z, r))
            return true;
    }
    for (auto i : collision_vec_rectangular)
    {
        auto [ax, az, bx, bz] = i;
        if (is_inside_rectangular(player_position + move_direction, ax, az, bx, bz))
            return true;
    }
    return false;
}

void create_collisions_rectangular(glm::vec3 pos, glm::vec3 size, float scale = 1)
{
    glm::vec3 position_min = pos - size * 0.16f;
    glm::vec3 position_max = pos + size * 0.16f;
    collision_vec_rectangular.push_back({position_min.x, position_min.z, position_max.x, position_max.z});
}

void create_cave_collisions()
{
    // right entry wall
    collision_vec_rectangular.push_back({-17.9404, -10.6314, -16.2945, -4.91122});
    // left entry wall
    collision_vec_rectangular.push_back({-22.1937, -16.303, -20.5368, -4.30833});
    // left exit wall ?
    collision_vec_rectangular.push_back({-18.727, -13.6163, -5.33569, -10.1472});
    // right exit wall
    collision_vec_rectangular.push_back({-20.5335, -16.9402, -9.57806, -14.6046});
}
glm::vec3 return_x0z(glm::vec3 vec)
{
    return {vec.x, 0, vec.z};
}

//For loading skybox texture
GLuint loadCubemap(std::vector<std::filesystem::path> faces)
{
    GLuint sky_tex;
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &sky_tex);

    int width, height, channels;

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    unsigned char *data = stbi_load(faces[0].generic_string().data(), &width, &height, &channels, 4);
    glTextureStorage2D(sky_tex, std::log2(width), GL_RGBA8, width, height);

    for (GLuint i = 0; i < faces.size(); i++)
    {
        data = stbi_load(faces[i].generic_string().data(), &width, &height, &channels, 4);
        glTextureSubImage3D(
            sky_tex,
            0,
            0, 0, i,
            width, height, 1,
            GL_RGBA, GL_UNSIGNED_BYTE,
            data);
        stbi_image_free(data);
    }

    return sky_tex;
}

GLuint load_texture_2d(const std::filesystem::path filename)
{
    int width, height, channels;
    unsigned char *data = stbi_load(filename.generic_string().data(), &width, &height, &channels, 4);

    GLuint texture;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);

    glTextureStorage2D(texture, std::log2(width), GL_RGBA8, width, height);

    glTextureSubImage2D(texture,
                        0,                         //
                        0, 0,                      //
                        width, height,             //
                        GL_RGBA, GL_UNSIGNED_BYTE, //
                        data);

    stbi_image_free(data);

    glGenerateTextureMipmap(texture);

    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return texture;
}

std::shared_ptr<Item> Application::spawn_object(std::string name, std::string texture_name, glm::mat4 mdl_matrix, glm::vec4 ambnt_color = glm::vec4(1.0f), glm::vec4 dffs_color = glm::vec4(1.0f), glm::vec4 spclr_color = glm::vec4(0.0f))
{
    std::set<std::string> no_collision_objects = {"cabin/house-front-moved-origin.obj",
                                                  "cabin/house_left.obj",
                                                  "cabin/house_right.obj",
                                                  "cabin/house_back.obj",
                                                  "cabin/house_roof.obj",
                                                  "cabin-floor.obj",
                                                  "dry_branch.obj",
                                                  "cave-floor.obj",
                                                  "cave.obj"};
    auto object = std::make_shared<Item>();
    object->name = name;

    object->collision_index = collision_vec_cylindrical.size();
    object->model_matrix = mdl_matrix;
    object->ambient_color = ambnt_color;
    object->diffuse_color = dffs_color;
    object->specular_color = spclr_color;
    object->texture_name = texture_name;
    size_t index;
    if (!editor.items_map.contains(name))
    {
        geometries.push_back(make_shared<Geometry>(Geometry::from_file(objects_path / name)));
        index = geometries.size() - 1;
    }else{
        index = editor.items_map.at(name)->geometrie_index;
    }
    
    object->geometrie_index = index;
    std::shared_ptr<Geometry> new_object = geometries[index];
    objects_ubos.push_back({.model_matrix = mdl_matrix,
                            .ambient_color = ambnt_color,
                            .diffuse_color = dffs_color,
                            .specular_color = spclr_color});
    object->object_ubos_index = objects_ubos.size() - 1;
    if (texture_name.empty())
    {
        object->has_texture = false;
    }
    else
    {
        stbi_set_flip_vertically_on_load(true);
        object->texture = load_texture_2d(images_path / texture_name);
        object->has_texture = true;
    }

    object->item_index = editor.items.size();
    editor.items_map.try_emplace(object->name, object);
    editor.items.push_back(object);
    if (!no_collision_objects.contains(object->name))
    {
        collision_vec_cylindrical.push_back({object->model_matrix[3][0], object->model_matrix[3][2], 0.2f});
    }
    return object;
};

std::shared_ptr<Item> Application::spawn_object(std::string name, std::string texture_name)
{
    return spawn_object(name, texture_name, glm::mat4(1.0f));
};
std::shared_ptr<Item> Application::spawn_object(std::string name)
{
    return spawn_object(name, "");
};


// Creates trees in a grid layout spread randomly
void Application::create_forest()
{
    int GRID_SIZE = 10;
    int DENSITY = 1; // trees per unit squared
    int LOWEST_DENSITY = 1;
    float tree_collision_size = 0.1f;
    float pos_x = 0;
    float pos_z = 0;
    int current_density = DENSITY;
    std::vector<std::pair<std::string, std::string>> objects = {{"tree_8.obj", "8_tree.png"}};

    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            if (j % 2 == 0)
            {
                continue;
            }
            current_density = DENSITY;
            for (int k = 0; k < current_density; k++)
            {
                pos_x = i - GRID_SIZE / 2 + static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                pos_z = j - GRID_SIZE / 2 + static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                if (is_inside_forbiden_zone({pos_x, pos_z}))
                {
                    continue;
                }
                auto tree_variance = 5.0f * (static_cast<float>(rand()) / (static_cast<float>(RAND_MAX)));
                auto scale = std::clamp(tree_variance, 2.0f, 10.0f);
                auto chosen_object = objects.at(rand() % objects.size());
                auto obj = spawn_object(chosen_object.first, chosen_object.second, glm::translate(glm::mat4(1.0f), glm::vec3(pos_x, 0.290986f, pos_z)));
                obj->scale_matrix(glm::vec3(scale), objects_ubos);
                collision_vec_cylindrical.push_back({pos_x, pos_z, tree_collision_size * scale});
            }
        }
    }
}

glm::mat4 translate_and_scale(glm::vec3 translate, glm::vec3 scale = glm::vec3(1.0f))
{
    auto out = glm::mat4(1.0f);
    out = glm::translate(out, translate);
    out = glm::scale(out, scale);
    return out;
}

void GUI::load_textures(std::filesystem::path images_path)
{
    pause_texture = load_texture_2d(images_path / "paused.png");
    pickup_texture = load_texture_2d(images_path / "pickup.png");
    more_wood_texture = load_texture_2d(images_path / "more_wood.png");
    start_fire_texture = load_texture_2d(images_path / "start_fire.png");
    intro_texture = load_texture_2d(images_path / "intro.png");
    outro_texture = load_texture_2d(images_path / "outro.png");
    oob_texture = load_texture_2d(images_path / "oob.png");
}

Application::Application(int initial_width, int initial_height, std::vector<std::string> arguments)
    : PV112Application(initial_width, initial_height, arguments)
{
    this->width = initial_width;
    this->height = initial_height;
    images_path = lecture_folder_path / "images";
    objects_path = lecture_folder_path / "objects";

    // --------------------------------------------------------------------------
    //  Load/Create Objects
    // --------------------------------------------------------------------------
    geometries.push_back(make_shared<Cube>());

    sphere = geometries[0];
    cube = geometries[0];

    std::vector skybox_sides = {
        images_path / "skybox-dark-stars/right.png",
        images_path / "skybox-dark-stars/left.png",
        images_path / "skybox-dark-stars/top.png",
        images_path / "skybox-dark-stars/bottom.png",
        images_path / "skybox-dark-stars/front.png",
        images_path / "skybox-dark-stars/back.png",
    };
    skybox_texture = loadCubemap(skybox_sides);

    // --------------------------------------------------------------------------
    // Initialize UBO Data
    // --------------------------------------------------------------------------
    camera_ubo.position = glm::vec4(camera.get_eye_position(), 1.0f);
    camera_ubo.projection = glm::perspective(glm::radians(45.0f), float(width) / float(height), 0.01f, 1000.0f);
    camera_ubo.view = glm::lookAt(camera.get_eye_position(), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    light_ubo.position = glm::vec4(0.0f, 5.0f, 2.0f, 1.0f);
    glm::vec3 light_color = {14, 14, 40};
    light_ubo.ambient_color = glm::vec4(light_color/255, 1.0f);
    // 80,104,134 color for moon
    light_ubo.diffuse_color = glm::vec4(80 / 255.0f, 104 / 225.0f, 134 / 225.0f, 0.0f);
    light_ubo.specular_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    //Moon
    moon_ubo.position = glm::vec4(0.0f, light_ubo_height, 0.0f, 1.0f);
    moon_ubo.ambient_color = glm::vec4(80 / 255.0f, 104 / 225.0f, 134 / 225.0f, 0.0f);
    moon_ubo.diffuse_color = glm::vec4(80 / 255.0f, 104 / 225.0f, 134 / 225.0f, 0.0f);
    moon_ubo.specular_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    //Fire
    fire_ubo.position = glm::vec4(12.6465, -0.109174, -11.5774, 1.0f);
    fire_ubo.ambient_color = glm::vec4(1.0f);
    fire_ubo.diffuse_color = glm::vec4(216.0f/255, 104.0f/255, 41.0f/255, 0.0f);
    fire_ubo.specular_color = glm::vec4(216.0f / 255, 104.0f / 255, 41.0f / 255, 0.0f);

    // Skybox
    objects_ubos.push_back({.model_matrix = glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(0.1f)), glm::vec3(light_ubo.position)),
                            .ambient_color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f),
                            .diffuse_color = glm::vec4(0.0),
                            .specular_color = glm::vec4(0.0f)});
    objects_ubos.push_back({.model_matrix = glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(0.1f)), glm::vec3(light_ubo.position)),
                            .ambient_color = glm::vec4(0.33f, 0.33f, 0.5f, 1.0f),
                            .diffuse_color = glm::vec4(1.0f),
                            .specular_color = glm::vec4(0.0f)});


    auto cabin_scaling_vector = glm::vec3(1.0f);

    load_file();
    create_cave_collisions();
    //create_forest();
    //  --------------------------------------------------------------------------
    //  Create Buffers
    //  --------------------------------------------------------------------------
    glCreateBuffers(1, &camera_buffer);
    glNamedBufferStorage(camera_buffer, sizeof(CameraUBO), &camera_ubo, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &light_buffer);
    glNamedBufferStorage(light_buffer, sizeof(LightUBO), &light_ubo, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &moon_buffer);
    glNamedBufferStorage(moon_buffer, sizeof(LightUBO), &moon_ubo, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &fire_buffer);
    glNamedBufferStorage(fire_buffer, sizeof(LightUBO), &fire_ubo, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &objects_buffer);
    glNamedBufferStorage(objects_buffer, sizeof(ObjectUBO) * 1500, nullptr, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &quad_buffer);
    glNamedBufferStorage(quad_buffer, 16 * sizeof(float), quad, 0);
    glCreateVertexArrays(1, &quad_vao);
    glVertexArrayVertexBuffer(quad_vao, 0, quad_buffer, 0, 4 * sizeof(float));
    glEnableVertexArrayAttrib(quad_vao, 0);
    glVertexArrayAttribFormat(quad_vao, 0, 4, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(quad_vao, 0, 0);

    
    gui.load_textures(images_path);
    player.position.y = player_height;
    compile_shaders();
    //Added uniform for intensity of light
    glProgramUniform1f(main_program, 4, light_ubo_intensity);
    fmod_init();
    start_default_sounds();
}

Application::~Application()
{

    delete_shaders();
    //editor.dump_file(objects_path);
    glDeleteBuffers(1, &camera_buffer);
    glDeleteBuffers(1, &light_buffer);
    glDeleteBuffers(1, &objects_buffer);
    fmod_system->release();
}

// ----------------------------------------------------------------------------
// Methods
// ----------------------------------------------------------------------------

void Application::delete_shaders() {}

void Application::compile_shaders()
{
    delete_shaders();
    main_program = create_program(lecture_shaders_path / "main.vert", lecture_shaders_path / "main.frag");
    skybox_program = create_program(lecture_shaders_path / "skybox.vert", lecture_shaders_path / "skybox.frag");
    fire_program = create_program(lecture_shaders_path / "fire.vert", lecture_shaders_path / "fire.frag");
    snow_program = create_program(lecture_shaders_path / "snow.vert", lecture_shaders_path / "snow.frag");
    pause_program = create_program(lecture_shaders_path / "pause.vert", lecture_shaders_path / "pause.frag");
}

void Player::move(Camera camera, float delta)
{
    auto looking_to = glm::normalize((camera.get_looking_to()));
    auto move_by = speed * delta / 1500;

    glm::vec3 input_direction = glm::normalize(input_direction);

    input_direction.x = -(left ? 1.0f : 0.0f) - (right ? -1.0f : 0.0f);
    input_direction.z = -(forward ? 1.0f : 0.0f) + (backward ? 1.0f : 0.0f);

    glm::mat4 camera_matrix = glm::rotate(camera.get_angle_direction(), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::vec3 move_direction = glm::vec3(0);

    if (forward)
    {
        move_direction += looking_to * move_by;
    }
    if (backward)
    {
        move_direction += -looking_to * move_by;
    }
    if (right)
    {
        move_direction += return_x0z(glm::normalize(glm::cross(camera.get_looking_to(), camera.get_upVec())) * move_by);
    }
    if (left)
    {
        move_direction += -return_x0z(glm::normalize(glm::cross(camera.get_looking_to(), camera.get_upVec())) * move_by);
    }
    if (DEBUG)
    {
        if (up)
        {
            position.y += move_by;
        }
        if (down)
        {
            position.y -= move_by;
        }
    }

    if (check_collisions(position, move_direction))
    {
        return;
    }

    position += return_x0z(move_direction);

    // set fmod listener attributes
    FMOD_3D_ATTRIBUTES attributes = {{0}};
    attributes.position = {position.x, position.y, position.z};
    attributes.forward = {looking_to.x, looking_to.y, looking_to.z};
    auto up_vec = -camera.get_upVec(); // don't know why the - but it works
    attributes.up = {up_vec.x, up_vec.y, up_vec.z};
    fmod_system->setListenerAttributes(0, &attributes);
    // fmod_3d_pos("event:/FX/Fire", fire_pos);

    // static int step_count = 0; // static var for testing, if used, make it a Player property?
    static glm::vec3 last_step = position;
    if (glm::length(position - last_step) >= step_length)
    {
        step_sound();
        last_step = position;
        // step_count++;
        // const float max_steps = 100.0f;
        // fmod_system->setParameterByName("Wind", TRUNC(step_count / max_steps));
    }

    if (fire_lit)
    {
        if (glm::length(position - fire_pos) > 8.0f)
        {
            stop_fmod_event("event:/FX/Fire");
            fire_lit = false;
        }
    }

}

void Player::light_fire(){
    if (glm::length(position - fire_pos) <= 0.3 && branches >= 5)
    {
        FMOD::Studio::EventDescription *event_desc = NULL;
        FMOD::Studio::EventInstance *event_inst = NULL;
        fmod_system->getEvent("event:/FX/MatchStrike", &event_desc);
        event_desc->createInstance(&event_inst);
        event_inst->start();
        fire_lit = true;
        std::cout << "fire lit\n";
        start_fmod_instances("event:/FX/Fire");
    }
    
}

bool Player::is_inside_cabin()
{
    return distance_to_cabin() < 1.0f;
};

bool Player::is_inside_cave()
{
    std::vector<float> x = {-10.2418, -13.8495, -18.5664, -19.0475, -19.2726, -19.2264};
    std::vector<float> z = {-13.8795, -13.7655, -13.6163, -13.0849, -8.97754, -5.36872};

    for (int i = 0; i < x.size(); i++)
    {
        if (glm::length(return_x0z(position) - glm::vec3(x[i], 0.0f, z[i])) < 1.5f)
        {
            return true;
        };
    }
    return false;
};

bool Player::is_out_of_bounds(){
    if (position.x > 15 || position.x < -21 
    ||  position.z > 12 || position.z < -15){
        return true;
    }
    return false;
}

float Player::distance_to_cabin()
{
    // TODO change cabin_pos to relative not constant
    auto distance = glm::length(cabin_pos - position);
    return distance;
}

float Player::distance_to_cave()
{
    // TODO change cave_pos to relative not constant
    auto distance = glm::length(cave_pos - position);
    return distance;
}

float Player::distance_to_origin()
{
    // TODO change cave_pos to relative not constant
    auto distance = glm::length(glm::vec3(0.0f) - position);
    return distance;
}

std::optional<std::shared_ptr<Item>> Player::is_near_branch(Editor editor){
    for (auto item : editor.items){
        if (item->name.compare("dry_branch.obj") != 0){
            continue;
        }
        glm::vec3 branch_pos = {item->model_matrix[3][0],
                                0,
                                item->model_matrix[3][2]};
        float distance = glm::length(branch_pos - return_x0z(position));
        if (distance < distance_to_branch){
            return item;
        }
    }
    return {};
}

void Editor::move(float delta, std::vector<ObjectUBO> &ubos, Camera camera)
{
    if (!current_item)
    {
        current_item = items.at(0);
    }
    if (!current_item->selected){
        return;
    }
    float actual_speed = move_speed / 1000;
    glm::vec3 lookingTo = normalize(camera.get_looking_to());
    if (move_up)
    {  
        current_item->translate_matrix(glm::vec3(0.0f, actual_speed, 0.0f), ubos);
    }
    if (move_down)
    {
        current_item->translate_matrix(glm::vec3(0.0f, -actual_speed, 0.0f), ubos);
    }
    if (move_forward)
    {
        current_item->translate_matrix(glm::normalize(return_x0z(lookingTo)) * actual_speed, ubos);
    }
    if (move_backward)
    {
        current_item->translate_matrix(glm::normalize(-return_x0z(lookingTo)) * actual_speed, ubos);
    }
    if (move_right)
    {
        current_item->translate_matrix(return_x0z(glm::normalize(glm::cross(camera.get_looking_to(), camera.get_upVec())) * actual_speed), ubos);
    }
    if (move_left)
    {
        current_item->translate_matrix(-return_x0z(glm::normalize(glm::cross(camera.get_looking_to(), camera.get_upVec())) * actual_speed), ubos);
    }
    if (scale_up)
    {
        current_item->scale_matrix(glm::vec3(1.0f + scale_speed/100), ubos);
    }
    if (scale_down)
    {
        current_item->scale_matrix(glm::vec3(1.0f - scale_speed/100), ubos);
    }
}

void Editor::next_object()
{
    
    if (!current_item->selected){
        current_item->selected = true;
        return;
    }
    // Flag for highlighting of currently selected objects
    current_item->selected = false;
    //Reset to first object after calling next_object() on last object
    if (current_item->item_index + 1 > items.size() - 1)
    {
        current_item = items.at(0);
    }
    else
    {
        current_item = items.at(current_item->item_index + 1);
    }
    current_item->selected = true;

    std::cout << current_item->name << "," << current_item->id << std::endl;
}

void Editor::previous_object()
{
    if (!current_item->selected)
    {
        current_item->selected = true;
        return;
    }
    current_item->selected = false;
    if (current_item->item_index == 0)
    {
        current_item = items.at(items.size() - 1);
    }
    else
    {
        current_item = items.at(current_item->item_index - 1);
    }
    current_item->selected = true;
    std::cout << current_item->name << "," << current_item->id << std::endl;
}

//Prints object information to stdout
void Editor::dump()
{
    std::cout << std::endl;
    if (!current_item)
    {
        std::cout << "no object";
        return;
    }
    for (auto item : items)
    {
        auto mm = item->model_matrix;
        std::cout << item->name << ">> trans : glm::vec3("
                  << mm[3][0]
                  << ","
                  << mm[3][1]
                  << ","
                  << mm[3][2] << ")" << std::endl;
        std::cout << item->name << ">> scale : glm::vec3("
                  << mm[0][0]
                  << ","
                  << mm[1][1]
                  << ","
                  << mm[2][2] << ")" << std::endl;
    }
}

//Serializes object info to file
void Editor::dump_file(std::filesystem::path path)
{
    std::ofstream output_file;
    output_file.open(path / "positions.txt",
                     std::ios::out | std::ios::trunc);

    std::cout << std::endl;
    if (!current_item)
    {
        std::cout << "no object";
        return;
    }
    for (auto item : items)
    {
        // do not dump trees, they are generated randomly and we dont need to store their values
        // if (item->name == "8_tree.obj")
        // {
        //     continue;
        // }
        auto mm = item->model_matrix;
        output_file << item->name << ";"
                    << mm[3][0]
                    << ","
                    << mm[3][1]
                    << ","
                    << mm[3][2] << ";"
                    << mm[0][0]
                    << ","
                    << mm[1][1]
                    << ","
                    << mm[2][2] << ";"
                    << item->texture_name << std::endl;
    }
    output_file.close();
}

std::vector<std::string> split(std::string line, char delim)
{
    std::vector<std::string> result;
    std::stringstream ss(line);
    std::string item;

    while (std::getline(ss, item, delim))
    {
        result.push_back(item);
    }
    return result;
}

float to_float(std::string str)
{
    return static_cast<float>(std::stod(str));
}

//Deserialization
void Application::load_file()
{
    std::ifstream input_file;
    input_file.open(objects_path / "positions.txt", std::ios::in);
    std::string delimiter = ";";

    std::string line;
    while (std::getline(input_file, line))
    {
        if (line.empty()){
            std::cout << "empty line";
            continue;
        }
        auto split_line = split(line, ';');
        auto translate_str = split_line[1];
        auto scale_str = split_line[2];

        auto split_translate = split(translate_str, ',');
        auto split_scale = split(scale_str, ',');

        if (split_line.size() < 4)
        {
            spawn_object(split_line.at(0), "", translate_and_scale({to_float(split_translate[0]), to_float(split_translate[1]), to_float(split_translate[2])}, {to_float(split_scale[0]), to_float(split_scale[1]), to_float(split_scale[2])}));
        }
        else
        {
            spawn_object(split_line.at(0), split_line[3], translate_and_scale({to_float(split_translate[0]), to_float(split_translate[1]), to_float(split_translate[2])}, {to_float(split_scale[0]), to_float(split_scale[1]), to_float(split_scale[2])}));
        }
    }
    input_file.close();
}

void Application::update(float delta)
{
    player.move(camera, delta);
    editor.move(delta, objects_ubos, camera);
    fire_radius_time += delta * 0.001f;
    fire_radius = ((sin(fire_radius_time) + pow(cos(2 * fire_radius_time / 3), 2)) / 4 + 1);


    tick();
    fmod_system->update(); // keep this at the end
    
}

void Application::tick()
{
    if (paused)
    {
        return;
    }
    using namespace std::chrono;
    const int update_period = 100;
    static auto last_tick = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    auto this_tick = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    static unsigned long cabin_time = 0;
    static unsigned long cave_time = 0;
    static float breath = 0.0f;
    

    if (this_tick - last_tick >= update_period)
    {
        overall_time++;
        last_tick = this_tick;

        if (player.is_inside_cabin())
        {
            if (environment != CABIN)
            {
                std::cout << "in cabin\n";
                cabin_time = 0;
                breath = 0.0f;
                change_sound_environment(CABIN);
            }
            cabin_time++;
            set_fmod_parameter("event:/Music/CabinMusic", "FireProgress", std::clamp(cabin_time / 300.0f, 0.0f, 1.0f));
        }
        else if (player.is_inside_cave())
        {
            if (environment != CAVE)
            {
                std::cout << "in cave\n";
                cave_time = 0;
                change_sound_environment(CAVE);
            }
            cave_time++;
            breath += 0.004f;
            fmod_system->setParameterByName("FrightLevel", std::clamp(cave_time / 300.0f, 0.0f, 1.0f));
        }
        else
        {
            if (environment != FOREST)
            {
                std::cout << "out\n";
                change_sound_environment(FOREST);
                fmod_system->setParameterByName("FrightLevel", 0.0f);
                breath = 0.2f;
            }
            breath += 0.00001f;
        }

        auto cave_distance = player.distance_to_cave();
        auto cabin_distance = player.distance_to_cabin();
        set_fmod_parameter("event:/Music/CaveMusic", "CaveDistance", cave_distance);
        set_fmod_parameter("event:/Music/CabinMusic", "CabinDistance", cabin_distance);
        fmod_system->setParameterByName("Wind", std::clamp(player.distance_to_origin() / 25.0f, 0.0f, 1.0f));
        set_fmod_parameter("event:/Character/Breathing", "BreathIntensity", std::clamp(breath, 0.0f, 1.0f));
    }
}

void Application::draw_objects()
{
    for (auto item : editor.items)
    {
        if (item->is_picked_up){
            continue;
        }
        glUniform1i(glGetUniformLocation(main_program, "has_texture"), item->has_texture);
        glUniform1i(glGetUniformLocation(main_program, "selected"), item->selected);
        if (item->has_texture)
        {
            glBindTextureUnit(3, item->texture);
        }
        glNamedBufferSubData(objects_buffer, 0, sizeof(ObjectUBO) * objects_ubos.size(), objects_ubos.data());
        glBindBufferRange(GL_UNIFORM_BUFFER, 2, objects_buffer, item->object_ubos_index * 256, sizeof(ObjectUBO));
        if (item->name.compare("fireplace-logs.obj") == 0){
            glDisable(GL_CULL_FACE);
            geometries.at(item->geometrie_index)->draw();
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            continue;
        }
            geometries.at(item->geometrie_index)->draw();
    }
};


void Application::render()
{
    
    if (paused){
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);   
    }else{
        if(glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_HIDDEN){
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        };
    }
    
    // --------------------------------------------------------------------------
    // Update UBOs
    // --------------------------------------------------------------------------
    // Camera
    camera_ubo.position = glm::vec4(camera.get_eye_position(), 1.0f);
    camera_ubo.projection = glm::perspective(glm::radians(45.0f), static_cast<float>(width) / static_cast<float>(height), 0.01f, 1000.0f);
    camera_ubo.view = camera.get_view_matrix();
    camera.update_position(player.position);

    // Skybox
    objects_ubos[0].model_matrix = glm::translate(glm::mat4(1.0f), camera.get_eye_position());
    glNamedBufferSubData(camera_buffer, 0, sizeof(CameraUBO), &camera_ubo);
    glNamedBufferSubData(objects_buffer, 0, sizeof(ObjectUBO) * objects_ubos.size(), objects_ubos.data());

    //Main light
    light_ubo.position = glm::vec4(player.position.x, player.position.y, player.position.z, 1.0f);
    glProgramUniform1f(main_program, 4, light_ubo_intensity);
    glNamedBufferSubData(light_buffer, 0, sizeof(LightUBO), &light_ubo);

    //Moon light
    moon_ubo.position = glm::vec4(moon_ubo.position.x, light_ubo_height, moon_ubo.position.y, moon_ubo.position.z);
    glNamedBufferSubData(moon_buffer, 0, sizeof(LightUBO), &moon_ubo);
    glBindBufferBase(GL_UNIFORM_BUFFER, 4, moon_buffer);

    //Fire light
    glBindBufferBase(GL_UNIFORM_BUFFER, 5, fire_buffer);
    glProgramUniform1f(main_program, 6, fire_lit);
    glProgramUniform1f(main_program, 7, fire_radius);

    //Flashlight
    glProgramUniform1f(main_program, 8, flashlight_lit);

    // --------------------------------------------------------------------------
    // Draw scene
    // --------------------------------------------------------------------------
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);

    // Clear
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Configure fixed function pipeline
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Skybox
    glDepthMask(GL_FALSE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glUseProgram(skybox_program);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, camera_buffer);
    glBindBufferRange(GL_UNIFORM_BUFFER, 2, objects_buffer, 0 * 256, sizeof(ObjectUBO));

    glBindTextureUnit(3, skybox_texture);
    sphere->draw();
    glDepthMask(GL_TRUE);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);


    // Draw objects
    glUseProgram(main_program);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, camera_buffer);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, light_buffer);
    glBindBufferRange(GL_UNIFORM_BUFFER, 2, objects_buffer, 0 * 256, sizeof(ObjectUBO));
    glUniform1i(glGetUniformLocation(main_program, "has_texture"), false);

    draw_objects();

    

    glUseProgram(pause_program);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    if (paused)
    {
        
        glBindTextureUnit(0, gui.pause_texture);
        glBindVertexArray(quad_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
    }

    auto branch = player.is_near_branch(editor);
    if (branch && !branch.value()->is_picked_up && overall_time > 250){
        
        glBindTextureUnit(0, gui.pickup_texture);
        glBindVertexArray(quad_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
    }

    if (overall_time > 5 && overall_time < 250){
        
        glBindTextureUnit(0, gui.intro_texture);
        glBindVertexArray(quad_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
        std::cout << "Elapsed time is: " << overall_time << std::endl;
    }

    if (glm::length(player.position - fire_pos) <= 0.3){
        if (player.branches < 5){
            glBindTextureUnit(0, gui.more_wood_texture);
        }else if (!fire_lit){
            glBindTextureUnit(0, gui.start_fire_texture);
        }
        else{
            glBindTextureUnit(0, gui.outro_texture);
        }
        glBindVertexArray(quad_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
    }

    if (player.is_out_of_bounds()){
        glBindTextureUnit(0, gui.oob_texture);
        glBindVertexArray(quad_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
    }
}

void GUI::render(){}

void Application::render_ui()
{
    const float unit = ImGui::GetFontSize();

    // Show simple menu.
    if (!paused){
        return;
    }
    ImGui::Begin("Editor", nullptr);
    ImGui::SetWindowSize(ImVec2(25 * unit, 20 * unit));
    ImGui::SetWindowPos(ImVec2(2 * unit, 2 * unit));

    ImGui::Text("Current object: %s, %u", editor.current_item->name.data(), editor.current_item->id);
    ImGui::Text("Controls: \n Movement - W,A,S,D \n Object selection - M,N \n Object movement - Arrows, PgUp,PgDown \n Object scaling - J,K \n Save - B");
    ImGui::SliderFloat("Player speed", &player.speed, 0, 10);
    ImGui::SliderFloat("Light height", &light_ubo_height, 0, 100);
    ImGui::SliderFloat("Light intensity", &light_ubo_intensity, 0, 1);
    ImGui::SliderFloat("Object move speed", &editor.move_speed, 0, 10);
    ImGui::SliderFloat("Scale speed", &editor.scale_speed, 0, 10);
    ImGui::End();
}
                                                                                                                                                                                                       
// ----------------------------------------------------------------------------
// Input Events
// ----------------------------------------------------------------------------

void Application::on_resize(int width, int height)
{
    // Calls the default implementation to set the class variables.
    PV112Application::on_resize(width, height);
}

void Application::on_mouse_move(double x, double y)
{
    if (paused){
        return;
    }
    camera.on_mouse_move(x, y);
}

void Application::on_mouse_button(int button, int action, int mods)
{
    if (paused){
        return;
    }
    camera.on_mouse_button(button, action, mods);
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
        std::cout << player.position[0] << "," << player.position[1] << "," << player.position[2] << "," << light_ubo_intensity << std::endl;
}

void Application::on_key_pressed(int key, int scancode, int action, int mods)

{
    //Possible TODO: disable walking when paused
    switch (key)
    {
    case GLFW_KEY_ESCAPE:
        if (action == GLFW_RELEASE)
        {
            editor.current_item->selected = false;
        }
        break;

    case GLFW_KEY_SPACE:
        if (action == GLFW_RELEASE){
            paused = !paused;
        }
        break;
    case 87: // W
        if (action == GLFW_RELEASE)
        {
            player.forward = false;
        }
        else
        {
            player.forward = true;
        }
        // player.input_direction.x += 1.0f;
        break;
    case 65: // A
        if (action == GLFW_RELEASE)
        {
            player.left = false;
        }
        else
        {
            player.left = true;
        }
        // player.input_direction.z += -1.0f;
        break;
    case 83: // S
        if (action == GLFW_RELEASE)
        {
            player.backward = false;
        }
        else
        {
            player.backward = true;
        }
        // player.input_direction.x += -1.0f;
        break;
    case 68: // D
        if (action == GLFW_RELEASE)
        {
            player.right = false;
        }
        else
        {
            player.right = true;
        }
        // player.input_direction.z += 1.0f;
        break;
    case GLFW_KEY_E:
        if (action == GLFW_RELEASE)
        {
            player.up = false;
        }
        else
        {
            player.up = true;
        }
        break;
    case GLFW_KEY_Q:
        if (action == GLFW_RELEASE)
        {
            player.down = false;
        }
        else
        {
            player.down = true;
        }
        break;
    case GLFW_KEY_PAGE_UP:
        if (action == GLFW_RELEASE)
        {
            editor.move_up = false;
        }
        else
        {
            editor.move_up = true;
        }
        break;
    case GLFW_KEY_PAGE_DOWN:
        if (action == GLFW_RELEASE)
        {
            editor.move_down = false;
        }
        else
        {
            editor.move_down = true;
        }
        break;
    case GLFW_KEY_UP:
        if (action == GLFW_RELEASE)
        {
            editor.move_forward = false;
        }
        else
        {
            editor.move_forward = true;
        }
        break;
    case GLFW_KEY_DOWN:
        if (action == GLFW_RELEASE)
        {
            editor.move_backward = false;
        }
        else
        {
            editor.move_backward = true;
        }
        break;
    case GLFW_KEY_RIGHT:
        if (action == GLFW_RELEASE)
        {
            editor.move_right = false;
        }
        else
        {
            editor.move_right = true;
        }
        break;
    case GLFW_KEY_LEFT:
        if (action == GLFW_RELEASE)
        {
            editor.move_left = false;
        }
        else
        {
            editor.move_left = true;
        }
        break;
    case GLFW_KEY_K:
        if (action == GLFW_RELEASE)
        {
            editor.scale_up = false;
        }
        else
        {
            editor.scale_up = true;
        }
        break;
    case GLFW_KEY_J:
        if (action == GLFW_RELEASE)
        {
            editor.scale_down = false;
        }
        else
        {
            editor.scale_down = true;
        }
        break;
    case GLFW_KEY_M:
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
        {
            editor.next_object();
        }
        break;
    case GLFW_KEY_N:
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
        {
            editor.previous_object();
        }
        break;
    case GLFW_KEY_B:
        if (action == GLFW_RELEASE)
        {
            editor.dump();
            editor.dump_file(objects_path);
        }
        break;
    case GLFW_KEY_INSERT:
        if (action == GLFW_RELEASE)
        {
            spawn_object(editor.current_item->name, editor.current_item->texture_name, glm::translate(glm::mat4(1.0f), return_x0z(player.position)));
        }
    case GLFW_KEY_F:
        if(action == GLFW_RELEASE){
            player.light_fire();

            auto branch = player.is_near_branch(editor);
            if (branch)
            {
                branch.value()->is_picked_up = true;
                player.branches += 1;
            }
        }
    default:
        break;
    }
    // Calls default implementation that invokes compile_shaders when 'R key is hit.
    PV112Application::on_key_pressed(key, scancode, action, mods);
}


/* Known bugs
- Trees spawning twice at the same location when loaded from positions.txt file
- scaling objects moves them instead of just scaling them, such behaviour is expected but it would be better if scaling did not move objects underground
- camera jitter after unpause 
*/