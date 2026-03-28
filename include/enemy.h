#ifndef ENEMY_H
#define ENEMY_H

#include "world.h"

// Simple patrol axis variants for enemy movement.
typedef enum {
    ENEMY_MOVE_AXIS_X = 0,
    ENEMY_MOVE_AXIS_Y = 1
} EnemyMoveAxis;

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
    int startY;
    int moveRange;
    int moveSpeed;
    int moveDirection; // 1 = positive axis direction, -1 = negative
    EnemyMoveAxis moveAxis;
} Enemy;

void initEnemy(
    Enemy *enemy,
    int startX,
    int startY,
    int width,
    int height,
    int maxHealth,
    int moveRange,
    int moveSpeed,
    EnemyMoveAxis moveAxis
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
