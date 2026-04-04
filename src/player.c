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
    player->knockbackX = 0;
    player->knockbackY = 0;
    player->knockbackTimer = 0;
    player->dashX = 0;
    player->dashY = 0;
    player->dashTimer = 0;
    player->dashDuration = 5;
    player->dashSpeed = 3;
    player->dashCooldownTimer = 0;
    player->dashCooldownFrames = 24;
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
    u16 keysPressed,
    int rectWidth,
    int rectHeight,
    const GameObject *roomObstacles,
    int roomObstacleCount,
    const GameObject *toggleObstacles,
    int toggleObstacleCount,
    const GameObject *extraObstacles,
    int extraObstacleCount
)
{
    if (player->isDead) {
        return;
    }

    int moveX = 0;
    int moveY = 0;

    if (player->dashCooldownTimer > 0) {
        player->dashCooldownTimer--;
    }

    // Apply knockback movement first.
    // While active, normal directional movement is temporarily disabled.
    if (player->knockbackTimer > 0) {
        moveX = player->knockbackX;
        moveY = player->knockbackY;
        player->knockbackTimer--;
    } else {
        // Continue an active dash.
        if (player->dashTimer > 0) {
            moveX = player->dashX;
            moveY = player->dashY;
            player->dashTimer--;
        } else {
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

            // Start a dash in the current facing direction.
            // Keep the logic simple: short burst + short cooldown.
            if ((keysPressed & KEY_L) && !(keysPressed & KEY_B) && player->dashCooldownTimer == 0) {
                player->dashX = 0;
                player->dashY = 0;

                if (player->direction == DIRECTION_UP) {
                    player->dashY = -player->dashSpeed;
                } else if (player->direction == DIRECTION_DOWN) {
                    player->dashY = player->dashSpeed;
                } else if (player->direction == DIRECTION_LEFT) {
                    player->dashX = -player->dashSpeed;
                } else {
                    player->dashX = player->dashSpeed;
                }

                moveX = player->dashX;
                moveY = player->dashY;
                player->dashTimer = player->dashDuration - 1;
                player->dashCooldownTimer = player->dashCooldownFrames;
            }
        }
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
            isBlockedX = isCollidingWithActiveObjects(&nextPlayerRectX, extraObstacles, extraObstacleCount);
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
            isBlockedY = isCollidingWithActiveObjects(&nextPlayerRectY, extraObstacles, extraObstacleCount);
        }

        if (!isBlockedY) {
            player->y = nextY;
        }
    }
}
