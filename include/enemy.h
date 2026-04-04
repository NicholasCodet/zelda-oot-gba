#ifndef ENEMY_H
#define ENEMY_H

#include "world.h"

struct Player;

// Simple enemy behavior variants used by rooms.
typedef enum {
    ENEMY_TYPE_CHASER = 0,
    ENEMY_TYPE_PATROL = 1,
    ENEMY_TYPE_BRUTE = 2,
    ENEMY_TYPE_BOSS = 3
} EnemyType;

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
    EnemyType type;

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
    // Chaser anti-stick state after contact.
    int chaserRecoverTimer;
    int chaserRetreatX;
    int chaserRetreatY;
    int chaserRetreatTimer;
    int behaviorTick;

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
    EnemyMoveAxis moveAxis,
    EnemyType type
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
