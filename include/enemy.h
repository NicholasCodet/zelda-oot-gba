#ifndef ENEMY_H
#define ENEMY_H

#include "world.h"

struct Player;

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
    int isBoss;

    int startX;
    int startY;
    int moveRange;
    int moveSpeed;
    int moveDirection; // 1 = positive axis direction, -1 = negative
    EnemyMoveAxis moveAxis;

    // Short cooldown after boss contact damage to avoid sticky re-contact.
    int bossRecoverTimer;
    // Short forced retreat state after boss contact.
    int bossRetreatX;
    int bossRetreatY;
    int bossRetreatTimer;
    // Simple readable boss pacing: chase windows followed by short pauses.
    int bossChaseTimer;
    int bossPauseTimer;

    // Short visual feedback timer used when the enemy is hit.
    int hitFlashTimer;
    int hitFlashDuration;
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
    const struct Player *player,
    const GameObject *roomObstacles,
    int roomObstacleCount,
    const GameObject *toggleObstacles,
    int toggleObstacleCount
);

#endif
