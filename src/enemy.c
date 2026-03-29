#include "enemy.h"
#include "player.h"

static int isEnemyMoveBlocked(
    int nextX,
    int nextY,
    const Enemy *enemy,
    const GameObject *roomObstacles,
    int roomObstacleCount,
    const GameObject *toggleObstacles,
    int toggleObstacleCount
)
{
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

    return enemyBlocked;
}

static int absoluteValue(int value)
{
    return (value < 0) ? -value : value;
}

static int signValue(int value)
{
    if (value > 0) {
        return 1;
    }
    if (value < 0) {
        return -1;
    }
    return 0;
}

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
    enemy->active = (maxHealth > 0) ? 1 : 0;

    enemy->maxHealth = maxHealth;
    enemy->health = (maxHealth > 0) ? maxHealth : 0;
    // Keep boss tagging simple for now: high-health enemies are treated as bosses.
    enemy->isBoss = (maxHealth >= 4) ? 1 : 0;

    enemy->startX = startX;
    enemy->startY = startY;
    enemy->moveRange = moveRange;
    enemy->moveSpeed = moveSpeed;
    enemy->moveDirection = 1;
    enemy->moveAxis = moveAxis;
    enemy->bossRecoverTimer = 0;
    enemy->bossRetreatX = 0;
    enemy->bossRetreatY = 0;
    enemy->bossRetreatTimer = 0;
    enemy->bossChaseTimer = 0;
    enemy->bossPauseTimer = 0;

    enemy->hitFlashTimer = 0;
    enemy->hitFlashDuration = 6;
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
    const Player *player,
    const GameObject *roomObstacles,
    int roomObstacleCount,
    const GameObject *toggleObstacles,
    int toggleObstacleCount
)
{
    if (!enemy->active) {
        return;
    }

    // Boss behavior: direct chase, but with a short recovery pause after
    // successful contact to keep the fight readable and less sticky.
    if (enemy->isBoss && player != 0) {
        // After a successful hit on the player, briefly force the boss to
        // retreat so it cannot stay glued to the player rectangle.
        if (enemy->bossRetreatTimer > 0) {
            int moved = 0;

            if (enemy->bossRetreatX != 0) {
                int nextX = enemy->x + enemy->bossRetreatX;
                if (!isEnemyMoveBlocked(
                        nextX,
                        enemy->y,
                        enemy,
                        roomObstacles,
                        roomObstacleCount,
                        toggleObstacles,
                        toggleObstacleCount
                    )) {
                    enemy->x = nextX;
                    moved = 1;
                }
            }

            if (enemy->bossRetreatY != 0) {
                int nextY = enemy->y + enemy->bossRetreatY;
                if (!isEnemyMoveBlocked(
                        enemy->x,
                        nextY,
                        enemy,
                        roomObstacles,
                        roomObstacleCount,
                        toggleObstacles,
                        toggleObstacleCount
                    )) {
                    enemy->y = nextY;
                    moved = 1;
                }
            }

            if (moved) {
                enemy->bossRetreatTimer--;
            } else {
                // Stop retreat early if both axes are blocked.
                enemy->bossRetreatTimer = 0;
            }

            return;
        }

        if (enemy->bossRecoverTimer > 0) {
            enemy->bossRecoverTimer--;
            return;
        }

        // Add short readable pacing windows:
        // a brief chase phase, then a short pause.
        if (enemy->bossPauseTimer > 0) {
            enemy->bossPauseTimer--;
            return;
        }
        if (enemy->bossChaseTimer <= 0) {
            enemy->bossChaseTimer = 20;
        }

        // Keep boss speed moderate: pressure comes from higher health and
        // consistent axis-based pursuit rather than diagonal free movement.
        int bossStep = enemy->moveSpeed;
        int deltaX = player->x - enemy->x;
        int deltaY = player->y - enemy->y;
        int moveX = 0;
        int moveY = 0;
        int moved = 0;
        const int axisAlignThreshold = 3;

        // Axis-first behavior:
        // 1) align horizontally OR vertically
        // 2) move only on one axis in the current frame.
        if (absoluteValue(deltaX) > axisAlignThreshold &&
            absoluteValue(deltaX) >= absoluteValue(deltaY)) {
            moveX = signValue(deltaX) * bossStep;
        } else if (absoluteValue(deltaY) > axisAlignThreshold) {
            moveY = signValue(deltaY) * bossStep;
        } else if (absoluteValue(deltaX) > axisAlignThreshold) {
            moveX = signValue(deltaX) * bossStep;
        } else if (absoluteValue(deltaY) > 0) {
            moveY = signValue(deltaY) * bossStep;
        } else if (absoluteValue(deltaX) > 0) {
            moveX = signValue(deltaX) * bossStep;
        }

        // Apply one-axis move. If blocked, try the other axis only as fallback.
        if (moveX != 0) {
            int nextX = enemy->x + moveX;
            if (!isEnemyMoveBlocked(
                    nextX,
                    enemy->y,
                    enemy,
                    roomObstacles,
                    roomObstacleCount,
                    toggleObstacles,
                    toggleObstacleCount
                )) {
                enemy->x = nextX;
                moved = 1;
            } else if (absoluteValue(deltaY) > axisAlignThreshold) {
                int fallbackY = signValue(deltaY) * bossStep;
                int nextY = enemy->y + fallbackY;
                if (!isEnemyMoveBlocked(
                        enemy->x,
                        nextY,
                        enemy,
                        roomObstacles,
                        roomObstacleCount,
                        toggleObstacles,
                        toggleObstacleCount
                    )) {
                    enemy->y = nextY;
                    moved = 1;
                }
            }
        } else if (moveY != 0) {
            int nextY = enemy->y + moveY;
            if (!isEnemyMoveBlocked(
                    enemy->x,
                    nextY,
                    enemy,
                    roomObstacles,
                    roomObstacleCount,
                    toggleObstacles,
                    toggleObstacleCount
                )) {
                enemy->y = nextY;
                moved = 1;
            } else if (absoluteValue(deltaX) > axisAlignThreshold) {
                int fallbackX = signValue(deltaX) * bossStep;
                int nextX = enemy->x + fallbackX;
                if (!isEnemyMoveBlocked(
                        nextX,
                        enemy->y,
                        enemy,
                        roomObstacles,
                        roomObstacleCount,
                        toggleObstacles,
                        toggleObstacleCount
                    )) {
                    enemy->x = nextX;
                    moved = 1;
                }
            }
        }

        if (moved) {
            enemy->bossChaseTimer--;
            if (enemy->bossChaseTimer <= 0) {
                enemy->bossPauseTimer = 8;
            }
        } else {
            // If no valid move, briefly pause then retry.
            enemy->bossPauseTimer = 6;
            enemy->bossChaseTimer = 0;
        }

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

    int enemyBlocked = isEnemyMoveBlocked(
        nextX,
        nextY,
        enemy,
        roomObstacles,
        roomObstacleCount,
        toggleObstacles,
        toggleObstacleCount
    );

    if (enemyBlocked) {
        // Do not enter obstacles; reverse direction for the next frame.
        enemy->moveDirection = -enemy->moveDirection;
    } else {
        // Free path: apply the movement along the configured axis.
        enemy->x = nextX;
        enemy->y = nextY;
    }
}
