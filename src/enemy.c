#include "enemy.h"

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
)
{
    enemy->x = startX;
    enemy->y = startY;
    enemy->width = width;
    enemy->height = height;
    enemy->active = 1;

    enemy->maxHealth = maxHealth;
    enemy->health = maxHealth;

    enemy->startX = startX;
    enemy->startY = startY;
    enemy->moveRange = moveRange;
    enemy->moveSpeed = moveSpeed;
    enemy->moveDirection = 1;
    enemy->moveAxis = moveAxis;
}

GameObject getEnemyRect(const Enemy *enemy)
{
    GameObject rect = {
        .x = enemy->x,
        .y = enemy->y,
        .width = enemy->width,
        .height = enemy->height,
        .active = enemy->active
    };

    return rect;
}

void updateEnemyMovement(
    Enemy *enemy,
    const GameObject *roomObstacles,
    int roomObstacleCount,
    const GameObject *toggleObstacles,
    int toggleObstacleCount
)
{
    if (!enemy->active) {
        return;
    }

    int nextX = enemy->x;
    int nextY = enemy->y;

    // Compute next position and patrol bounds on the configured axis.
    if (enemy->moveAxis == ENEMY_MOVE_AXIS_Y) {
        nextY = enemy->y + (enemy->moveDirection * enemy->moveSpeed);

        int minY = enemy->startY - enemy->moveRange;
        int maxY = enemy->startY + enemy->moveRange;
        if (nextY < minY || nextY > maxY) {
            enemy->moveDirection = -enemy->moveDirection;
            return;
        }
    } else {
        nextX = enemy->x + (enemy->moveDirection * enemy->moveSpeed);

        int minX = enemy->startX - enemy->moveRange;
        int maxX = enemy->startX + enemy->moveRange;
        if (nextX < minX || nextX > maxX) {
            enemy->moveDirection = -enemy->moveDirection;
            return;
        }
    }

    // Test the enemy's next rectangle against active room and toggle obstacles.
    GameObject nextEnemyRect = {
        .x = nextX,
        .y = nextY,
        .width = enemy->width,
        .height = enemy->height,
        .active = 1
    };

    int enemyBlocked = isCollidingWithActiveObjects(&nextEnemyRect, roomObstacles, roomObstacleCount);
    if (!enemyBlocked) {
        enemyBlocked = isCollidingWithActiveObjects(&nextEnemyRect, toggleObstacles, toggleObstacleCount);
    }

    if (enemyBlocked) {
        // Do not enter obstacles; reverse direction for the next frame.
        enemy->moveDirection = -enemy->moveDirection;
    } else {
        // Free path: apply the movement along the configured axis.
        enemy->x = nextX;
        enemy->y = nextY;
    }
}
