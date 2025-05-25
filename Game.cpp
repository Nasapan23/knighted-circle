#include "Game.h"
#include "dependente/glm/gtc/matrix_transform.hpp"
#include "dependente/glm/gtc/type_ptr.hpp"
#include <iostream>
#include <cstdlib>
#include <cmath>
#include "dependente/glfw/glfw3.h"

// Utility: generate circle vertices (positions only)
std::vector<float> createCircleVertices(float radius, int segments) {
    std::vector<float> vertices;
    // Center point
    vertices.push_back(0.0f);
    vertices.push_back(0.0f);
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * 3.14159265358979323846f * i / segments;
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        vertices.push_back(x);
        vertices.push_back(y);
    }
    return vertices;
}

Game::Game()
    : window(nullptr), screenWidth(0), screenHeight(0),
    shaderProgram(nullptr), textShader(nullptr), gameFont(nullptr),
    circleVAO(0), circleVBO(0),
    rectVAO(0), rectVBO(0),
    swordVAO(0), swordVBO(0),
    arrowVAO(0), arrowVBO(0),
    tileVAO(0), tileVBO(0),
    segments(50), baseRadius(0.05f),
    enemySpeed(0.008f), maxEnemies(4), enemySpawnTimer(0.0f), enemySpawnInterval(4.0f),
    arrowActive(false), mouseWasPressed(false),
    arrowSpeed(0.02f), damageTimer(0.0f), damageCooldown(3.0f),
    rightMouseWasPressed(false),
    terrainGenerated(false),
    deathScreenTimeout(3.0f), lastFrameTime(0.0), deltaTime(0.0f)
{
    // Initialize sword parameters - simplified
    sword.offsetX = baseRadius * 2.5f;  // Initial position
    sword.offsetY = 0.0f;
    sword.angle = 0.0f;
    sword.length = baseRadius * 3.5f;   
    sword.width = baseRadius * 0.4f;    
    sword.hitboxRadius = baseRadius * 1.5f; 
    sword.isSwinging = false;
    sword.swingSpeed = 0.05f;           // Moderate speed for visibility
    sword.swingAngle = 3.14159f * 2.0f; // Not used in new system
    sword.swingProgress = 0.0f;
    sword.damage = 25;                  // Good damage
    sword.cooldown = 1.0f;              // 1 second cooldown
    sword.cooldownTimer = 0.0f;
    
    // Seed random number generator
    srand(static_cast<unsigned int>(time(NULL)));
}

Game::~Game() {
    // Cleanup is done in cleanup()
}

float randomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

bool Game::init() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create full-screen window using primary monitor's resolution
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (!monitor) {
        std::cerr << "Failed to get primary monitor" << std::endl;
        glfwTerminate();
        return false;
    }
    
    // Hardcode resolution to 1920x1080 instead of using monitor's resolution
    screenWidth = 1920;
    screenHeight = 1080;
    
    // Create window with 1920x1080 resolution in fullscreen mode
    glfwWindowHint(GLFW_RED_BITS, 8);
    glfwWindowHint(GLFW_GREEN_BITS, 8);
    glfwWindowHint(GLFW_BLUE_BITS, 8);
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    window = glfwCreateWindow(screenWidth, screenHeight, "Medieval Fantasy Fight", monitor, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return false;
    }
    glViewport(0, 0, screenWidth, screenHeight);

    // Build shader program
    shaderProgram = new Shader("vertex_shader.glsl", "fragment_shader.glsl");
    textShader = new Shader("text_vertex.glsl", "text_fragment.glsl");

    // Enable blending for transparent elements
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set up projections
    float aspect = static_cast<float>(screenWidth) / screenHeight;
    // World coordinates: X from -aspect to aspect, Y from -1 to 1.
    projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    // Text coordinates: X from 0 to screenWidth, Y from 0 to screenHeight (bottom to top)
    textProjection = glm::ortho(0.0f, static_cast<float>(screenWidth), 0.0f, static_cast<float>(screenHeight));

    // Initialize font system
    gameFont = new Font();
    
    // Try different font paths in order of preference
    if (!gameFont->init("assets/fonts/arial.ttf", 48)) {
        if (!gameFont->init("assets/fonts/medieval.ttf", 48)) {
            if (!gameFont->init("C:/Windows/Fonts/arial.ttf", 48)) {
                std::cerr << "Failed to load any font" << std::endl;
                // Continue anyway, we'll just not render text
            }
            else {
                std::cout << "Loaded system font: C:/Windows/Fonts/arial.ttf" << std::endl;
            }
        }
        else {
            std::cout << "Loaded font: assets/fonts/medieval.ttf" << std::endl;
        }
    }
    else {
        std::cout << "Loaded font: assets/fonts/arial.ttf" << std::endl;
    }

    // Configure text shader
    textShader->use();
    textShader->setMat4("projection", glm::value_ptr(textProjection));
    textShader->setInt("text", 0);
    
    // Set the shader for the font
    if (gameFont) {
        gameFont->setShader(textShader->ID);
    }

    // Generate circle vertices (player, enemy, arrow all use the same base mesh)
    circleVertices = createCircleVertices(baseRadius, segments);
    glGenVertexArrays(1, &circleVAO);
    glGenBuffers(1, &circleVBO);
    glBindVertexArray(circleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
    glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float), circleVertices.data(), GL_STATIC_DRAW);
    // Our vertices are 2 floats per vertex (x, y)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Create the player
    player = new Player(0.0f, 0.0f, baseRadius, 0.001f);

    // Create initial enemies
    spawnEnemies(2);  // Start with 2 smart enemies instead of 3

    arrowActive = false;
    mouseWasPressed = false;

    // Generate rectangle vertices for health bar
    std::vector<float> rectVertices = {
        // positions (x, y)
        0.0f, 0.0f,  // bottom left
        1.0f, 0.0f,  // bottom right
        1.0f, 1.0f,  // top right
        0.0f, 0.0f,  // bottom left
        1.0f, 1.0f,  // top right
        0.0f, 1.0f   // top left
    };
    
    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectVBO);
    glBindVertexArray(rectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, rectVertices.size() * sizeof(float), rectVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Initialize the sword
    initSword();
    initArrow();
    initTerrain();
    std::cout << "Sword initialized with VAO ID: " << swordVAO << std::endl;

    return true;
}

void Game::processInput() {
    // Check ESC key.
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    // Update player input with deltaTime
    player->update(window, deltaTime);

    // Handle arrow firing on left mouse click (debounced)
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (!arrowActive && !mouseWasPressed) {
            double mouseX, mouseY;
            glfwGetCursorPos(window, &mouseX, &mouseY);
            float ndcX = static_cast<float>(mouseX) / screenWidth * 2.0f - 1.0f;
            float ndcY = 1.0f - static_cast<float>(mouseY) / screenHeight * 2.0f;
            float aspect = static_cast<float>(screenWidth) / screenHeight;
            float worldX = ndcX * aspect;
            float worldY = ndcY;
            float dirX = worldX - player->x;
            float dirY = worldY - player->y;
            float len = std::sqrt(dirX * dirX + dirY * dirY);
            if (len != 0) {
                dirX /= len;
                dirY /= len;
            }
            arrow.x = player->x;
            arrow.y = player->y;
            arrow.vx = arrowSpeed * dirX;
            arrow.vy = arrowSpeed * dirY;
            arrow.radius = baseRadius * 0.2f;
            // Calculate arrow rotation angle based on direction
            arrow.angle = atan2(dirY, dirX) - 3.14159f / 2.0f; // Subtract 90 degrees since arrow points up by default
            arrowActive = true;
            mouseWasPressed = true;
        }
    }
    else {
        mouseWasPressed = false;
    }
    
    // Handle sword swing on right mouse click - SIMPLIFIED
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        if (!sword.isSwinging && sword.cooldownTimer <= 0 && !rightMouseWasPressed && !player->isDead) {
            // Start sword attack - simple and reliable
            sword.isSwinging = true;
            sword.swingProgress = 0.0f;
            rightMouseWasPressed = true;
            
            std::cout << "Starting sword attack!" << std::endl;
        }
    }
    else {
        rightMouseWasPressed = false;
    }
}

void Game::spawnEnemies(int count) {
    float aspect = static_cast<float>(screenWidth) / screenHeight;
    
    for (int i = 0; i < count; i++) {
        // Only spawn if we have less than the maximum number of enemies
        if (enemies.size() < maxEnemies) {
            // Spawn enemies at random positions around the player
            float angle = randomFloat(0, 2 * 3.14159f);
            float distance = randomFloat(1.0f, 1.8f);  // Spawn farther away from the player
            
            float spawnX = player->x + std::cos(angle) * distance;
            float spawnY = player->y + std::sin(angle) * distance;
            
            // Clamp to screen boundaries
            if (spawnX < -aspect + baseRadius) spawnX = -aspect + baseRadius;
            if (spawnX > aspect - baseRadius) spawnX = aspect - baseRadius;
            if (spawnY < -1.0f + baseRadius) spawnY = -1.0f + baseRadius;
            if (spawnY > 1.0f - baseRadius) spawnY = 1.0f - baseRadius;
            
            // Add new enemy
            enemies.push_back(Enemy(spawnX, spawnY, baseRadius, enemySpeed));
        }
    }
}

void Game::update() {
    processInput();

    // For testing purposes, damage player every few seconds (T key)
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
        damageTimer += deltaTime;
        if (damageTimer >= damageCooldown) {
            player->takeDamage(10);
            damageTimer = 0.0f;
        }
    }
    
    // For testing purposes, heal player (H key)
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
        player->heal(5);
    }
    
    // For testing purposes, kill player instantly (K key)
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS && !player->isDead) {
        player->takeDamage(player->currentHealth);
    }

    // Update sword
    updateSword();

    // Spawn enemies periodically
    if (enemies.size() < maxEnemies) {
        enemySpawnTimer += deltaTime;
        if (enemySpawnTimer >= enemySpawnInterval) {
            spawnEnemies(1);  // Spawn one enemy at a time
            enemySpawnTimer = 0.0f;
        }
    }

    // Update arrows
    if (arrowActive) {
        arrow.x += arrow.vx * deltaTime * 60.0f;
        arrow.y += arrow.vy * deltaTime * 60.0f;
        
        // Continuously update arrow angle based on current velocity direction
        if (arrow.vx != 0.0f || arrow.vy != 0.0f) {
            arrow.angle = atan2(arrow.vy, arrow.vx) - 3.14159f / 2.0f;
        }
        
        float aspect = static_cast<float>(screenWidth) / screenHeight;
        if (arrow.x < -aspect + arrow.radius || arrow.x > aspect - arrow.radius ||
            arrow.y < -1.0f + arrow.radius || arrow.y > 1.0f - arrow.radius)
            arrowActive = false;
            
        // Check collision with enemies
        for (auto& enemy : enemies) {
            if (!enemy.isDead) {
                float dx = arrow.x - enemy.x;
                float dy = arrow.y - enemy.y;
                float d = std::sqrt(dx * dx + dy * dy);
                if (d < arrow.radius + enemy.radius) {
                    enemy.takeDamage(10); // Arrow does 10 damage
                    arrowActive = false;
                    break;
                }
            }
        }
    }

    // Update enemies and check for enemy arrow hits on player
    for (auto& enemy : enemies) {
        // Update enemy with deltaTime
        enemy.update(player->x, player->y, deltaTime);
        
        // Handle collision with player (both directions)
        if (!player->isDead && !enemy.isDead) {
            enemy.handleCollision(player->x, player->y, player->radius, deltaTime);
            player->handleCollision(enemy.x, enemy.y, enemy.radius, deltaTime);
            
            // Check for melee attacks on player
            if (!player->isInvulnerable) {
                int meleeDamage = 0;
                if (enemy.checkMeleeHit(player->x, player->y, player->radius, meleeDamage)) {
                    player->takeDamage(meleeDamage);
                }
            }
        }
        
        // Check if any enemy arrows hit the player
        if (!player->isDead && !player->isInvulnerable) {
            int damage = 0;
            if (enemy.checkArrowHit(player->x, player->y, player->radius, damage)) {
                player->takeDamage(damage);
            }
        }
    }

    // Handle enemy-to-enemy collisions
    for (size_t i = 0; i < enemies.size(); i++) {
        for (size_t j = i + 1; j < enemies.size(); j++) {
            if (!enemies[i].isDead && !enemies[j].isDead) {
                enemies[i].handleCollision(enemies[j].x, enemies[j].y, enemies[j].radius, deltaTime);
                enemies[j].handleCollision(enemies[i].x, enemies[i].y, enemies[i].radius, deltaTime);
            }
        }
    }

    // Boundary clamping for player
    float aspect = static_cast<float>(screenWidth) / screenHeight;
    if (player->x < -aspect + player->radius) player->x = -aspect + player->radius;
    if (player->x > aspect - player->radius) player->x = aspect - player->radius;
    if (player->y < -1.0f + player->radius) player->y = -1.0f + player->radius;
    if (player->y > 1.0f - player->radius) player->y = 1.0f - player->radius;
    
    // Boundary clamping for enemies
    for (auto& enemy : enemies) {
        if (enemy.x < -aspect + enemy.radius) enemy.x = -aspect + enemy.radius;
        if (enemy.x > aspect - enemy.radius) enemy.x = aspect - enemy.radius;
        if (enemy.y < -1.0f + enemy.radius) enemy.y = -1.0f + enemy.radius;
        if (enemy.y > 1.0f - enemy.radius) enemy.y = 1.0f - enemy.radius;
    }
}

void Game::renderHealthBar() {
    // Bind the rectangle VAO
    glBindVertexArray(rectVAO);
    
    // Set up health bar position and size (top left)
    float aspect = static_cast<float>(screenWidth) / screenHeight;
    float barWidth = 0.3f;
    float barHeight = 0.05f;
    float barPosX = -aspect + 0.05f; // 0.05f padding from left edge
    float barPosY = 1.0f - barHeight - 0.05f; // 0.05f padding from top edge
    
    // Draw health bar background (dark red)
    shaderProgram->setVec4("uColor", 0.4f, 0.1f, 0.1f, 1.0f);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(barPosX, barPosY, 0.0f));
    model = glm::scale(model, glm::vec3(barWidth, barHeight, 1.0f));
    shaderProgram->setMat4("uModel", glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // Draw health bar fill (bright red) - scale based on current health
    float healthPercentage = player->getHealthPercentage();
    if (healthPercentage > 0.0f) { // Only render if there's health remaining
        shaderProgram->setVec4("uColor", 0.9f, 0.2f, 0.2f, 1.0f);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(barPosX, barPosY, 0.0f));
        model = glm::scale(model, glm::vec3(barWidth * healthPercentage, barHeight, 1.0f));
        shaderProgram->setMat4("uModel", glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    glBindVertexArray(0);
}

void Game::renderEnemyHealthBar(const Enemy& enemy) {
    if (enemy.isDead) return; // Don't render health bar for dead enemies
    
    // Bind the rectangle VAO
    glBindVertexArray(rectVAO);
    
    // Set up health bar position and size (above the enemy)
    float barWidth = 0.1f;  // Smaller than player's health bar
    float barHeight = 0.02f;
    float barPosX = enemy.x - barWidth / 2.0f; // Center bar above enemy
    float barPosY = enemy.y + enemy.radius + 0.02f; // Position above enemy with small gap
    
    // Draw health bar background (dark red)
    shaderProgram->setVec4("uColor", 0.4f, 0.1f, 0.1f, 1.0f);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(barPosX, barPosY, 0.0f));
    model = glm::scale(model, glm::vec3(barWidth, barHeight, 1.0f));
    shaderProgram->setMat4("uModel", glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // Draw remaining health (bright red) - scale based on current health
    float healthPercentage = enemy.getHealthPercentage();
    if (healthPercentage > 0.0f) { // Only render if there's health remaining
        shaderProgram->setVec4("uColor", 0.9f, 0.2f, 0.2f, 1.0f);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(barPosX, barPosY, 0.0f));
        model = glm::scale(model, glm::vec3(barWidth * healthPercentage, barHeight, 1.0f));
        shaderProgram->setMat4("uModel", glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    glBindVertexArray(0);
}

void Game::renderDeathScreen() {
    // Calculate how long the player has been dead
    float timeSinceDeath = static_cast<float>(glfwGetTime()) - player->timeOfDeath;
    
    // If player has been dead longer than timeout, show death screen
    if (timeSinceDeath >= deathScreenTimeout) {
        // Bind the rectangle VAO
        glBindVertexArray(rectVAO);
        
        // Fill the screen with dark overlay
        float aspect = static_cast<float>(screenWidth) / screenHeight;
        shaderProgram->setVec4("uColor", 0.0f, 0.0f, 0.0f, 0.7f); // Semi-transparent black
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-aspect, -1.0f, 0.0f)); // Bottom-left corner
        model = glm::scale(model, glm::vec3(2.0f * aspect, 2.0f, 1.0f)); // Scale to fill screen
        shaderProgram->setMat4("uModel", glm::value_ptr(model));
        
        // Enable blending for transparent overlay
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // Render text with proper font rendering
        if (gameFont) {
            // Center "YOU DIED!" text on screen
            const std::string deathMessage = "YOU DIED!";
            float textScale = 2.0f;
            
            // In LearnOpenGL tutorial, text coordinates are from the baseline
            // Calculate width of text (approximate)
            float textWidth = deathMessage.length() * 20.0f * textScale;
            
            // Position text in center of screen
            float textX = (screenWidth - textWidth) / 2.0f;
            float textY = screenHeight / 2.0f;
            
            // Draw "YOU DIED!" text - red color
            gameFont->renderText(deathMessage, textX, textY, textScale, glm::vec3(0.8f, 0.0f, 0.0f));
            
            // Draw instruction text below
            const std::string exitMessage = "Press ESC to exit";
            float instructionScale = 1.0f;
            
            // Position instruction text below main message
            float instructionWidth = exitMessage.length() * 10.0f * instructionScale;
            float instructionX = (screenWidth - instructionWidth) / 2.0f;
            float instructionY = textY - 50.0f;
            
            gameFont->renderText(exitMessage, instructionX, instructionY, instructionScale, glm::vec3(1.0f, 1.0f, 1.0f));
        }
        
        // Disable blending
        glDisable(GL_BLEND);
        
        glBindVertexArray(0);
    }
}

void Game::render() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background
    glClear(GL_COLOR_BUFFER_BIT);
    shaderProgram->use();
    shaderProgram->setMat4("uProjection", glm::value_ptr(projection));

    // Render background tiles first
    renderTerrain();

    // Set up for player rendering
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(player->x, player->y, 0.0f));
    model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
    shaderProgram->setMat4("uModel", glm::value_ptr(model));
    shaderProgram->setVec2("uOffset", 0.0f, 0.0f); // Reset offset
    shaderProgram->setFloat("uScale", 1.0f);
    
    // Draw player
    if (player->isDead) {
        // Dead player (red)
        shaderProgram->setVec4("uColor", 0.7f, 0.0f, 0.0f, 1.0f); // Red
    } else if (player->isInvulnerable) {
        // Flash the player white during invulnerability
        int flashInterval = static_cast<int>(player->invulnerabilityTimer * 10) % 2;
        if (flashInterval == 0) {
            shaderProgram->setVec4("uColor", 1.0f, 1.0f, 1.0f, 1.0f); // White
        } else {
            shaderProgram->setVec4("uColor", 0.2f, 0.7f, 0.3f, 1.0f); // Green
        }
    } else {
        // Normal player (green)
        shaderProgram->setVec4("uColor", 0.2f, 0.7f, 0.3f, 1.0f);
    }
    
    glBindVertexArray(circleVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, segments + 2);
    glBindVertexArray(0);
    
    // Render the sword
    renderSword();

    // Render player arrow with proper arrow shape
    renderArrow();

    // Draw all enemies and their arrows
    for (const auto& enemy : enemies) {
        if (!enemy.isDead) {
            // Set enemy color based on AI state
            switch (enemy.currentState) {
                case Enemy::WANDERING:
                    shaderProgram->setVec4("uColor", 0.6f, 0.4f, 0.4f, 1.0f); // Dark red (idle)
                    break;
                case Enemy::DETECTING:
                    shaderProgram->setVec4("uColor", 0.9f, 0.6f, 0.2f, 1.0f); // Orange (searching)
                    break;
                case Enemy::FOLLOWING:
                    shaderProgram->setVec4("uColor", 0.8f, 0.3f, 0.3f, 1.0f); // Red (following)
                    break;
                case Enemy::ATTACKING:
                    shaderProgram->setVec4("uColor", 1.0f, 0.2f, 0.2f, 1.0f); // Bright red (attacking)
                    break;
                case Enemy::FLEEING:
                    shaderProgram->setVec4("uColor", 0.7f, 0.2f, 0.8f, 1.0f); // Purple (fleeing)
                    break;
                default:
                    shaderProgram->setVec4("uColor", 0.8f, 0.2f, 0.2f, 1.0f); // Default red
                    break;
            }
            
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(enemy.x, enemy.y, 0.0f));
            model = glm::scale(model, glm::vec3(enemy.radius / baseRadius, enemy.radius / baseRadius, 1.0f));
            shaderProgram->setMat4("uModel", glm::value_ptr(model));
            glBindVertexArray(circleVAO);
            glDrawArrays(GL_TRIANGLE_FAN, 0, segments + 2);
            glBindVertexArray(0);
            
            // Draw enemy health bar
            renderEnemyHealthBar(enemy);
            
            // Draw enemy arrows
            for (const auto& arrow : enemy.arrows) {
                if (arrow.active) {
                    shaderProgram->setVec4("uColor", 0.8f, 0.6f, 0.0f, 1.0f); // Orange for enemy arrows
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(arrow.x, arrow.y, 0.0f));
                    model = glm::scale(model, glm::vec3(arrow.radius / baseRadius, arrow.radius / baseRadius, 1.0f));
                    shaderProgram->setMat4("uModel", glm::value_ptr(model));
                    glBindVertexArray(circleVAO);
                    glDrawArrays(GL_TRIANGLE_FAN, 0, segments + 2);
                    glBindVertexArray(0);
                }
            }
        }
    }

    // Reset for health bar and death screen
    model = glm::mat4(1.0f);
    shaderProgram->setMat4("uModel", glm::value_ptr(model));
    shaderProgram->setVec2("uOffset", 0.0f, 0.0f);
    shaderProgram->setFloat("uScale", 1.0f);

    // Draw health bar (only if player is alive)
    if (!player->isDead) {
        renderHealthBar();
    }
    
    // Draw death screen if player is dead
    if (player->isDead) {
        renderDeathScreen();
    }

    glfwSwapBuffers(window);
}

void Game::cleanup() {
    if (gameFont) {
        delete gameFont;
        gameFont = nullptr;
    }
    
    if (textShader) {
        delete textShader;
        textShader = nullptr;
    }
    
    if (shaderProgram) {
        delete shaderProgram;
        shaderProgram = nullptr;
    }
    
    if (player) {
        delete player;
        player = nullptr;
    }
    
    glDeleteVertexArrays(1, &circleVAO);
    glDeleteBuffers(1, &circleVBO);
    glDeleteVertexArrays(1, &rectVAO);
    glDeleteBuffers(1, &rectVBO);
    glDeleteVertexArrays(1, &swordVAO);
    glDeleteBuffers(1, &swordVBO);
    glDeleteVertexArrays(1, &arrowVAO);
    glDeleteBuffers(1, &arrowVBO);
    glDeleteVertexArrays(1, &tileVAO);
    glDeleteBuffers(1, &tileVBO);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Game::run() {
    lastFrameTime = glfwGetTime();
    
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        double currentTime = glfwGetTime();
        deltaTime = static_cast<float>(currentTime - lastFrameTime);
        lastFrameTime = currentTime;
        
        processInput();
        update();
        render();
        glfwPollEvents();
    }
}

void Game::initSword() {
    // Create vertices for a 2D sword shape
    // The sword will be composed of multiple geometric shapes (handle, guard, blade)
    swordVertices.clear();
    
    // Define sword relative to origin (0,0)
    float bladeLength = sword.length * 0.75f;  // 75% of total length is the blade
    float handleLength = sword.length * 0.25f; // 25% of total length is the handle
    float bladeWidth = sword.width;
    float handleWidth = sword.width * 0.5f;
    float guardWidth = sword.width * 3.0f;     // Wider guard for more medieval look
    float guardHeight = sword.width * 0.6f;
    
    // Blade vertices with a more tapered look (pointed at the end)
    swordVertices.push_back(0.0f);                     // Blade tip x
    swordVertices.push_back(bladeLength);              // Blade tip y
    
    swordVertices.push_back(-bladeWidth/2.0f);         // Blade left edge x
    swordVertices.push_back(guardHeight/2.0f);         // Blade bottom left y
    
    swordVertices.push_back(bladeWidth/2.0f);          // Blade right edge x
    swordVertices.push_back(guardHeight/2.0f);         // Blade bottom right y
    
    // Add an additional triangle to make the blade look more detailed (fuller/blood groove)
    swordVertices.push_back(0.0f);                     // Center point x
    swordVertices.push_back(bladeLength * 0.85f);      // Center point y (85% up the blade)
    
    swordVertices.push_back(-bladeWidth * 0.3f);       // Left edge x (narrower than blade)
    swordVertices.push_back(guardHeight/2.0f + bladeLength * 0.2f); // Left y (20% up the blade)
    
    swordVertices.push_back(bladeWidth * 0.3f);        // Right edge x (narrower than blade)
    swordVertices.push_back(guardHeight/2.0f + bladeLength * 0.2f); // Right y (20% up the blade)
    
    // Guard vertices (with a crossguard shape for medieval style)
    swordVertices.push_back(-guardWidth/2.0f);         // Guard left x
    swordVertices.push_back(guardHeight/2.0f);         // Guard top y
    
    swordVertices.push_back(guardWidth/2.0f);          // Guard right x
    swordVertices.push_back(guardHeight/2.0f);         // Guard top y
    
    swordVertices.push_back(guardWidth/2.0f);          // Guard right x 
    swordVertices.push_back(-guardHeight/2.0f);        // Guard bottom y
    
    swordVertices.push_back(-guardWidth/2.0f);         // Guard left x
    swordVertices.push_back(-guardHeight/2.0f);        // Guard bottom y
    
    // Handle vertices (rectangular) with pommel
    swordVertices.push_back(-handleWidth/2.0f);        // Handle left x
    swordVertices.push_back(-guardHeight/2.0f);        // Handle top y
    
    swordVertices.push_back(handleWidth/2.0f);         // Handle right x
    swordVertices.push_back(-guardHeight/2.0f);        // Handle top y
    
    swordVertices.push_back(handleWidth/2.0f);         // Handle right x
    swordVertices.push_back(-guardHeight/2.0f - handleLength); // Handle bottom y
    
    swordVertices.push_back(-handleWidth/2.0f);        // Handle left x
    swordVertices.push_back(-guardHeight/2.0f - handleLength); // Handle bottom y
    
    // Add a pommel (circular end cap for the handle)
    float pommelRadius = handleWidth * 0.8f;
    float pommelCenterY = -guardHeight/2.0f - handleLength - pommelRadius * 0.5f;
    
    // Pommel center
    swordVertices.push_back(0.0f);                    // Pommel center x
    swordVertices.push_back(pommelCenterY);           // Pommel center y
    
    // Add pommel vertices (simplified circle with 8 points)
    int pommelSegments = 8;
    for (int i = 0; i <= pommelSegments; ++i) {
        float angle = 2.0f * 3.14159265358979323846f * i / pommelSegments;
        float x = pommelRadius * 0.6f * cos(angle);  // Make it slightly oval
        float y = pommelRadius * sin(angle);
        
        swordVertices.push_back(x);                   // Pommel point x
        swordVertices.push_back(pommelCenterY + y);   // Pommel point y
    }
    
    // Set up VAO and VBO for sword
    glGenVertexArrays(1, &swordVAO);
    glGenBuffers(1, &swordVBO);
    
    glBindVertexArray(swordVAO);
    glBindBuffer(GL_ARRAY_BUFFER, swordVBO);
    glBufferData(GL_ARRAY_BUFFER, swordVertices.size() * sizeof(float), swordVertices.data(), GL_STATIC_DRAW);
    
    // Our vertices are 2 floats per vertex (x, y)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

void Game::initArrow() {
    // Create vertices for a 2D arrow shape
    arrowVertices.clear();
    
    // Define arrow relative to origin (0,0), pointing upward (positive Y direction)
    // Make arrow much smaller
    float arrowLength = baseRadius * 1.5f;  // Much smaller - was 4.0f
    float headLength = arrowLength * 0.4f;  // 40% of length is the arrowhead
    float shaftLength = arrowLength * 0.6f; // 60% of length is the shaft
    float headWidth = baseRadius * 0.6f;    // Much smaller width - was 1.5f
    float shaftWidth = baseRadius * 0.15f;  // Much thinner shaft - was 0.3f
    float fletchingLength = arrowLength * 0.25f; // Length of fletching
    float fletchingWidth = baseRadius * 0.4f;   // Smaller fletching - was 0.8f
    
    // Arrowhead (triangle pointing up)
    arrowVertices.push_back(0.0f);                    // Tip x
    arrowVertices.push_back(arrowLength);             // Tip y
    
    arrowVertices.push_back(-headWidth/2.0f);         // Left base x
    arrowVertices.push_back(arrowLength - headLength); // Left base y
    
    arrowVertices.push_back(headWidth/2.0f);          // Right base x
    arrowVertices.push_back(arrowLength - headLength); // Right base y
    
    // Arrow shaft (rectangle)
    arrowVertices.push_back(-shaftWidth/2.0f);        // Shaft left x
    arrowVertices.push_back(arrowLength - headLength); // Shaft top y
    
    arrowVertices.push_back(shaftWidth/2.0f);         // Shaft right x
    arrowVertices.push_back(arrowLength - headLength); // Shaft top y
    
    arrowVertices.push_back(shaftWidth/2.0f);         // Shaft right x
    arrowVertices.push_back(fletchingLength);         // Shaft bottom y
    
    arrowVertices.push_back(-shaftWidth/2.0f);        // Shaft left x
    arrowVertices.push_back(fletchingLength);         // Shaft bottom y
    
    // Fletching (feathers at the back) - left side
    arrowVertices.push_back(-shaftWidth/2.0f);        // Fletching start left x
    arrowVertices.push_back(fletchingLength);         // Fletching start y
    
    arrowVertices.push_back(-fletchingWidth/2.0f);    // Fletching end left x
    arrowVertices.push_back(0.0f);                    // Fletching end y
    
    arrowVertices.push_back(-shaftWidth/4.0f);        // Fletching middle x
    arrowVertices.push_back(fletchingLength * 0.5f);  // Fletching middle y
    
    // Fletching (feathers at the back) - right side
    arrowVertices.push_back(shaftWidth/2.0f);         // Fletching start right x
    arrowVertices.push_back(fletchingLength);         // Fletching start y
    
    arrowVertices.push_back(fletchingWidth/2.0f);     // Fletching end right x
    arrowVertices.push_back(0.0f);                    // Fletching end y
    
    arrowVertices.push_back(shaftWidth/4.0f);         // Fletching middle x
    arrowVertices.push_back(fletchingLength * 0.5f);  // Fletching middle y
    
    // Set up VAO and VBO for arrow
    glGenVertexArrays(1, &arrowVAO);
    glGenBuffers(1, &arrowVBO);
    
    glBindVertexArray(arrowVAO);
    glBindBuffer(GL_ARRAY_BUFFER, arrowVBO);
    glBufferData(GL_ARRAY_BUFFER, arrowVertices.size() * sizeof(float), arrowVertices.data(), GL_STATIC_DRAW);
    
    // Our vertices are 2 floats per vertex (x, y)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

void Game::updateSword() {
    // Update sword cooldown timer
    if (sword.cooldownTimer > 0) {
        sword.cooldownTimer -= deltaTime;
        if (sword.cooldownTimer < 0) {
            sword.cooldownTimer = 0;
        }
    }
    
    // Simple, reliable sword positioning
    float currentTime = static_cast<float>(glfwGetTime());
    
    // If not swinging, sword orbits around player
    if (!sword.isSwinging) {
        // Simple orbital motion
        float orbitAngle = currentTime * 0.5f; // Slow orbit
        float orbitDistance = player->radius * 2.5f;
        
        sword.offsetX = cos(orbitAngle) * orbitDistance;
        sword.offsetY = sin(orbitAngle) * orbitDistance;
        sword.angle = orbitAngle;
    }
    // If swinging, do simple attack animation
    else {
        // Simple progress increment
        sword.swingProgress += sword.swingSpeed * deltaTime * 60.0f;
        
        // If animation is complete, reset everything
        if (sword.swingProgress >= 1.0f) {
            // Reset to normal state
            sword.isSwinging = false;
            sword.swingProgress = 0.0f;
            sword.cooldownTimer = sword.cooldown;
            
            // Immediately set to current orbital position
            float orbitAngle = currentTime * 0.5f;
            float orbitDistance = player->radius * 2.5f;
            sword.offsetX = cos(orbitAngle) * orbitDistance;
            sword.offsetY = sin(orbitAngle) * orbitDistance;
            sword.angle = orbitAngle;
            
            std::cout << "Sword attack finished, reset to orbit" << std::endl;
        }
        else {
            // Simple 3-phase attack animation
            float attackDistance = player->radius * 1.8f; // Closer during attack
            
            if (sword.swingProgress < 0.33f) {
                // Phase 1: Move to attack position (33% of animation)
                float phase1 = sword.swingProgress / 0.33f;
                float startAngle = sword.angle;
                float targetAngle = startAngle + (3.14159f / 2.0f); // 90 degrees
                
                float currentAngle = startAngle + (targetAngle - startAngle) * phase1;
                sword.offsetX = cos(currentAngle) * attackDistance;
                sword.offsetY = sin(currentAngle) * attackDistance;
                sword.angle = currentAngle;
            }
            else if (sword.swingProgress < 0.66f) {
                // Phase 2: Swing around player (33% of animation)
                float phase2 = (sword.swingProgress - 0.33f) / 0.33f;
                float swingAngle = sword.angle + (3.14159f * 2.0f * phase2); // Full circle
                
                sword.offsetX = cos(swingAngle) * attackDistance;
                sword.offsetY = sin(swingAngle) * attackDistance;
                sword.angle = swingAngle;
                
                // Check for enemy hits during this phase
                for (auto& enemy : enemies) {
                    if (!enemy.isDead) {
                        if (checkSwordHit(enemy.x, enemy.y, enemy.radius)) {
                            enemy.takeDamage(sword.damage);
                        }
                    }
                }
            }
            else {
                // Phase 3: Return to orbit position (33% of animation)
                float phase3 = (sword.swingProgress - 0.66f) / 0.34f;
                float currentAngle = sword.angle;
                float targetOrbitAngle = currentTime * 0.5f;
                float targetDistance = player->radius * 2.5f;
                
                // Interpolate back to orbit
                float finalAngle = currentAngle + (targetOrbitAngle - currentAngle) * phase3;
                float finalDistance = attackDistance + (targetDistance - attackDistance) * phase3;
                
                sword.offsetX = cos(finalAngle) * finalDistance;
                sword.offsetY = sin(finalAngle) * finalDistance;
                sword.angle = finalAngle;
            }
        }
    }
    
    // Debug output
    std::cout << "Sword state - Swinging: " << sword.isSwinging 
              << ", Progress: " << sword.swingProgress 
              << ", Position: (" << sword.offsetX << ", " << sword.offsetY << ")" << std::endl;
}

bool Game::checkSwordHit(float targetX, float targetY, float targetRadius) {
    // Calculate the position of the sword's hitbox
    float hitboxX = player->x + sword.offsetX;
    float hitboxY = player->y + sword.offsetY;
    
    // Check if the sword's hitbox overlaps with the target
    float dx = hitboxX - targetX;
    float dy = hitboxY - targetY;
    float distance = sqrt(dx * dx + dy * dy);
    
    // Return true if the sword hit the target
    return distance < (sword.hitboxRadius + targetRadius);
}

void Game::renderSword() {
    if (player->isDead) return; // Don't render sword if player is dead
    
    // Simple check - if VAO is invalid, reinitialize
    if (swordVAO == 0) {
        initSword();
        if (swordVAO == 0) return; // Still failed, skip this frame
    }
    
    // Bind the sword VAO
    glBindVertexArray(swordVAO);
    
    // Set sword color based on state
    if (sword.isSwinging) {
        // Brighter during attack
        shaderProgram->setVec4("uColor", 0.8f, 0.9f, 1.0f, 1.0f);
    } else {
        // Normal steel blue
        shaderProgram->setVec4("uColor", 0.6f, 0.7f, 0.9f, 1.0f);
    }
    
    // Create model matrix for sword
    glm::mat4 model = glm::mat4(1.0f);
    
    // Position the sword at player position + offset
    model = glm::translate(model, glm::vec3(player->x + sword.offsetX, player->y + sword.offsetY, 0.0f));
    
    // Rotate the sword to point outward from player
    float pointingAngle = atan2(sword.offsetY, sword.offsetX) - 3.14159f / 2.0f;
    model = glm::rotate(model, pointingAngle, glm::vec3(0.0f, 0.0f, 1.0f));
    
    // Apply the model matrix
    shaderProgram->setMat4("uModel", glm::value_ptr(model));
    
    // Simple scale effect during swing
    float scale = sword.isSwinging ? 1.2f : 1.0f;
    shaderProgram->setFloat("uScale", scale);
    
    // Draw the sword parts with different colors
    
    // Main blade triangle
    shaderProgram->setVec4("uColor", 0.7f, 0.8f, 0.95f, 1.0f);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    // Blade detail/fuller
    shaderProgram->setVec4("uColor", 0.5f, 0.6f, 0.8f, 1.0f);
    glDrawArrays(GL_TRIANGLES, 3, 3);
    
    // Guard - gold color
    shaderProgram->setVec4("uColor", 0.9f, 0.7f, 0.2f, 1.0f);
    glDrawArrays(GL_TRIANGLE_FAN, 6, 4);
    
    // Handle - brown color
    shaderProgram->setVec4("uColor", 0.6f, 0.3f, 0.1f, 1.0f);
    glDrawArrays(GL_TRIANGLE_FAN, 10, 4);
    
    // Pommel - gold to match guard
    shaderProgram->setVec4("uColor", 0.9f, 0.7f, 0.2f, 1.0f);
    glDrawArrays(GL_TRIANGLE_FAN, 14, 10);
    
    // Reset scale
    shaderProgram->setFloat("uScale", 1.0f);
    
    glBindVertexArray(0);
}

void Game::renderArrow() {
    if (!arrowActive || player->isDead) return; // Don't render if arrow is not active or player is dead
    
    // Simple check - if VAO is invalid, reinitialize
    if (arrowVAO == 0) {
        initArrow();
        if (arrowVAO == 0) return; // Still failed, skip this frame
    }
    
    // Bind the arrow VAO
    glBindVertexArray(arrowVAO);
    
    // Create model matrix for arrow
    glm::mat4 model = glm::mat4(1.0f);
    
    // Position the arrow at its current location
    model = glm::translate(model, glm::vec3(arrow.x, arrow.y, 0.0f));
    
    // Rotate the arrow to point in its direction of travel
    model = glm::rotate(model, arrow.angle, glm::vec3(0.0f, 0.0f, 1.0f));
    
    // Scale the arrow to be much smaller
    float arrowScale = 0.4f; // Much smaller scale - was 0.8f
    model = glm::scale(model, glm::vec3(arrowScale, arrowScale, 1.0f));
    
    // Apply the model matrix
    shaderProgram->setMat4("uModel", glm::value_ptr(model));
    
    // Draw the arrow parts with different colors
    
    // Arrowhead (metallic silver/gray)
    shaderProgram->setVec4("uColor", 0.8f, 0.8f, 0.9f, 1.0f);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    // Arrow shaft (brown wood color)
    shaderProgram->setVec4("uColor", 0.6f, 0.4f, 0.2f, 1.0f);
    glDrawArrays(GL_TRIANGLE_FAN, 3, 4);
    
    // Fletching left (feather color - reddish brown)
    shaderProgram->setVec4("uColor", 0.7f, 0.3f, 0.2f, 1.0f);
    glDrawArrays(GL_TRIANGLES, 7, 3);
    
    // Fletching right (feather color - reddish brown)
    shaderProgram->setVec4("uColor", 0.7f, 0.3f, 0.2f, 1.0f);
    glDrawArrays(GL_TRIANGLES, 10, 3);
    
    glBindVertexArray(0);
}

void Game::initTerrain() {
    // Create a simple rectangle for terrain element rendering
    tileVertices = {
        // positions (x, y) for a unit square
        0.0f, 0.0f,  // bottom left
        1.0f, 0.0f,  // bottom right
        1.0f, 1.0f,  // top right
        0.0f, 0.0f,  // bottom left
        1.0f, 1.0f,  // top right
        0.0f, 1.0f   // top left
    };
    
    // Set up VAO and VBO for terrain elements
    glGenVertexArrays(1, &tileVAO);
    glGenBuffers(1, &tileVBO);
    
    glBindVertexArray(tileVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tileVBO);
    glBufferData(GL_ARRAY_BUFFER, tileVertices.size() * sizeof(float), tileVertices.data(), GL_STATIC_DRAW);
    
    // Our vertices are 2 floats per vertex (x, y)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
    
    // Generate the scattered terrain elements
    generateTerrain();
}

void Game::generateTerrain() {
    if (terrainGenerated) return;
    
    // Calculate world bounds
    float aspect = static_cast<float>(screenWidth) / screenHeight;
    float worldWidth = 2.0f * aspect;
    float worldHeight = 2.0f;
    
    // Clear any existing terrain elements
    terrainElements.clear();
    
    // Generate scattered terrain elements
    int numElements = 150; // Total number of terrain elements to scatter
    
    for (int i = 0; i < numElements; i++) {
        TerrainElement element;
        
        // Random position across the world
        element.x = randomFloat(-aspect + 0.1f, aspect - 0.1f);
        element.y = randomFloat(-1.0f + 0.1f, 1.0f - 0.1f);
        
        // Check for overlap with existing elements (minimum distance)
        bool tooClose = false;
        float minDistance = 0.15f; // Minimum distance between elements
        
        for (const auto& existing : terrainElements) {
            float dx = element.x - existing.x;
            float dy = element.y - existing.y;
            float distance = sqrt(dx * dx + dy * dy);
            
            if (distance < minDistance) {
                tooClose = true;
                break;
            }
        }
        
        // Skip this element if it's too close to another
        if (tooClose) {
            i--; // Try again
            continue;
        }
        
        // Random element type
        int typeRand = rand() % 100;
        if (typeRand < 35) {
            element.type = GRASS_BLADE;
            element.size = randomFloat(0.02f, 0.04f);
        } else if (typeRand < 55) {
            element.type = STONE_ROCK;
            element.size = randomFloat(0.03f, 0.06f);
        } else if (typeRand < 75) {
            element.type = DIRT_PATCH;
            element.size = randomFloat(0.025f, 0.045f);
        } else if (typeRand < 90) {
            element.type = COBBLE_STONE;
            element.size = randomFloat(0.04f, 0.07f);
        } else {
            element.type = SAND_GRAIN;
            element.size = randomFloat(0.015f, 0.025f);
        }
        
        // Random rotation for variety
        element.rotation = randomFloat(0.0f, 6.28318f); // 0 to 2π
        
        // Grayscale colors with variation
        float baseGray = 0.0f;
        float variation = randomFloat(0.7f, 1.0f);
        
        switch (element.type) {
            case GRASS_BLADE:
                baseGray = 0.4f * variation; // Medium gray
                break;
            case STONE_ROCK:
                baseGray = 0.6f * variation; // Light gray
                break;
            case DIRT_PATCH:
                baseGray = 0.3f * variation; // Dark gray
                break;
            case COBBLE_STONE:
                baseGray = 0.5f * variation; // Medium-light gray
                break;
            case SAND_GRAIN:
                baseGray = 0.7f * variation; // Lightest gray
                break;
        }
        
        element.color = glm::vec3(baseGray, baseGray, baseGray);
        element.rendered = false; // Will be rendered once and then marked as rendered
        
        terrainElements.push_back(element);
    }
    
    terrainGenerated = true;
    std::cout << "Generated " << static_cast<int>(terrainElements.size()) << " scattered terrain elements" << std::endl;
}

void Game::renderTerrain() {
    if (!terrainGenerated || tileVAO == 0) return;
    
    // Bind the terrain VAO
    glBindVertexArray(tileVAO);
    
    // Render all terrain elements
    for (auto& element : terrainElements) {
        // Set element color
        shaderProgram->setVec4("uColor", element.color.r, element.color.g, element.color.b, 1.0f);
        
        // Create model matrix for this element
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(element.x, element.y, 0.0f));
        model = glm::rotate(model, element.rotation, glm::vec3(0.0f, 0.0f, 1.0f));
        
        // Draw different shapes based on element type
        switch (element.type) {
            case GRASS_BLADE:
            {
                // Draw thin vertical rectangle for grass blade
                glm::mat4 grassModel = model;
                grassModel = glm::scale(grassModel, glm::vec3(element.size * 0.3f, element.size * 2.0f, 1.0f));
                shaderProgram->setMat4("uModel", glm::value_ptr(grassModel));
                glDrawArrays(GL_TRIANGLES, 0, 6);
                break;
            }
                
            case STONE_ROCK:
            {
                // Draw irregular stone shape (multiple small rectangles)
                glm::mat4 stoneModel = model;
                stoneModel = glm::scale(stoneModel, glm::vec3(element.size, element.size * 0.8f, 1.0f));
                shaderProgram->setMat4("uModel", glm::value_ptr(stoneModel));
                glDrawArrays(GL_TRIANGLES, 0, 6);
                
                // Add a smaller rectangle for irregular shape
                glm::mat4 model2 = glm::mat4(1.0f);
                model2 = glm::translate(model2, glm::vec3(element.x + element.size * 0.3f, element.y + element.size * 0.2f, 0.0f));
                model2 = glm::rotate(model2, element.rotation + 0.5f, glm::vec3(0.0f, 0.0f, 1.0f));
                model2 = glm::scale(model2, glm::vec3(element.size * 0.6f, element.size * 0.5f, 1.0f));
                shaderProgram->setMat4("uModel", glm::value_ptr(model2));
                glDrawArrays(GL_TRIANGLES, 0, 6);
                break;
            }
                
            case DIRT_PATCH:
            {
                // Draw small square for dirt patch
                glm::mat4 dirtModel = model;
                dirtModel = glm::scale(dirtModel, glm::vec3(element.size, element.size, 1.0f));
                shaderProgram->setMat4("uModel", glm::value_ptr(dirtModel));
                glDrawArrays(GL_TRIANGLES, 0, 6);
                break;
            }
                
            case COBBLE_STONE:
            {
                // Draw rectangular cobblestone
                glm::mat4 cobbleModel = model;
                cobbleModel = glm::scale(cobbleModel, glm::vec3(element.size * 1.2f, element.size * 0.8f, 1.0f));
                shaderProgram->setMat4("uModel", glm::value_ptr(cobbleModel));
                glDrawArrays(GL_TRIANGLES, 0, 6);
                
                // Add border lines for cobblestone effect
                float darkGray = element.color.r * 0.5f;
                shaderProgram->setVec4("uColor", darkGray, darkGray, darkGray, 1.0f);
                
                // Top border
                glm::mat4 borderModel = glm::mat4(1.0f);
                borderModel = glm::translate(borderModel, glm::vec3(element.x, element.y + element.size * 0.4f, 0.0f));
                borderModel = glm::rotate(borderModel, element.rotation, glm::vec3(0.0f, 0.0f, 1.0f));
                borderModel = glm::scale(borderModel, glm::vec3(element.size * 1.2f, element.size * 0.05f, 1.0f));
                shaderProgram->setMat4("uModel", glm::value_ptr(borderModel));
                glDrawArrays(GL_TRIANGLES, 0, 6);
                
                // Side border
                borderModel = glm::mat4(1.0f);
                borderModel = glm::translate(borderModel, glm::vec3(element.x + element.size * 0.6f, element.y, 0.0f));
                borderModel = glm::rotate(borderModel, element.rotation, glm::vec3(0.0f, 0.0f, 1.0f));
                borderModel = glm::scale(borderModel, glm::vec3(element.size * 0.05f, element.size * 0.8f, 1.0f));
                shaderProgram->setMat4("uModel", glm::value_ptr(borderModel));
                glDrawArrays(GL_TRIANGLES, 0, 6);
                break;
            }
                
            case SAND_GRAIN:
            {
                // Draw tiny square for sand grain
                glm::mat4 sandModel = model;
                sandModel = glm::scale(sandModel, glm::vec3(element.size, element.size, 1.0f));
                shaderProgram->setMat4("uModel", glm::value_ptr(sandModel));
                glDrawArrays(GL_TRIANGLES, 0, 6);
                break;
            }
        }
    }
    
    glBindVertexArray(0);
}
