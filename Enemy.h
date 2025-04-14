#ifndef ENEMY_H
#define ENEMY_H

#include <vector>

struct Arrow {
    float x, y;
    float vx, vy;
    float radius;
    bool active;
};

class Enemy {
public:
    float x, y;
    float radius;
    float speed;
    int maxHealth;
    int currentHealth;
    bool isDead;
    
    // AI behavior
    float detectionRange;  // Range to detect and start following player
    float shootingRange;   // Range to start shooting at player
    float wanderRadius;    // Radius within which enemy wanders
    float homeX, homeY;    // Home position for wandering
    
    // Wandering behavior
    float wanderTimer;
    float wanderInterval;
    float targetX, targetY;
    
    // Shooting behavior
    float shootingTimer;
    float shootingCooldown;
    bool canShoot;
    std::vector<Arrow> arrows;

    Enemy(float startX, float startY, float rad, float spd);
    void update(float playerX, float playerY, float deltaTime);
    void takeDamage(int amount);
    float getHealthPercentage() const;
    void shoot(float playerX, float playerY);
    void updateArrows(float deltaTime);
    bool checkArrowHit(float targetX, float targetY, float targetRadius, int& damage);
    
private:
    void wander(float deltaTime);
    void followPlayer(float playerX, float playerY, float deltaTime);
    void updateWanderTarget();
};

#endif
