#include "Enemy.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

// Helper function to get random float between min and max
float randomFloatRange(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

Enemy::Enemy(float startX, float startY, float rad, float spd)
    : x(startX), y(startY), radius(rad), speed(spd), 
      maxHealth(75), currentHealth(75), isDead(false),
      detectionRange(0.8f), shootingRange(0.5f), meleeRange(0.15f), wanderRadius(0.4f),
      homeX(startX), homeY(startY), 
      lastPlayerX(0), lastPlayerY(0), playerPredictionTime(0.3f),
      collisionRadius(rad * 1.2f), pushForce(0.02f),
      meleeTimer(0.0f), meleeCooldown(1.5f), canMelee(true), meleeDamage(15),
      wanderTimer(0.0f), wanderInterval(2.0f), targetX(startX), targetY(startY),
      shootingTimer(0.0f), shootingCooldown(1.8f), canShoot(true),
      currentState(WANDERING), stateTimer(0.0f) {
    
    // Initialize with a random wander target
    updateWanderTarget();
}

void Enemy::update(float playerX, float playerY, float deltaTime) {
    if (isDead) return;

    // Calculate distance to player
    float dx = playerX - x;
    float dy = playerY - y;
    float distToPlayer = std::sqrt(dx * dx + dy * dy);

    // Update timers
    if (!canShoot) {
        shootingTimer += deltaTime;
        if (shootingTimer >= shootingCooldown) {
            shootingTimer = 0.0f;
            canShoot = true;
        }
    }
    
    if (!canMelee) {
        meleeTimer += deltaTime;
        if (meleeTimer >= meleeCooldown) {
            meleeTimer = 0.0f;
            canMelee = true;
        }
    }
    
    stateTimer += deltaTime;

    // Update AI state based on player distance and health
    updateAIState(playerX, playerY, distToPlayer);

    // Execute behavior based on current state
    switch (currentState) {
        case WANDERING:
            wander(deltaTime);
            break;
        case DETECTING:
        case FOLLOWING:
            followPlayer(playerX, playerY, deltaTime);
            break;
        case ATTACKING:
            attackPlayer(playerX, playerY, deltaTime);
            break;
        case FLEEING:
            fleeFromPlayer(playerX, playerY, deltaTime);
            break;
    }

    // Update arrows
    updateArrows(deltaTime);
    
    // Store player position for prediction
    lastPlayerX = playerX;
    lastPlayerY = playerY;
}

void Enemy::updateAIState(float playerX, float playerY, float distToPlayer) {
    AIState newState = currentState;
    
    // Health-based behavior
    float healthPercentage = getHealthPercentage();
    
    if (healthPercentage < 0.3f && distToPlayer < shootingRange) {
        // Low health and player is close - flee
        newState = FLEEING;
    }
    else if (distToPlayer <= meleeRange) {
        // Very close - attack with melee
        newState = ATTACKING;
    }
    else if (distToPlayer <= shootingRange) {
        // In shooting range - attack with ranged
        newState = ATTACKING;
    }
    else if (distToPlayer <= detectionRange) {
        // Player detected - follow
        if (hasLineOfSight(playerX, playerY)) {
            newState = FOLLOWING;
        } else {
            newState = DETECTING; // Lost line of sight, but still aware
        }
    }
    else {
        // Player not detected - wander
        newState = WANDERING;
    }
    
    // Reset state timer if state changed
    if (newState != currentState) {
        stateTimer = 0.0f;
        currentState = newState;
    }
}

void Enemy::wander(float deltaTime) {
    wanderTimer += deltaTime;
    
    // Pick a new target after interval expires or if we reached current target
    if (wanderTimer >= wanderInterval) {
        updateWanderTarget();
        wanderTimer = 0.0f;
    }
    
    // Move toward the current wander target
    float dx = targetX - x;
    float dy = targetY - y;
    float dist = std::sqrt(dx * dx + dy * dy);
    
    // If we're close enough to target, pick a new one
    if (dist < radius * 0.5f) {
        updateWanderTarget();
        return;
    }
    
    // Move toward target at reduced speed
    moveTowards(targetX, targetY, speed * 0.4f, deltaTime);
}

void Enemy::followPlayer(float playerX, float playerY, float deltaTime) {
    // Predict where the player will be
    float predX, predY;
    predictPlayerMovement(playerX, playerY, predX, predY);
    
    // Move toward predicted position
    float followSpeed = speed * 0.8f;
    
    // If we lost line of sight, move to last known position
    if (currentState == DETECTING) {
        followSpeed *= 0.6f; // Move slower when searching
        moveTowards(lastPlayerX, lastPlayerY, followSpeed, deltaTime);
    } else {
        moveTowards(predX, predY, followSpeed, deltaTime);
    }
}

void Enemy::attackPlayer(float playerX, float playerY, float deltaTime) {
    float dx = playerX - x;
    float dy = playerY - y;
    float distToPlayer = std::sqrt(dx * dx + dy * dy);
    
    // Melee attack if very close
    if (distToPlayer <= meleeRange && canMelee) {
        // Don't move, just attack
        canMelee = false;
        meleeTimer = 0.0f;
    }
    // Ranged attack if in shooting range
    else if (distToPlayer <= shootingRange && canShoot) {
        // Predict player movement for better accuracy
        float predX, predY;
        predictPlayerMovement(playerX, playerY, predX, predY);
        shoot(predX, predY);
        canShoot = false;
        shootingTimer = 0.0f;
        
        // Move to maintain optimal distance
        float optimalDistance = shootingRange * 0.7f;
        if (distToPlayer < optimalDistance) {
            // Too close, back away slightly
            moveTowards(x - dx * 0.1f, y - dy * 0.1f, speed * 0.3f, deltaTime);
        } else if (distToPlayer > shootingRange * 0.9f) {
            // Too far, move closer
            moveTowards(playerX, playerY, speed * 0.6f, deltaTime);
        }
    }
    else {
        // Move to get into range
        moveTowards(playerX, playerY, speed * 0.9f, deltaTime);
    }
}

void Enemy::fleeFromPlayer(float playerX, float playerY, float deltaTime) {
    // Calculate direction away from player
    float dx = x - playerX;
    float dy = y - playerY;
    float dist = std::sqrt(dx * dx + dy * dy);
    
    if (dist > 0) {
        dx /= dist;
        dy /= dist;
    }
    
    // Flee toward home position if possible, otherwise just away from player
    float fleeX = x + dx * 0.5f;
    float fleeY = y + dy * 0.5f;
    
    // Bias toward home position
    float homeWeight = 0.3f;
    fleeX = fleeX * (1.0f - homeWeight) + homeX * homeWeight;
    fleeY = fleeY * (1.0f - homeWeight) + homeY * homeWeight;
    
    moveTowards(fleeX, fleeY, speed * 1.2f, deltaTime); // Flee faster
}

void Enemy::moveTowards(float targetX, float targetY, float moveSpeed, float deltaTime) {
    float dx = targetX - x;
    float dy = targetY - y;
    float dist = std::sqrt(dx * dx + dy * dy);
    
    if (dist > 0.001f) {
        dx /= dist;
        dy /= dist;
        
        x += dx * moveSpeed * deltaTime * 60.0f;
        y += dy * moveSpeed * deltaTime * 60.0f;
    }
}

void Enemy::predictPlayerMovement(float playerX, float playerY, float& predX, float& predY) {
    // Calculate player velocity based on position change
    float playerVelX = (playerX - lastPlayerX) / (1.0f/60.0f); // Assume 60fps
    float playerVelY = (playerY - lastPlayerY) / (1.0f/60.0f);
    
    // Predict where player will be
    predX = playerX + playerVelX * playerPredictionTime;
    predY = playerY + playerVelY * playerPredictionTime;
}

bool Enemy::hasLineOfSight(float targetX, float targetY) const {
    // Simple line of sight check - in a real game you'd check for obstacles
    // For now, just return true if within detection range
    float dx = targetX - x;
    float dy = targetY - y;
    float dist = std::sqrt(dx * dx + dy * dy);
    return dist <= detectionRange;
}

void Enemy::handleCollision(float otherX, float otherY, float otherRadius, float deltaTime) {
    if (isColliding(otherX, otherY, otherRadius)) {
        // Calculate push direction
        float dx = x - otherX;
        float dy = y - otherY;
        float dist = std::sqrt(dx * dx + dy * dy);
        
        if (dist > 0.001f) {
            // Normalize direction
            dx /= dist;
            dy /= dist;
            
            // Calculate overlap
            float overlap = (radius + otherRadius) - dist;
            
            // Push apart
            float pushDistance = overlap * 0.5f + pushForce;
            x += dx * pushDistance;
            y += dy * pushDistance;
        }
    }
}

bool Enemy::isColliding(float otherX, float otherY, float otherRadius) const {
    float dx = x - otherX;
    float dy = y - otherY;
    float dist = std::sqrt(dx * dx + dy * dy);
    return dist < (collisionRadius + otherRadius);
}

bool Enemy::checkMeleeHit(float targetX, float targetY, float targetRadius, int& damage) {
    if (!canMelee) return false;
    
    float dx = x - targetX;
    float dy = y - targetY;
    float dist = std::sqrt(dx * dx + dy * dy);
    
    if (dist <= meleeRange + targetRadius) {
        damage = meleeDamage;
        return true;
    }
    return false;
}

void Enemy::updateWanderTarget() {
    // Pick a random point within wanderRadius of home position
    float angle = randomFloatRange(0, 2 * 3.14159f);
    float distance = randomFloatRange(wanderRadius * 0.3f, wanderRadius);
    
    targetX = homeX + std::cos(angle) * distance;
    targetY = homeY + std::sin(angle) * distance;
}

void Enemy::takeDamage(int amount) {
    if (isDead) return;
    
    if (amount > 0) {
        currentHealth -= amount;
        if (currentHealth <= 0) {
            currentHealth = 0;
            isDead = true;
        }
    }
}

float Enemy::getHealthPercentage() const {
    return static_cast<float>(currentHealth) / maxHealth;
}

void Enemy::shoot(float playerX, float playerY) {
    // Create an arrow pointing at the predicted player position
    Arrow arrow;
    arrow.x = x;
    arrow.y = y;
    
    // Calculate direction to target
    float dx = playerX - x;
    float dy = playerY - y;
    float dist = std::sqrt(dx * dx + dy * dy);
    
    // Normalize and set velocity
    if (dist > 0) {
        dx /= dist;
        dy /= dist;
    }
    
    // Improved arrow speed and accuracy
    float arrowSpeed = 0.015f;
    arrow.vx = arrowSpeed * dx;
    arrow.vy = arrowSpeed * dy;
    
    arrow.radius = radius * 0.15f;
    arrow.active = true;
    
    arrows.push_back(arrow);
}

void Enemy::updateArrows(float deltaTime) {
    // Keep track of which arrows to remove
    std::vector<size_t> arrowsToRemove;
    
    // Update all arrows
    for (size_t i = 0; i < arrows.size(); i++) {
        auto& arrow = arrows[i];
        if (arrow.active) {
            // Move arrow
            arrow.x += arrow.vx * deltaTime * 60.0f;
            arrow.y += arrow.vy * deltaTime * 60.0f;
            
            // Check if arrow is out of bounds
            if (arrow.x < -3.0f || arrow.x > 3.0f || arrow.y < -2.0f || arrow.y > 2.0f) {
                arrow.active = false;
                arrowsToRemove.push_back(i);
            }
        } else {
            arrowsToRemove.push_back(i);
        }
    }
    
    // Remove inactive arrows
    for (int i = arrowsToRemove.size() - 1; i >= 0; i--) {
        size_t index = arrowsToRemove[i];
        if (index < arrows.size()) {
            arrows.erase(arrows.begin() + index);
        }
    }
    
    // Cap maximum arrows
    if (arrows.size() > 8) {
        arrows.erase(arrows.begin(), arrows.begin() + (arrows.size() - 8));
    }
}

bool Enemy::checkArrowHit(float targetX, float targetY, float targetRadius, int& damage) {
    // Check each arrow against the target
    for (auto& arrow : arrows) {
        if (arrow.active) {
            float dx = arrow.x - targetX;
            float dy = arrow.y - targetY;
            float dist = std::sqrt(dx * dx + dy * dy);
            
            // Check collision
            if (dist < arrow.radius + targetRadius) {
                arrow.active = false;
                damage = 8; // Increased arrow damage
                return true;
            }
        }
    }
    return false;
}
