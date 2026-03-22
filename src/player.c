#include "player.h"

void initPlayer(
    Player *player,
    int startX,
    int startY,
    int speed,
    int maxHealth,
    int invulnerabilityFrames
)
{
    player->x = startX;
    player->y = startY;
    player->speed = speed;
    player->direction = DIRECTION_DOWN;

    player->maxHealth = maxHealth;
    player->health = maxHealth;
    player->invulnerabilityFrames = invulnerabilityFrames;
    player->invulnerabilityTimer = 0;
    player->isDead = 0;
}

void tickPlayerInvulnerability(Player *player)
{
    if (player->invulnerabilityTimer > 0) {
        player->invulnerabilityTimer--;
    }
}

GameObject getPlayerRect(const Player *player, int width, int height)
{
    GameObject rect = {
        .x = player->x,
        .y = player->y,
        .width = width,
        .height = height,
        .active = 1
    };

    return rect;
}

void updatePlayerMovement(
    Player *player,
    u16 keys,
    int rectWidth,
    int rectHeight,
    const GameObject *roomObstacles,
    int roomObstacleCount,
    const GameObject *toggleObstacles,
    int toggleObstacleCount
)
{
    if (player->isDead) {
        return;
    }

    // Compute requested movement for this frame.
    int moveX = 0;
    int moveY = 0;

    // Move one pixel per frame while a direction is held.
    if (keys & KEY_UP) {
        moveY -= player->speed;
        player->direction = DIRECTION_UP;
    }
    if (keys & KEY_DOWN) {
        moveY += player->speed;
        player->direction = DIRECTION_DOWN;
    }
    if (keys & KEY_LEFT) {
        moveX -= player->speed;
        player->direction = DIRECTION_LEFT;
    }
    if (keys & KEY_RIGHT) {
        moveX += player->speed;
        player->direction = DIRECTION_RIGHT;
    }

    // Resolve X movement first.
    if (moveX != 0) {
        int nextX = player->x + moveX;

        // Clamp to screen bounds on X.
        if (nextX < 0) {
            nextX = 0;
        }
        if (nextX > (240 - rectWidth)) {
            nextX = 240 - rectWidth;
        }

        // Test collision using new X and current Y.
        GameObject nextPlayerRectX = {
            .x = nextX,
            .y = player->y,
            .width = rectWidth,
            .height = rectHeight,
            .active = 1
        };

        int isBlockedX = isCollidingWithActiveObjects(&nextPlayerRectX, roomObstacles, roomObstacleCount);
        if (!isBlockedX) {
            isBlockedX = isCollidingWithActiveObjects(&nextPlayerRectX, toggleObstacles, toggleObstacleCount);
        }

        if (!isBlockedX) {
            player->x = nextX;
        }
    }

    // Resolve Y movement after X.
    if (moveY != 0) {
        int nextY = player->y + moveY;

        // Clamp to screen bounds on Y.
        if (nextY < 0) {
            nextY = 0;
        }
        if (nextY > (160 - rectHeight)) {
            nextY = 160 - rectHeight;
        }

        // Test collision using current X and new Y.
        GameObject nextPlayerRectY = {
            .x = player->x,
            .y = nextY,
            .width = rectWidth,
            .height = rectHeight,
            .active = 1
        };

        int isBlockedY = isCollidingWithActiveObjects(&nextPlayerRectY, roomObstacles, roomObstacleCount);
        if (!isBlockedY) {
            isBlockedY = isCollidingWithActiveObjects(&nextPlayerRectY, toggleObstacles, toggleObstacleCount);
        }

        if (!isBlockedY) {
            player->y = nextY;
        }
    }
}
