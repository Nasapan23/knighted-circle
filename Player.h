#ifndef PLAYER_H
#define PLAYER_H

#include "dependente/glfw/glfw3.h"

class Player {
public:
    float x, y;
    float radius;
    float speed;

    // Constructor: starting position, radius, and speed.
    Player(float startX, float startY, float rad, float spd);
    void update(GLFWwindow* window);
};

#endif
