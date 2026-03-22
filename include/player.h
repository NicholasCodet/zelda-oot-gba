#ifndef PLAYER_H
#define PLAYER_H

#include <gba_input.h>

#include "world.h"

// Cardinal directions used by player movement state.
typedef enum {
    DIRECTION_UP = 0,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_RIGHT
} Direction;

// Player gameplay state.
typedef struct Player {
    int x;
    int y;
    int speed;
    Direction direction;

    int maxHealth;
    int health;
    int invulnerabilityFrames;
    int invulnerabilityTimer;
    int isDead;

    // Simple temporary knockback state.
    int knockbackX;
    int knockbackY;
    int knockbackTimer;
} Player;

void initPlayer(
    Player *player,
    int startX,
    int startY,
    int speed,
    int maxHealth,
    int invulnerabilityFrames
);

void tickPlayerInvulnerability(Player *player);

GameObject getPlayerRect(const Player *player, int width, int height);

void updatePlayerMovement(
    Player *player,
    u16 keys,
    int rectWidth,
    int rectHeight,
    const GameObject *roomObstacles,
    int roomObstacleCount,
    const GameObject *toggleObstacles,
    int toggleObstacleCount
);

#endif
