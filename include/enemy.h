#ifndef ENEMY_H
#define ENEMY_H

#include "world.h"

// Enemy gameplay state.
typedef struct {
    int x;
    int y;
    int width;
    int height;
    int active;

    int maxHealth;
    int health;

    int startX;
    int moveRange;
    int moveSpeed;
    int moveDirection; // 1 = right, -1 = left
} Enemy;

void initEnemy(
    Enemy *enemy,
    int startX,
    int startY,
    int width,
    int height,
    int maxHealth,
    int moveRange,
    int moveSpeed
);

GameObject getEnemyRect(const Enemy *enemy);

void updateEnemyMovement(
    Enemy *enemy,
    const GameObject *roomObstacles,
    int roomObstacleCount,
    const GameObject *toggleObstacles,
    int toggleObstacleCount
);

#endif
