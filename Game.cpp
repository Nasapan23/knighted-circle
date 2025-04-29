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
    segments(50), baseRadius(0.05f),
    enemySpeed(0.005f), maxEnemies(3), enemySpawnTimer(0.0f), enemySpawnInterval(5.0f),
    arrowActive(false), mouseWasPressed(false),
    arrowSpeed(0.02f), damageTimer(0.0f), damageCooldown(3.0f),
    rightMouseWasPressed(false),
    deathScreenTimeout(3.0f), lastFrameTime(0.0), deltaTime(0.0f)
{
    // Initialize sword parameters
    sword.offsetX = baseRadius * 2.5f;  // Position sword farther from player
    sword.offsetY = 0.0f;
    sword.angle = 0.0f;
    sword.length = baseRadius * 3.5f;   // Slightly longer sword
    sword.width = baseRadius * 0.4f;    // Slightly thinner sword blade
    sword.hitboxRadius = baseRadius * 1.5f; // Slightly larger hitbox for better hit detection
    sword.isSwinging = false;
    sword.swingSpeed = 0.03f;           // MUCH slower swing speed for better visibility
    sword.swingAngle = 3.14159f * 2.0f; // Not directly used in the new animation
    sword.swingProgress = 0.0f;
    sword.damage = 20;                  // Damage value
    sword.cooldown = 0.8f;              // Longer cooldown (0.8 seconds)
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
    spawnEnemies(3);  // Start with 3 enemies

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
            arrowActive = true;
            mouseWasPressed = true;
        }
    }
    else {
        mouseWasPressed = false;
    }
    
    // Handle sword swing on right mouse click (debounced)
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        if (!sword.isSwinging && sword.cooldownTimer <= 0 && !rightMouseWasPressed && !player->isDead) {
            // CRITICAL: Before starting swing, ensure sword position is valid
            // If the sword has disappeared, this will reset it
            float currentBaseAngle = glfwGetTime() * 0.4f;
            float distanceFromPlayer = player->radius * 2.5f;
            
            // Only reset if position is invalid (close to zero)
            if (std::abs(sword.offsetX) < 0.001f && std::abs(sword.offsetY) < 0.001f) {
                std::cout << "Fixing invalid sword position before swing!" << std::endl;
                sword.offsetX = cos(currentBaseAngle) * distanceFromPlayer;
                sword.offsetY = sin(currentBaseAngle) * distanceFromPlayer;
            }
            
            // Start a new sword swing
            sword.isSwinging = true;
            sword.swingProgress = 0.0f;
            sword.angle = currentBaseAngle; // Use current base angle as starting point
            rightMouseWasPressed = true;
            
            // Debug: Report sword swing start with position
            std::cout << "Starting sword swing at position: " << sword.offsetX << ", " << sword.offsetY << std::endl;
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
        
        // Check if any enemy arrows hit the player
        if (!player->isDead && !player->isInvulnerable) {
            int damage = 0;
            if (enemy.checkArrowHit(player->x, player->y, player->radius, damage)) {
                player->takeDamage(damage);
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
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    shaderProgram->use();
    shaderProgram->setMat4("uProjection", glm::value_ptr(projection));

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
    
    // CRITICAL: Render the sword hovering around the player
    // Always call this even if player is dead - the renderSword function 
    // has its own check to not render for dead players
    try {
        // Wrap in try/catch to prevent crashes if something goes wrong
        renderSword();
    } catch (const std::exception& e) {
        std::cerr << "ERROR rendering sword: " << e.what() << std::endl;
        // Attempt to fix the sword state
        sword.isSwinging = false;
        sword.swingProgress = 0.0f;
        float baseAngle = glfwGetTime() * 0.4f;
        float distanceFromPlayer = player->radius * 2.5f;
        sword.offsetX = cos(baseAngle) * distanceFromPlayer;
        sword.offsetY = sin(baseAngle) * distanceFromPlayer;
        sword.angle = baseAngle;
    }

    // Draw player arrow (yellow circle)
    if (arrowActive && !player->isDead) {  // Don't show arrow if player is dead
        shaderProgram->setVec4("uColor", 1.0f, 1.0f, 0.0f, 1.0f);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(arrow.x, arrow.y, 0.0f));
        model = glm::scale(model, glm::vec3(arrow.radius / baseRadius, arrow.radius / baseRadius, 1.0f));
        shaderProgram->setMat4("uModel", glm::value_ptr(model));
        glBindVertexArray(circleVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, segments + 2);
        glBindVertexArray(0);
    }

    // Draw all enemies and their arrows
    for (const auto& enemy : enemies) {
        if (!enemy.isDead) {
            // Draw enemy (red circle)
            shaderProgram->setVec4("uColor", 0.8f, 0.2f, 0.2f, 1.0f); // Red color for enemy
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
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Game::run() {
    lastFrameTime = glfwGetTime();
    float lastSwordCheck = 0.0f;
    
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        double currentTime = glfwGetTime();
        deltaTime = static_cast<float>(currentTime - lastFrameTime);
        lastFrameTime = currentTime;
        
        // Periodic check to ensure sword is visible
        if (currentTime - lastSwordCheck > 5.0f) {
            lastSwordCheck = static_cast<float>(currentTime);
            
            // Check sword state
            if (std::abs(sword.offsetX) < 0.001f && std::abs(sword.offsetY) < 0.001f) {
                std::cerr << "WARNING: Sword position near origin detected in run loop. Fixing..." << std::endl;
                // Reset sword position
                float baseAngle = glfwGetTime() * 0.4f;
                float distanceFromPlayer = player->radius * 2.5f;
                sword.offsetX = cos(baseAngle) * distanceFromPlayer;
                sword.offsetY = sin(baseAngle) * distanceFromPlayer;
                sword.angle = baseAngle;
            }
        }
        
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

void Game::updateSword() {
    // Debug output for tracking
    static bool lastSwingState = false;
    if (sword.isSwinging != lastSwingState) {
        std::cout << "Sword swing state changed: " << (sword.isSwinging ? "Started" : "Finished") << std::endl;
        lastSwingState = sword.isSwinging;
    }
    
    // Update sword cooldown timer
    if (sword.cooldownTimer > 0) {
        sword.cooldownTimer -= deltaTime;
        if (sword.cooldownTimer < 0) {
            sword.cooldownTimer = 0;
        }
    }
    
    // Base angle for the sword's orbit around player
    float baseAngle = glfwGetTime() * 0.4f; // Even slower orbit around player when idle
    
    // If not swinging, keep sword floating outward from player
    if (!sword.isSwinging) {
        sword.angle = baseAngle;
        // Keep sword at a fixed distance outward from the player (2.5x player radius)
        float distanceFromPlayer = player->radius * 2.5f;
        sword.offsetX = cos(baseAngle) * distanceFromPlayer;
        sword.offsetY = sin(baseAngle) * distanceFromPlayer;
        
        // Debug: Periodically report sword position when idle
        static float lastReportTime = 0.0f;
        float currentTime = static_cast<float>(glfwGetTime());
        if (currentTime - lastReportTime > 3.0f) {
            std::cout << "Idle sword position: " << sword.offsetX << ", " << sword.offsetY << std::endl;
            lastReportTime = currentTime;
        }
    }
    // If swinging, update the swing animation - slow 90-degree rotation followed by local rotation
    else {
        // Use deltaTime to make animation speed consistent across different frame rates
        sword.swingProgress += sword.swingSpeed * deltaTime * 60.0f;
        
        // Debug output to track progress
        if (sword.swingProgress > 0.95f) {
            std::cout << "Swing nearly complete: " << sword.swingProgress << std::endl;
        }
        
        // If swing animation is complete
        if (sword.swingProgress >= 1.0f) {
            // Important: Reset swing state properly
            sword.isSwinging = false;
            sword.swingProgress = 0.0f;
            sword.cooldownTimer = sword.cooldown;
            
            // CRITICAL FIX: Explicitly reset sword position based on current time
            // The issue was likely that we were saving the initial baseAngle at the start
            // but using a new baseAngle when resetting, causing a jump in position
            // Get fresh baseAngle for consistent positioning
            float currentBaseAngle = glfwGetTime() * 0.4f;
            float distanceFromPlayer = player->radius * 2.5f;
            sword.offsetX = cos(currentBaseAngle) * distanceFromPlayer;
            sword.offsetY = sin(currentBaseAngle) * distanceFromPlayer;
            sword.angle = currentBaseAngle;
            
            // Debug: Log when swing completes with position details
            std::cout << "Swing reset complete. New position: " << sword.offsetX << ", " << sword.offsetY << std::endl;
        } else {
            // For the attack animation - make each phase much longer:
            // 1. First 30% of animation: 90-degree rotation to starting position (slower)
            // 2. Middle 40% of animation: Local rotation (360 degrees - just one rotation)
            // 3. Final 30% of animation: Return to original position (slower)
            
            // Store swing start angle on first frame
            static float startAngle = 0.0f;
            if (sword.swingProgress <= sword.swingSpeed) {
                startAngle = sword.angle;
                std::cout << "Swing started with angle: " << startAngle << std::endl;
            }
            
            float currentSwingAngle;
            float distanceFromPlayer;
            
            if (sword.swingProgress < 0.3f) {
                // First phase: slow 90-degree rotation to get into position
                float phase1Progress = sword.swingProgress / 0.3f; // Normalize to 0-1 for this phase
                currentSwingAngle = startAngle + (3.14159f / 2.0f) * phase1Progress;
                // Move slightly closer to player as we prepare to swing
                distanceFromPlayer = player->radius * (2.5f - 0.5f * phase1Progress);
            } else if (sword.swingProgress < 0.7f) {
                // Second phase: Local rotation (just one 360 degree rotation, not two)
                float phase2Progress = (sword.swingProgress - 0.3f) / 0.4f; // Normalize to 0-1 for this phase
                currentSwingAngle = startAngle + (3.14159f / 2.0f) + (3.14159f * 2.0f * phase2Progress);
                // Keep sword close to player during rotation
                distanceFromPlayer = player->radius * 2.0f;
            } else {
                // Third phase: Slow return to original position
                float phase3Progress = (sword.swingProgress - 0.7f) / 0.3f; // Normalize to 0-1 for this phase
                currentSwingAngle = startAngle + (3.14159f / 2.0f) + (3.14159f * 2.0f) - (3.14159f / 2.0f) * phase3Progress;
                // Move back to original distance
                distanceFromPlayer = player->radius * (2.0f + 0.5f * phase3Progress);
            }
            
            // Update sword position based on swing angle
            sword.angle = currentSwingAngle;
            sword.offsetX = cos(sword.angle) * distanceFromPlayer;
            sword.offsetY = sin(sword.angle) * distanceFromPlayer;
            
            // Check for hits on enemies during the middle phase of the swing animation
            if (sword.swingProgress > 0.3f && sword.swingProgress < 0.7f) {
                // Check for hits on each enemy
                for (auto& enemy : enemies) {
                    if (!enemy.isDead) {
                        if (checkSwordHit(enemy.x, enemy.y, enemy.radius)) {
                            enemy.takeDamage(sword.damage);
                        }
                    }
                }
            }
        }
    }
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
    
    // CRITICAL: Always check if our sword has valid data
    // This function should never be skipped - the sword must always be visible
    std::cout << "Rendering sword at: " << player->x + sword.offsetX << ", " << player->y + sword.offsetY << std::endl;
    
    // Always make sure we have a valid VAO before rendering
    if (swordVAO == 0) {
        std::cerr << "ERROR: Sword VAO is 0! Reinitializing..." << std::endl;
        // This shouldn't happen, but we'll reinitialize if needed
        initSword();
        if (swordVAO == 0) {
            std::cerr << "CRITICAL ERROR: Failed to initialize sword VAO!" << std::endl;
            return; // Cannot render without VAO
        }
    }
    
    // Safety check for position - if the sword is somehow at the origin (0,0) fix it
    if (std::abs(sword.offsetX) < 0.0001f && std::abs(sword.offsetY) < 0.0001f) {
        std::cerr << "ERROR: Sword position at origin! Fixing..." << std::endl;
        float baseAngle = glfwGetTime() * 0.4f;
        float distanceFromPlayer = player->radius * 2.5f;
        sword.offsetX = cos(baseAngle) * distanceFromPlayer;
        sword.offsetY = sin(baseAngle) * distanceFromPlayer;
        sword.angle = baseAngle;
    }
    
    // Bind the sword VAO
    glBindVertexArray(swordVAO);
    
    // Set sword color
    if (sword.isSwinging) {
        // During swing, use a brighter color with pulsing based on swing phase
        float brightness;
        if (sword.swingProgress < 0.3f) {
            // First phase - gradual buildup glow
            brightness = 0.8f + 0.3f * (sword.swingProgress / 0.3f);
        } else if (sword.swingProgress < 0.7f) {
            // Second phase - gentle pulsing during rotation
            float phase2Progress = (sword.swingProgress - 0.3f) / 0.4f;
            brightness = 1.1f + 0.2f * sin(phase2Progress * 3.14159f * 3.0f); // Pulse more slowly
        } else {
            // Third phase - cooling down
            float phase3Progress = (sword.swingProgress - 0.7f) / 0.3f;
            brightness = 1.1f - 0.3f * phase3Progress;
        }
        
        shaderProgram->setVec4("uColor", 0.6f * brightness, 0.7f * brightness, 0.9f * brightness, 1.0f);
    } else {
        // Normal sword color - steel blue
        shaderProgram->setVec4("uColor", 0.6f, 0.7f, 0.9f, 1.0f);
    }
    
    // Create model matrix for sword
    glm::mat4 model = glm::mat4(1.0f);
    
    // Position the sword at player position + offset
    model = glm::translate(model, glm::vec3(player->x + sword.offsetX, player->y + sword.offsetY, 0.0f));
    
    // Rotate the sword
    float pointingAngle;
    if (sword.isSwinging && sword.swingProgress >= 0.3f && sword.swingProgress < 0.7f) {
        // During rotation phase, add spin effect to the sword itself - slower rotation
        float phase2Progress = (sword.swingProgress - 0.3f) / 0.4f;
        float spinRotation = phase2Progress * 3.14159f * 2.0f; // One full rotation of the sword itself
        
        // Base angle points from player to sword
        pointingAngle = atan2(sword.offsetY, sword.offsetX) - 3.14159f / 2.0f;
        
        // Add spin rotation
        pointingAngle += spinRotation;
    } else {
        // Normal pointing direction when not in rotation phase
        pointingAngle = atan2(sword.offsetY, sword.offsetX) - 3.14159f / 2.0f;
    }
    
    model = glm::rotate(model, pointingAngle, glm::vec3(0.0f, 0.0f, 1.0f));
    
    // Apply the model matrix
    shaderProgram->setMat4("uModel", glm::value_ptr(model));
    
    // Scale effect during swing
    float scale = 1.0f;
    if (sword.isSwinging) {
        if (sword.swingProgress < 0.3f) {
            // First phase - gradual growth
            scale = 1.0f + 0.1f * (sword.swingProgress / 0.3f);
        } else if (sword.swingProgress < 0.7f) {
            // Second phase - gentle pulsing during rotation
            float phase2Progress = (sword.swingProgress - 0.3f) / 0.4f;
            scale = 1.1f + 0.15f * sin(phase2Progress * 3.14159f * 3.0f); // Slower pulsing
        } else {
            // Third phase - return to normal
            float phase3Progress = (sword.swingProgress - 0.7f) / 0.3f;
            scale = 1.1f - 0.1f * phase3Progress;
        }
    }
    
    shaderProgram->setFloat("uScale", scale);
    
    // Draw the sword parts
    
    // Main blade triangle
    shaderProgram->setVec4("uColor", 0.7f, 0.8f, 0.95f, 1.0f); // Lighter blade color
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    // Blade detail/fuller
    shaderProgram->setVec4("uColor", 0.5f, 0.6f, 0.8f, 1.0f); // Darker center for depth
    glDrawArrays(GL_TRIANGLES, 3, 3);
    
    // Guard - gold color
    shaderProgram->setVec4("uColor", 0.9f, 0.7f, 0.2f, 1.0f); // Gold color
    glDrawArrays(GL_TRIANGLE_FAN, 6, 4);
    
    // Handle - brown color
    shaderProgram->setVec4("uColor", 0.6f, 0.3f, 0.1f, 1.0f); // Brown wood color
    glDrawArrays(GL_TRIANGLE_FAN, 10, 4);
    
    // Pommel - gold to match guard
    shaderProgram->setVec4("uColor", 0.9f, 0.7f, 0.2f, 1.0f); // Gold color
    glDrawArrays(GL_TRIANGLE_FAN, 14, 10); // Draw pommel as a triangle fan
    
    // Reset scale
    shaderProgram->setFloat("uScale", 1.0f);
    
    // Add trail effects only during the rotation phase, and make them less intense
    if (sword.isSwinging && sword.swingProgress >= 0.3f && sword.swingProgress < 0.7f) {
        // Create fewer trail segments for a less cluttered look
        int numTrails = 3; // Reduced from 5
        for (int i = 1; i <= numTrails; i++) {
            float trailOffset = i * 0.05f; // Increased spacing between trails
            float prevProgress = (sword.swingProgress - 0.3f) / 0.4f - trailOffset;
            
            // Skip trails that would be before the start of phase 2
            if (prevProgress < 0) continue;
            
            // Calculate previous position and angle
            float prevRotation = prevProgress * 3.14159f * 2.0f; // One full rotation
            float prevAngle = sword.angle - trailOffset * 3.14159f * 2.0f;
            float prevDistance = player->radius * 2.0f; // During phase 2, distance is constant
            float prevX = player->x + cos(prevAngle) * prevDistance;
            float prevY = player->y + sin(prevAngle) * prevDistance;
            
            // Calculate previous sword rotation angle
            float prevPointingAngle = atan2(
                prevY - player->y, 
                prevX - player->x
            ) - 3.14159f / 2.0f + prevRotation;
            
            // For each trail segment, draw a simpler, more transparent trail
            float alpha = 0.08f * (1.0f - (static_cast<float>(i) / numTrails)); // Lower alpha for subtlety
            
            std::vector<float> trailVertices = {
                // Simplified trail - just the tip and edges of the blade
                prevX, prevY,  // Center point
                prevX + cos(prevPointingAngle) * sword.length * 0.6f * scale,
                prevY + sin(prevPointingAngle) * sword.length * 0.6f * scale,
                prevX + cos(prevPointingAngle + 0.3f) * sword.width * scale * 0.4f,
                prevY + sin(prevPointingAngle + 0.3f) * sword.width * scale * 0.4f
            };
            
            // Create and bind temporary VBO for trail
            GLuint trailVBO;
            glGenBuffers(1, &trailVBO);
            glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
            glBufferData(GL_ARRAY_BUFFER, trailVertices.size() * sizeof(float), trailVertices.data(), GL_STATIC_DRAW);
            
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            
            // Set trail color - very subtle blue
            shaderProgram->setVec4("uColor", 0.6f, 0.7f, 1.0f, alpha);
            
            // Enable blending for transparency
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            
            // Draw trail as a triangle fan
            glDrawArrays(GL_TRIANGLE_FAN, 0, 3);
            
            // Clean up
            glDeleteBuffers(1, &trailVBO);
        }
        
        glDisable(GL_BLEND);
    }
    
    glBindVertexArray(0);
}
