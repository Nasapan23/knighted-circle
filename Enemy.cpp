#include "Enemy.h"
#include <cmath>

Enemy::Enemy(float startX, float startY, float rad, float spd)
    : x(startX), y(startY), radius(rad), speed(spd) {
}

void Enemy::update(float playerX, float playerY) {
    float dx = playerX - x;
    float dy = playerY - y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len != 0) {
        dx /= len;
        dy /= len;
    }
    x += speed * dx;
    y += speed * dy;
}
