#ifndef ENEMY_H
#define ENEMY_H

class Enemy {
public:
    float x, y;
    float radius;
    float speed;

    Enemy(float startX, float startY, float rad, float spd);
    void update(float playerX, float playerY);
};

#endif
