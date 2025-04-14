#include "Enemy.h"
#include <cmath>
#include <cstdlib>

// Helper function to get random float between min and max
float randomFloatRange(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

Enemy::Enemy(float startX, float startY, float rad, float spd)
    : x(startX), y(startY), radius(rad), speed(spd), 
      maxHealth(50), currentHealth(50), isDead(false),
      detectionRange(0.4f), shootingRange(0.3f), wanderRadius(0.3f),
      homeX(startX), homeY(startY), wanderTimer(0.0f), wanderInterval(3.0f),
      targetX(startX), targetY(startY), shootingTimer(0.0f),
      shootingCooldown(2.0f), canShoot(true) {
    
    // Initialize with a random wander target
    updateWanderTarget();
}

void Enemy::update(float playerX, float playerY, float deltaTime) {
    if (isDead) return;

    // Update shooting timer
    if (!canShoot) {
        shootingTimer += deltaTime;
        if (shootingTimer >= shootingCooldown) {
            shootingTimer = 0.0f;
            canShoot = true;
        }
    }

    // Calculate distance to player
    float dx = playerX - x;
    float dy = playerY - y;
    float distToPlayer = std::sqrt(dx * dx + dy * dy);

    // Update arrows
    updateArrows(deltaTime);

    // Player is in shooting range - shoot and follow
    if (distToPlayer <= shootingRange) {
        if (canShoot) {
            shoot(playerX, playerY);
            canShoot = false;
        }
        followPlayer(playerX, playerY, deltaTime);
    }
    // Player is in detection range - follow but don't shoot
    else if (distToPlayer <= detectionRange) {
        followPlayer(playerX, playerY, deltaTime);
    }
    // Player not in range - wander around home position
    else {
        wander(deltaTime);
    }
}

void Enemy::wander(float deltaTime) {
    // Update wander timer
    wanderTimer += deltaTime;
    
    // Pick a new target after interval expires
    if (wanderTimer >= wanderInterval) {
        updateWanderTarget();
        wanderTimer = 0.0f;
    }
    
    // Move toward the current wander target
    float dx = targetX - x;
    float dy = targetY - y;
    float dist = std::sqrt(dx * dx + dy * dy);
    
    // If we're close enough to target, pick a new one
    if (dist < radius) {
        updateWanderTarget();
        return;
    }
    
    // Move toward target
    if (dist > 0) {
        dx /= dist;
        dy /= dist;
    }
    
    // Move at half speed when wandering
    x += (speed * 0.5f) * dx * deltaTime * 60.0f; // Scale by deltaTime and 60fps
    y += (speed * 0.5f) * dy * deltaTime * 60.0f;
}

void Enemy::followPlayer(float playerX, float playerY, float deltaTime) {
    float dx = playerX - x;
    float dy = playerY - y;
    float dist = std::sqrt(dx * dx + dy * dy);
    
    // Normalize direction
    if (dist > 0) {
        dx /= dist;
        dy /= dist;
    }
    
    // Move toward player at normal speed
    x += speed * dx * deltaTime * 60.0f; // Scale by deltaTime and 60fps
    y += speed * dy * deltaTime * 60.0f;
}

void Enemy::updateWanderTarget() {
    // Pick a random point within wanderRadius of home position
    float angle = randomFloatRange(0, 2 * 3.14159f);
    float distance = randomFloatRange(0, wanderRadius);
    
    targetX = homeX + std::cos(angle) * distance;
    targetY = homeY + std::sin(angle) * distance;
}

void Enemy::takeDamage(int amount) {
    if (isDead) return; // Already dead, can't take more damage
    
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
    // Create an arrow pointing at the player
    Arrow arrow;
    arrow.x = x;
    arrow.y = y;
    
    // Calculate direction to player
    float dx = playerX - x;
    float dy = playerY - y;
    float dist = std::sqrt(dx * dx + dy * dy);
    
    // Normalize and set velocity (half the player's arrow speed)
    if (dist > 0) {
        dx /= dist;
        dy /= dist;
    }
    
    // Arrow speed is half of player's (assuming arrowSpeed = 0.02f in Game)
    float arrowSpeed = 0.01f;
    arrow.vx = arrowSpeed * dx;
    arrow.vy = arrowSpeed * dy;
    
    // Arrow size is same as player's
    arrow.radius = radius * 0.2f;
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
            
            // Check if arrow is out of bounds (using same bounds as in Game)
            // This is approximate since we don't have the aspect ratio here
            if (arrow.x < -2.0f || arrow.x > 2.0f || arrow.y < -1.0f || arrow.y > 1.0f) {
                arrow.active = false;
                arrowsToRemove.push_back(i);
            }
        } else {
            // Arrow is already inactive, mark for removal
            arrowsToRemove.push_back(i);
        }
    }
    
    // Remove inactive arrows (starting from the end to avoid index issues)
    for (int i = arrowsToRemove.size() - 1; i >= 0; i--) {
        size_t index = arrowsToRemove[i];
        if (index < arrows.size()) {
            arrows.erase(arrows.begin() + index);
        }
    }
    
    // Cap the maximum number of active arrows to prevent performance issues
    if (arrows.size() > 10) {
        // Remove the oldest arrows if we have too many
        arrows.erase(arrows.begin(), arrows.begin() + (arrows.size() - 10));
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
                // Damage is half of player's arrow (assuming player does 10 damage)
                damage = 5;
                return true;
            }
        }
    }
    return false;
}
