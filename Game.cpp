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
    shaderProgram(nullptr), circleVAO(0), circleVBO(0),
    segments(50), baseRadius(0.05f),
    enemySpeed(0.005f), arrowActive(false), mouseWasPressed(false),
    arrowSpeed(0.02f)
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
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    if (!mode) {
        std::cerr << "Failed to get video mode" << std::endl;
        glfwTerminate();
        return false;
    }
    screenWidth = mode->width;
    screenHeight = mode->height;
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

    // Set up an orthographic projection.
    float aspect = static_cast<float>(screenWidth) / screenHeight;
    // World coordinates: X from -aspect to aspect, Y from -1 to 1.
    projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);

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
    player = new Player(0.0f, 0.0f, baseRadius, 0.005f);

    // Create enemies.
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

    arrowActive = false;
    mouseWasPressed = false;

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

    // Resolve enemy-enemy collisions.
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

    // Check arrow-enemy collisions.
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

    // Boundary clamping for player.
    float aspect = static_cast<float>(screenWidth) / screenHeight;
    if (player->x < -aspect + player->radius) player->x = -aspect + player->radius;
    if (player->x > aspect - player->radius) player->x = aspect - player->radius;
    if (player->y < -1.0f + player->radius) player->y = -1.0f + player->radius;
    if (player->y > 1.0f - player->radius) player->y = 1.0f - player->radius;
    // Boundary clamping for enemies.
    for (auto& enemy : enemies) {
        if (enemy.x < -aspect + enemy.radius) enemy.x = -aspect + enemy.radius;
        if (enemy.x > aspect - enemy.radius) enemy.x = aspect - enemy.radius;
        if (enemy.y < -1.0f + enemy.radius) enemy.y = -1.0f + enemy.radius;
        if (enemy.y > 1.0f - enemy.radius) enemy.y = 1.0f - enemy.radius;
    }
}

void Game::render() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    shaderProgram->use();
    shaderProgram->setMat4("uProjection", glm::value_ptr(projection));

    // Draw player (green circle)
    shaderProgram->setVec4("uColor", 0.2f, 0.7f, 0.3f, 1.0f);
    shaderProgram->setFloat("uScale", 1.0f);
    shaderProgram->setVec2("uOffset", player->x, player->y);
    glBindVertexArray(circleVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, segments + 2);
    glBindVertexArray(0);

    // Draw enemies (red circles)
    shaderProgram->setVec4("uColor", 1.0f, 0.0f, 0.0f, 1.0f);
    for (const auto& enemy : enemies) {
        shaderProgram->setFloat("uScale", enemy.radius / baseRadius);
        shaderProgram->setVec2("uOffset", enemy.x, enemy.y);
        glBindVertexArray(circleVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, segments + 2);
    }
    glBindVertexArray(0);

    // Draw arrow (yellow circle)
    if (arrowActive) {
        shaderProgram->setVec4("uColor", 1.0f, 1.0f, 0.0f, 1.0f);
        shaderProgram->setFloat("uScale", arrow.radius / baseRadius);
        shaderProgram->setVec2("uOffset", arrow.x, arrow.y);
        glBindVertexArray(circleVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, segments + 2);
    }
    glBindVertexArray(0);

    glfwSwapBuffers(window);
}

void Game::cleanup() {
    glDeleteVertexArrays(1, &circleVAO);
    glDeleteBuffers(1, &circleVBO);
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
