#ifndef PLAYER_H
#define PLAYER_H

#include "dependente/glfw/glfw3.h"

class Player {
public:
    float x, y;
    float radius;
    float speed;
    int maxHealth;
    int currentHealth;
    bool isInvulnerable;
    float invulnerabilityTimer;
    float invulnerabilityDuration;
    bool isDead;
    float timeOfDeath;
    
    // Collision properties
    float collisionRadius;
    float pushForce;

    // Constructor: starting position, radius, and speed.
    Player(float startX, float startY, float rad, float spd);
    void update(GLFWwindow* window, float deltaTime);
    void takeDamage(int amount);
    void heal(int amount);
    float getHealthPercentage() const;
    void die();
    
    // Collision handling
    void handleCollision(float otherX, float otherY, float otherRadius, float deltaTime);
    bool isColliding(float otherX, float otherY, float otherRadius) const;
};

#endif
