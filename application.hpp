// ################################################################################
// Common Framework for Computer Graphics Courses at FI MUNI.
//
// Copyright (c) 2021-2022 Visitlab (https://visitlab.fi.muni.cz)
// All rights reserved.
// ################################################################################

#pragma once

#include "camera.h"
#include "cube.hpp"
#include "pv112_application.hpp"
#include "sphere.hpp"
#include "teapot.hpp"
#include <set>
#include <map>
#include <optional>

// ----------------------------------------------------------------------------
// UNIFORM STRUCTS
// ----------------------------------------------------------------------------
struct CameraUBO
{
    glm::mat4 projection;
    glm::mat4 view;
    glm::vec4 position;
};

struct SnowUBO{
    glm::vec3 position;
    float lifetime;
};

struct LightUBO
{
    glm::vec4 position;
    glm::vec4 ambient_color;
    glm::vec4 diffuse_color;
    glm::vec4 specular_color;
};

struct alignas(256) ObjectUBO
{
    glm::mat4 model_matrix;  // [  0 -  64) bytes
    glm::vec4 ambient_color; // [ 64 -  80) bytes
    glm::vec4 diffuse_color; // [ 80 -  96) bytes

    // Contains shininess in .w element
    glm::vec4 specular_color; // [ 96 - 112) bytes
};

// Enums
enum Environment
{
    FOREST,
    CAVE,
    CABIN
};
enum Surface
{
    SNOW,
    WOOD,
    STONE
};

// Constants
const float clear_color[4] = {0.0, 0.0, 0.0, 1.0};
const float clear_depth[1] = {1.0};
const float step_length = 0.25f;


// Globals
struct Globals
{
    glm::vec2 input_dir = glm::vec2(0.0f, 0.0f);
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
};

/* namespace Snow
{
    int NUMBER_OF_FLAKES = 20;
    float SPREAD = 3;

    class Emiter
    {
    public:
        std::vector<SnowUBO> snowflakes;
        GLuint vbo;
        
        Emiter()
        {
            for (int i = 0; i < NUMBER_OF_FLAKES; i++)
            {
                SnowUBO snowflake;
                snowflake.lifetime = 10;
                snowflake.position = glm::vec3(static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * SPREAD,
                                               static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * SPREAD,
                                               static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * SPREAD);
                snowflakes.push_back(snowflake);
            }
        }
        void update_particles(float delta, std::vector<SnowUBO> &buffer)
        {
            for (SnowUBO snowflake : snowflakes)
            {
                if (snowflake.lifetime < 0)
                {
                    snowflake.position = glm::vec3(0.0f);
                    snowflake.lifetime = 10;
                    continue;
                }
                snowflake.lifetime -= delta;
                snowflake.position -= glm::vec3(0.0f, delta * 2.0f, 0.0f);
            }
            buffer = snowflakes;
        };
    };
}; */



class Item
{
protected:
    static size_t nextID;

public:
    size_t id;
    bool selected = false;
    std::string name;
    size_t object_ubos_index;
    size_t item_index;
    size_t geometrie_index;
    size_t collision_index;
    bool has_texture;
    std::filesystem::path filename;
    GLuint texture;
    std::string texture_name;
    float scale = 1;
    //Flag for branches
    bool is_picked_up = false;
    // float size;

    glm::mat4 model_matrix = glm::mat4(1.0f);
    glm::vec4 ambient_color = glm::vec4(0.0f);
    glm::vec4 diffuse_color = glm::vec4(1.0f);
    glm::vec4 specular_color = glm::vec4(0.0f);

    Item(){
        id = ++nextID;
    };
    void translate_matrix(glm::vec3 vector, std::vector<ObjectUBO> &ubo);
    void scale_matrix(glm::vec3 vector, std::vector<ObjectUBO> &ubo);
};




class Editor
{
public:
    bool move_up = false;
    bool move_down = false;
    bool move_forward = false;
    bool move_backward = false;
    bool move_right = false;
    bool move_left = false;
    bool scale_up = false;
    bool scale_down = false;
    float scale_speed = 1.0f;
    float move_speed = 5.0f;
    std::vector<std::shared_ptr<Item>> items = {};
    std::map<std::string, std::shared_ptr<Item>> items_map = {};

    std::shared_ptr<Item> current_item;

    void move(float delta, std::vector<ObjectUBO> &, Camera camera);
    void next_object();
    void previous_object();
    void dump();
    void dump_file(std::filesystem::path path);
};

class Player
{
public:
    bool forward = false;
    bool backward = false;
    bool right = false;
    bool left = false;
    bool up = false;
    bool down = false;
    glm::vec3 position;
    glm::mat4 model_matrix;
    glm::vec3 input_direction;
    size_t branches = 0;

    float speed = 0.4f;
    Player();

    void update_position(float x, float y);
    void move(Camera camera, float delta);
    float distance_to_cabin();
    float distance_to_cave();
    float distance_to_origin();
    bool is_inside_cabin();
    bool is_inside_cave();
    bool is_out_of_bounds();
    void light_fire();
    std::optional<std::shared_ptr<Item>> is_near_branch(Editor editor);
};

class GUI{
    public:
        GLuint pickup_texture;
        GLuint intro_texture;
        GLuint pause_texture;
        GLuint outro_texture;
        GLuint more_wood_texture;
        GLuint start_fire_texture;
        GLuint oob_texture;

        void load_textures(std::filesystem::path images_path);
        void update(float delta);
        void render();
};


class Application : public PV112Application
{

    // ----------------------------------------------------------------------------
    // Variables
    // ----------------------------------------------------------------------------
    std::filesystem::path images_path;
    std::filesystem::path objects_path;
    float light_ubo_height = 10;
    float light_ubo_intensity = 0.27f;
    bool paused = false;
    bool flashlight_lit = true;
    unsigned long overall_time = 0;

    // Main program
    GLuint main_program;
    GLuint skybox_program;
    GLuint fire_program;
    GLuint snow_program;
    GLuint pause_program;
    GLuint trees_program;
    // TODO: feel free to add as many as you need/like

    Editor editor;
    Player player;
    GUI gui;
    //Snow::Emiter emiter;
    // List of geometries used in the project
    std::vector<std::shared_ptr<Geometry>> geometries;
    // Shared pointers are pointers that automatically count how many times they are used. When there are 0 pointers to the object pointed
    // by shared_ptrs, the object is automatically deallocated. Consequently, we gain 3 main properties:
    // 1. Objects are not unnecessarily copied
    // 2. We don't have to track pointers
    // 3. We don't have to deallocate these geometries
    std::shared_ptr<Geometry> sphere;
    std::shared_ptr<Geometry> bunny;
    std::shared_ptr<Geometry> terrain;
    std::shared_ptr<Geometry> tree;
    std::shared_ptr<Geometry> cube;

    // Default camera that rotates around center.
    Camera camera;
    float time = 0;
    // Globals
    Globals globals;

    // UBOs
    GLuint camera_buffer = 0;
    CameraUBO camera_ubo;

    GLuint light_buffer = 0;
    LightUBO light_ubo;

    GLuint moon_buffer = 0;
    LightUBO moon_ubo;

    GLuint fire_buffer = 0;
    LightUBO fire_ubo;
    float fire_radius = 0;
    float fire_radius_time = 0;

    GLuint objects_buffer = 0;
    std::vector<ObjectUBO> objects_ubos;

    GLuint snow_buffer = 0;
    std::vector<SnowUBO> snow_ubos;

    GLuint quad_buffer = 0;
    GLuint quad_vao = 0;
    const float quad[16] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f
        };
    GLuint pause_texture;
    //GUI prompts
    

    // Lights
    std::vector<LightUBO>
        lights;
    GLuint lights_buffer = 0;

    //Trees
    size_t ob_ubos_size = 0;
    size_t number_of_trees = 0;

    // Textures
    GLuint marble_texture = 0;
    GLuint skybox_texture = 0;
    // ----------------------------------------------------------------------------
    // Constructors & Destructors
    // ----------------------------------------------------------------------------
public:
    /**
     * Constructs a new @link Application with a custom width and height.
     *
     * @param 	initial_width 	The initial width of the window.
     * @param 	initial_height	The initial height of the window.
     * @param 	arguments	  	The command line arguments used to obtain the application directory.
     */
    Application(int initial_width, int initial_height, std::vector<std::string> arguments = {});

    /** Destroys the {@link Application} and releases the allocated resources. */
    ~Application() override;

    // ----------------------------------------------------------------------------
    // Methods
    // ----------------------------------------------------------------------------
    void draw_objects();
    // void draw_particles();
    void load_file();
    std::shared_ptr<Item> spawn_object(std::string name);
    std::shared_ptr<Item> spawn_object(std::string name, std::string texture_name);
    std::shared_ptr<Item> spawn_object(std::string name, std::string texture, glm::mat4 mdl_matrix, glm::vec4 ambnt_color, glm::vec4 dffs_color, glm::vec4 spclr_color);
    void create_forest();
    /** @copydoc PV112Application::compile_shaders */
    void compile_shaders() override;

    /** @copydoc PV112Application::delete_shaders */
    void delete_shaders() override;

    /** @copydoc PV112Application::update */
    void update(float delta) override;

    /** @copydoc PV112Application::render */
    void render() override;

    /** @copydoc PV112Application::render_ui */
    void render_ui() override;

    void tick();

    // ----------------------------------------------------------------------------
    // Input Events
    // ----------------------------------------------------------------------------

    /** @copydoc PV112Application::on_resize */
    void on_resize(int width, int height) override;

    /** @copydoc PV112Application::on_mouse_move */
    void on_mouse_move(double x, double y) override;

    /** @copydoc PV112Application::on_mouse_button */
    void on_mouse_button(int button, int action, int mods) override;

    /** @copydoc PV112Application::on_key_pressed */
    void on_key_pressed(int key, int scancode, int action, int mods) override;
};
