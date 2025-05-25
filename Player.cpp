#include "Player.h"
#include <cmath>

Player::Player(float startX, float startY, float rad, float spd)
    : x(startX), y(startY), radius(rad), speed(spd),
      maxHealth(100), currentHealth(100), isInvulnerable(false),
      invulnerabilityTimer(0.0f), invulnerabilityDuration(1.0f),
      isDead(false), timeOfDeath(0.0f),
      collisionRadius(rad * 1.1f), pushForce(0.015f) {
}

void Player::update(GLFWwindow* window, float deltaTime) {
    // Don't process input if player is dead
    if (isDead) {
        return;
    }
    
    // Calculate movement based on deltaTime for consistent speed
    float adjustedSpeed = speed * deltaTime * 60.0f; // Scale for 60fps equivalent
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        y += adjustedSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        y -= adjustedSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        x -= adjustedSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        x += adjustedSpeed;
        
    // Update invulnerability timer
    if (isInvulnerable) {
        invulnerabilityTimer -= deltaTime; // Use deltaTime instead of fixed value
        if (invulnerabilityTimer <= 0) {
            isInvulnerable = false;
            invulnerabilityTimer = 0;
        }
    }
}

void Player::takeDamage(int amount) {
    if (isDead) return; // Already dead, can't take more damage
    
    if (!isInvulnerable && amount > 0) {
        currentHealth -= amount;
        if (currentHealth <= 0) {
            currentHealth = 0;
            die();
            return;
        }
        
        // Start invulnerability period
        isInvulnerable = true;
        invulnerabilityTimer = invulnerabilityDuration;
    }
}

void Player::heal(int amount) {
    if (isDead) return; // Dead players can't be healed
    
    if (amount > 0) {
        currentHealth += amount;
        if (currentHealth > maxHealth) {
            currentHealth = maxHealth;
        }
    }
}

float Player::getHealthPercentage() const {
    return static_cast<float>(currentHealth) / maxHealth;
}

void Player::die() {
    isDead = true;
    currentHealth = 0;
    timeOfDeath = static_cast<float>(glfwGetTime()); // Record time of death for the death timer
}

void Player::handleCollision(float otherX, float otherY, float otherRadius, float deltaTime) {
    if (isDead) return; // Dead players don't collide
    
    if (isColliding(otherX, otherY, otherRadius)) {
        // Calculate push direction (away from other object)
        float dx = x - otherX;
        float dy = y - otherY;
        float dist = std::sqrt(dx * dx + dy * dy);
        
        if (dist > 0.001f) {
            // Normalize direction
            dx /= dist;
            dy /= dist;
            
            // Calculate overlap
            float overlap = (collisionRadius + otherRadius) - dist;
            
            // Push player away
            float pushDistance = overlap * 0.5f + pushForce;
            x += dx * pushDistance;
            y += dy * pushDistance;
        }
    }
}

bool Player::isColliding(float otherX, float otherY, float otherRadius) const {
    if (isDead) return false; // Dead players don't collide
    
    float dx = x - otherX;
    float dy = y - otherY;
    float dist = std::sqrt(dx * dx + dy * dy);
    return dist < (collisionRadius + otherRadius);
}
