#include "Player.h"

Player::Player(float startX, float startY, float rad, float spd)
    : x(startX), y(startY), radius(rad), speed(spd) {
}

void Player::update(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        y += speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        y -= speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        x -= speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        x += speed;
}
