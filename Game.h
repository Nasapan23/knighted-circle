#ifndef GAME_H
#define GAME_H

#include <vector>
#include <map>
#include <string>
#include "Shader.h"
#include "Player.h"
#include "Enemy.h"
#include "Font.h"
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
    void renderHealthBar();
    void renderDeathScreen();

    GLFWwindow* window;
    int screenWidth, screenHeight;
    Shader* shaderProgram;
    Shader* textShader;
    Font* gameFont;

    // For drawing circles (player, enemy, arrow)
    GLuint circleVAO, circleVBO;
    std::vector<float> circleVertices;
    int segments;       // Number of segments in circle (e.g., 50)
    float baseRadius;   // Base radius for the circles (player & enemy)

    // For drawing rectangles (health bar, death screen)
    GLuint rectVAO, rectVBO;

    glm::mat4 projection; // Orthographic projection matrix
    glm::mat4 textProjection; // Orthographic projection for text (in screen coordinates)

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

    // Test damage
    float damageTimer;
    float damageCooldown;
    
    // Death screen
    float deathScreenTimeout;
};

#endif
