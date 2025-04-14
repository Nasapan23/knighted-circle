#ifndef GAME_H
#define GAME_H

#include <vector>
#include "Shader.h"
#include "Player.h"
#include "Enemy.h"
#include "dependente/glew/glew.h"
#include "dependente/glm/glm.hpp"

class Game {
public:
    Game();
    ~Game();

    bool init();
    void run();
    void cleanup();

private:
    void processInput();
    void update();
    void render();

    GLFWwindow* window;
    int screenWidth, screenHeight;
    Shader* shaderProgram;

    // For drawing circles (player, enemy, arrow)
    GLuint circleVAO, circleVBO;
    std::vector<float> circleVertices;
    int segments;       // Number of segments in circle (e.g., 50)
    float baseRadius;   // Base radius for the circles (player & enemy)

    glm::mat4 projection; // Orthographic projection matrix

    // Game entities:
    Player* player;
    std::vector<Enemy> enemies;
    float enemySpeed;

    // Arrow projectile:
    struct Arrow {
        float x, y;
        float vx, vy;
        float radius;
    } arrow;
    bool arrowActive;
    bool mouseWasPressed;
    float arrowSpeed;
};

#endif
