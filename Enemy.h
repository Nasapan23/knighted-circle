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
    
    // Enhanced AI behavior
    float detectionRange;      // Range to detect and start following player
    float shootingRange;       // Range to start shooting at player
    float meleeRange;          // Range for melee attacks
    float wanderRadius;        // Radius within which enemy wanders
    float homeX, homeY;        // Home position for wandering
    
    // Smart movement and collision
    float lastPlayerX, lastPlayerY;  // Track player's last known position
    float playerPredictionTime;      // How far ahead to predict player movement
    float collisionRadius;           // Radius for collision detection
    float pushForce;                 // Force applied during collisions
    
    // Enhanced combat
    float meleeTimer;
    float meleeCooldown;
    bool canMelee;
    int meleeDamage;
    
    // Wandering behavior
    float wanderTimer;
    float wanderInterval;
    float targetX, targetY;
    
    // Shooting behavior
    float shootingTimer;
    float shootingCooldown;
    bool canShoot;
    std::vector<Arrow> arrows;
    
    // AI states
    enum AIState {
        WANDERING,
        DETECTING,
        FOLLOWING,
        ATTACKING,
        FLEEING
    };
    AIState currentState;
    float stateTimer;

    Enemy(float startX, float startY, float rad, float spd);
    void update(float playerX, float playerY, float deltaTime);
    void takeDamage(int amount);
    float getHealthPercentage() const;
    void shoot(float playerX, float playerY);
    void updateArrows(float deltaTime);
    bool checkArrowHit(float targetX, float targetY, float targetRadius, int& damage);
    bool checkMeleeHit(float targetX, float targetY, float targetRadius, int& damage);
    
    // Enhanced collision system
    void handleCollision(float otherX, float otherY, float otherRadius, float deltaTime);
    bool isColliding(float otherX, float otherY, float otherRadius) const;
    
private:
    void wander(float deltaTime);
    void followPlayer(float playerX, float playerY, float deltaTime);
    void attackPlayer(float playerX, float playerY, float deltaTime);
    void fleeFromPlayer(float playerX, float playerY, float deltaTime);
    void updateWanderTarget();
    void updateAIState(float playerX, float playerY, float distToPlayer);
    
    // Smart movement functions
    void moveTowards(float targetX, float targetY, float moveSpeed, float deltaTime);
    void predictPlayerMovement(float playerX, float playerY, float& predX, float& predY);
    bool hasLineOfSight(float targetX, float targetY) const;
};

#endif
