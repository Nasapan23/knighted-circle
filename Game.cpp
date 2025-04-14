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
    segments(50), baseRadius(0.05f),
    enemySpeed(0.005f), arrowActive(false), mouseWasPressed(false),
    arrowSpeed(0.02f), damageTimer(0.0f), damageCooldown(3.0f),
    deathScreenTimeout(3.0f)
{
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

    // Create the player. (Make player slower by reducing speed from 0.01f to, say, 0.005f)
    player = new Player(0.0f, 0.0f, baseRadius, 0.001f);

    // Create enemies.
    /*
    const int numEnemies = 10;
    for (int i = 0; i < numEnemies; ++i) {
        float x, y;
        int edge = rand() % 4;
        float worldAspect = aspect;
        switch (edge) {
        case 0: // left
            x = -worldAspect;
            y = randomFloat(-1.0f, 1.0f);
            break;
        case 1: // right
            x = worldAspect;
            y = randomFloat(-1.0f, 1.0f);
            break;
        case 2: // top
            x = randomFloat(-worldAspect, worldAspect);
            y = 1.0f;
            break;
        case 3: // bottom
            x = randomFloat(-worldAspect, worldAspect);
            y = -1.0f;
            break;
        }
        // Create enemy with same baseRadius and enemySpeed.
        enemies.push_back(Enemy(x, y, baseRadius, enemySpeed));
    }
    */

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

    return true;
}

void Game::processInput() {
    // Check ESC key.
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    // Update player input.
    player->update(window);

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
}

void Game::update() {
    processInput();

    // For testing purposes, damage player every few seconds (T key)
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
        damageTimer += 0.016f; // Assuming ~60 FPS
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

    // Update arrow.
    if (arrowActive) {
        arrow.x += arrow.vx;
        arrow.y += arrow.vy;
        float aspect = static_cast<float>(screenWidth) / screenHeight;
        if (arrow.x < -aspect + arrow.radius || arrow.x > aspect - arrow.radius ||
            arrow.y < -1.0f + arrow.radius || arrow.y > 1.0f - arrow.radius)
            arrowActive = false;
    }

    // Update enemies.
    /*
    for (auto& enemy : enemies) {
        enemy.update(player->x, player->y);
        // Resolve collision with player.
        float dx = enemy.x - player->x;
        float dy = enemy.y - player->y;
        float d = std::sqrt(dx * dx + dy * dy);
        float minDist = enemy.radius + player->radius;
        if (d < minDist && d > 0) {
            float nx = dx / d;
            float ny = dy / d;
            enemy.x = player->x + nx * minDist;
            enemy.y = player->y + ny * minDist;
        }
    }
    */

    // Resolve enemy-enemy collisions.
    /*
    for (size_t i = 0; i < enemies.size(); i++) {
        for (size_t j = i + 1; j < enemies.size(); j++) {
            float dx = enemies[i].x - enemies[j].x;
            float dy = enemies[i].y - enemies[j].y;
            float d = std::sqrt(dx * dx + dy * dy);
            float minDist = enemies[i].radius + enemies[j].radius;
            if (d < minDist && d > 0) {
                float overlap = (minDist - d) / 2.0f;
                float nx = dx / d;
                float ny = dy / d;
                enemies[i].x += nx * overlap;
                enemies[i].y += ny * overlap;
                enemies[j].x -= nx * overlap;
                enemies[j].y -= ny * overlap;
            }
        }
    }
    */

    // Check arrow-enemy collisions.
    /*
    if (arrowActive) {
        for (size_t i = 0; i < enemies.size(); i++) {
            float dx = arrow.x - enemies[i].x;
            float dy = arrow.y - enemies[i].y;
            float d = std::sqrt(dx * dx + dy * dy);
            if (d < arrow.radius + enemies[i].radius) {
                enemies.erase(enemies.begin() + i);
                arrowActive = false;
                break;
            }
        }
    }
    */

    // Boundary clamping for player.
    float aspect = static_cast<float>(screenWidth) / screenHeight;
    if (player->x < -aspect + player->radius) player->x = -aspect + player->radius;
    if (player->x > aspect - player->radius) player->x = aspect - player->radius;
    if (player->y < -1.0f + player->radius) player->y = -1.0f + player->radius;
    if (player->y > 1.0f - player->radius) player->y = 1.0f - player->radius;
    // Boundary clamping for enemies.
    /*
    for (auto& enemy : enemies) {
        if (enemy.x < -aspect + enemy.radius) enemy.x = -aspect + enemy.radius;
        if (enemy.x > aspect - enemy.radius) enemy.x = aspect - enemy.radius;
        if (enemy.y < -1.0f + enemy.radius) enemy.y = -1.0f + enemy.radius;
        if (enemy.y > 1.0f - enemy.radius) enemy.y = 1.0f - enemy.radius;
    }
    */
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
    shaderProgram->setVec4("uColor", 0.9f, 0.2f, 0.2f, 1.0f);
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(barPosX, barPosY, 0.0f));
    model = glm::scale(model, glm::vec3(barWidth * healthPercentage, barHeight, 1.0f));
    shaderProgram->setMat4("uModel", glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
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

    // Set up for player and arrow rendering (old method)
    glm::mat4 identityModel = glm::mat4(1.0f);
    shaderProgram->setMat4("uModel", glm::value_ptr(identityModel));
    
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
    
    shaderProgram->setFloat("uScale", 1.0f);
    shaderProgram->setVec2("uOffset", player->x, player->y);
    glBindVertexArray(circleVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, segments + 2);
    glBindVertexArray(0);

    // Draw enemies (red circles)
    /*
    shaderProgram->setVec4("uColor", 1.0f, 0.0f, 0.0f, 1.0f);
    for (const auto& enemy : enemies) {
        shaderProgram->setFloat("uScale", enemy.radius / baseRadius);
        shaderProgram->setVec2("uOffset", enemy.x, enemy.y);
        glBindVertexArray(circleVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, segments + 2);
    }
    glBindVertexArray(0);
    */

    // Draw arrow (yellow circle)
    if (arrowActive && !player->isDead) {  // Don't show arrow if player is dead
        shaderProgram->setVec4("uColor", 1.0f, 1.0f, 0.0f, 1.0f);
        shaderProgram->setFloat("uScale", arrow.radius / baseRadius);
        shaderProgram->setVec2("uOffset", arrow.x, arrow.y);
        glBindVertexArray(circleVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, segments + 2);
    }
    glBindVertexArray(0);

    // Reset offset for health bar and death screen
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
    
    glDeleteVertexArrays(1, &circleVAO);
    glDeleteBuffers(1, &circleVBO);
    glDeleteVertexArrays(1, &rectVAO);
    glDeleteBuffers(1, &rectVBO);
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Game::run() {
    while (!glfwWindowShouldClose(window)) {
        processInput();
        update();
        render();
        glfwPollEvents();
    }
}
