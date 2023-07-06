#include "application.hpp"


class Player{
public:
    bool forward = false;
    bool backward = false;
    bool right = false;
    bool left = false;
    bool up = false;
    bool down = false;
    glm::vec3 position;
    glm::mat4 model_matrix;
    
    float speed = 0.7f;

    Player();
    void update_position(float x, float y);
    void move(Camera camera, float delta);
};


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