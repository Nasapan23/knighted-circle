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
    void renderEnemyHealthBar(const Enemy& enemy);
    void renderDeathScreen();
    void spawnEnemies(int count);
    
    // Sword rendering and update functions
    void initSword();
    void updateSword();
    void renderSword();
    bool checkSwordHit(float targetX, float targetY, float targetRadius);
    
    // Timing variables
    double lastFrameTime;
    float deltaTime;

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
    
    // For drawing the sword
    GLuint swordVAO, swordVBO;
    std::vector<float> swordVertices;

    glm::mat4 projection; // Orthographic projection matrix
    glm::mat4 textProjection; // Orthographic projection for text (in screen coordinates)

    // Game entities:
    Player* player;
    std::vector<Enemy> enemies;
    float enemySpeed;
    int maxEnemies;
    float enemySpawnTimer;
    float enemySpawnInterval;

    // Arrow projectile:
    struct Arrow {
        float x, y;
        float vx, vy;
        float radius;
    } arrow;
    bool arrowActive;
    bool mouseWasPressed;
    float arrowSpeed;
    
    // Sword properties:
    struct Sword {
        float offsetX, offsetY;    // Offset from player position
        float angle;               // Current angle in radians
        float length;              // Length of the sword
        float width;               // Width of the sword blade
        float hitboxRadius;        // Radius of the sword's hit area
        bool isSwinging;           // Whether the sword is currently in swing animation
        float swingSpeed;          // Speed of the swing animation in radians per frame
        float swingAngle;          // Total angle to swing
        float swingProgress;       // Current progress of swing (0.0 to 1.0)
        float damage;              // Damage dealt by the sword
        float cooldown;            // Cooldown between swings
        float cooldownTimer;       // Current cooldown timer
    } sword;
    bool rightMouseWasPressed;     // Track right mouse button state

    // Test damage
    float damageTimer;
    float damageCooldown;
    
    // Death screen
    float deathScreenTimeout;
};

#endif
