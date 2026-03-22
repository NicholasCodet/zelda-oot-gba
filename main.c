#include <gba_systemcalls.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_input.h>

// GBA screen size in Mode 3.
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160

// Four cardinal directions used by player movement state.
typedef enum {
    DIRECTION_UP = 0,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_RIGHT
} Direction;

// Minimal player data used in this file for movement and drawing.
typedef struct {
    int x;
    int y;
    int speed;
    Direction direction;
} Player;

// Generic room/interactable object used for drawing and collision.
typedef struct {
    int x;
    int y;
    int width;
    int height;
    int active;
} GameObject;

// Draw a filled rectangle in Mode 3.
static void drawFilledRect(int x, int y, int width, int height, u16 color)
{
    volatile u16 *videoBuffer = (volatile u16 *)MODE3_FB;

    // Clip rectangle bounds to the visible screen to avoid out-of-range writes.
    int startX = x;
    int startY = y;
    int endX = x + width;
    int endY = y + height;

    if (startX < 0) {
        startX = 0;
    }
    if (startY < 0) {
        startY = 0;
    }
    if (endX > SCREEN_WIDTH) {
        endX = SCREEN_WIDTH;
    }
    if (endY > SCREEN_HEIGHT) {
        endY = SCREEN_HEIGHT;
    }

    // If fully off-screen, nothing needs to be drawn.
    if (startX >= endX || startY >= endY) {
        return;
    }

    for (int drawY = startY; drawY < endY; drawY++) {
        for (int drawX = startX; drawX < endX; drawX++) {
            videoBuffer[drawY * SCREEN_WIDTH + drawX] = color;
        }
    }
}

// Return 1 if two axis-aligned rectangles overlap, otherwise 0.
static int isCollidingAABB(const GameObject *a, const GameObject *b)
{
    return (a->x < (b->x + b->width)) &&
           ((a->x + a->width) > b->x) &&
           (a->y < (b->y + b->height)) &&
           ((a->y + a->height) > b->y);
}

// Return 1 if object overlaps any active object in the list, otherwise 0.
static int isCollidingWithActiveObjects(const GameObject *object, const GameObject *objects, int objectCount)
{
    for (int i = 0; i < objectCount; i++) {
        if (objects[i].active && isCollidingAABB(object, &objects[i])) {
            return 1;
        }
    }

    return 0;
}

// Return 1 if the player rectangle is within interaction range of an object.
static int isPlayerNearObject(const Player *player, int playerWidth, int playerHeight, const GameObject *object, int range)
{
    // Player rectangle at current position.
    GameObject playerRect = {
        .x = player->x,
        .y = player->y,
        .width = playerWidth,
        .height = playerHeight,
        .active = 1
    };

    // Expand the object rectangle to create a simple interaction zone.
    GameObject interactionZone = {
        .x = object->x - range,
        .y = object->y - range,
        .width = object->width + (range * 2),
        .height = object->height + (range * 2),
        .active = 1
    };

    return isCollidingAABB(&playerRect, &interactionZone);
}

// Update player position from input, screen bounds, and obstacle collision.
static void updatePlayer(
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
        if (nextX > (SCREEN_WIDTH - rectWidth)) {
            nextX = SCREEN_WIDTH - rectWidth;
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
        if (nextY > (SCREEN_HEIGHT - rectHeight)) {
            nextY = SCREEN_HEIGHT - rectHeight;
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

// Draw the player as a filled rectangle using the current position.
static void drawPlayer(const Player *player, int rectWidth, int rectHeight, u16 color)
{
    drawFilledRect(player->x, player->y, rectWidth, rectHeight, color);
}

// Redraw only one region of the scene.
// This restores background/static content underneath moving elements.
static void redrawSceneRegion(
    const GameObject *region,
    const GameObject *roomObstacles,
    int roomObstacleCount,
    const GameObject *toggleObstacles,
    int toggleObstacleCount,
    const GameObject *interactiveObjects,
    int interactiveCount,
    const u16 *interactiveOffColor,
    const u16 *interactiveOnColor,
    const GameObject *goalArea,
    int hasWon,
    const GameObject *enemyTarget
)
{
    // Start by restoring background color in this region.
    drawFilledRect(region->x, region->y, region->width, region->height, RGB5(0, 0, 0));

    // Restore always-on room obstacles.
    for (int i = 0; i < roomObstacleCount; i++) {
        if (roomObstacles[i].active && isCollidingAABB(region, &roomObstacles[i])) {
            drawFilledRect(
                roomObstacles[i].x,
                roomObstacles[i].y,
                roomObstacles[i].width,
                roomObstacles[i].height,
                RGB5(31, 0, 0)
            );
        }
    }

    // Restore currently active toggle obstacles.
    for (int i = 0; i < toggleObstacleCount; i++) {
        if (toggleObstacles[i].active && isCollidingAABB(region, &toggleObstacles[i])) {
            drawFilledRect(
                toggleObstacles[i].x,
                toggleObstacles[i].y,
                toggleObstacles[i].width,
                toggleObstacles[i].height,
                RGB5(31, 0, 0)
            );
        }
    }

    // Restore interactive objects using their current OFF/ON color states.
    for (int i = 0; i < interactiveCount; i++) {
        if (isCollidingAABB(region, &interactiveObjects[i])) {
            drawFilledRect(
                interactiveObjects[i].x,
                interactiveObjects[i].y,
                interactiveObjects[i].width,
                interactiveObjects[i].height,
                interactiveObjects[i].active ? interactiveOnColor[i] : interactiveOffColor[i]
            );
        }
    }

    // Restore goal with current win color.
    if (goalArea->active && isCollidingAABB(region, goalArea)) {
        if (hasWon) {
            drawFilledRect(goalArea->x, goalArea->y, goalArea->width, goalArea->height, RGB5(31, 31, 31));
        } else {
            drawFilledRect(goalArea->x, goalArea->y, goalArea->width, goalArea->height, RGB5(31, 0, 31));
        }
    }

    // Restore enemy only when active.
    if (enemyTarget->active && isCollidingAABB(region, enemyTarget)) {
        drawFilledRect(enemyTarget->x, enemyTarget->y, enemyTarget->width, enemyTarget->height, RGB5(31, 16, 0));
    }
}

int main(void)
{
    // Initialize the interrupt system so VBlank waiting works correctly.
    irqInit();
    irqEnable(IRQ_VBLANK);

    // Set the display to Mode 3 with background 2 enabled.
    // Mode 3 is a simple bitmap mode we can clear to a solid color.
    REG_DISPCNT = MODE_3 | BG2_ON;

    // Rectangle size for the player marker.
    const int rectWidth = 12;
    const int rectHeight = 12;

    // Two interactive objects; each controls one toggle obstacle.
    // Objects start OFF, and their linked obstacles start ON (blocking).
    GameObject interactiveObjects[] = {
        { .x = 52, .y = 44, .width = 16, .height = 16, .active = 0 },
        { .x = 52, .y = 96, .width = 16, .height = 16, .active = 0 }
    };
    GameObject toggleObstacles[] = {
        // These two vertical gates form a full-height barrier to the goal side.
        { .x = 150, .y = 28, .width = 12, .height = 104, .active = 1 },
        { .x = 162, .y = 28, .width = 12, .height = 104, .active = 1 }
    };
    const int interactiveCount = sizeof(interactiveObjects) / sizeof(interactiveObjects[0]);

    // Distinct OFF/ON colors for each interactive object.
    const u16 interactiveOffColor[] = { RGB5(0, 0, 31), RGB5(0, 31, 0) };
    const u16 interactiveOnColor[] = { RGB5(0, 31, 31), RGB5(31, 31, 0) };

    // Shared interaction distance.
    const int interactionRange = 10;

    // Goal area for the simple win condition.
    // The player wins by reaching this area after activating the room interactions.
    GameObject goalArea = {
        .x = 184,
        .y = 34,
        .width = 18,
        .height = 18,
        .active = 1
    };
    int hasWon = 0;

    // Simple static enemy target for combat testing.
    // It stays visible until hit by the player's attack.
    GameObject enemyTarget = {
        .x = 116,
        .y = 40,
        .width = 14,
        .height = 14,
        .active = 1
    };

    // Simple short-lived attack hitbox.
    const int attackWidth = 10;
    const int attackHeight = 10;
    const int attackDurationFrames = 6;
    int attackTimer = 0;
    GameObject attackHitbox = {
        .x = 0,
        .y = 0,
        .width = attackWidth,
        .height = attackHeight,
        .active = 0
    };

    // Fixed room obstacles (always active).
    GameObject roomObstacles[] = {
        // Room walls.
        { .x = 20, .y = 20, .width = 200, .height = 8, .active = 1 },   // Top wall
        { .x = 20, .y = 132, .width = 200, .height = 8, .active = 1 },  // Bottom wall
        { .x = 20, .y = 20, .width = 8, .height = 120, .active = 1 },   // Left wall
        { .x = 212, .y = 20, .width = 8, .height = 120, .active = 1 },  // Right wall

        // Internal block to test sliding/collision in the room.
        { .x = 100, .y = 64, .width = 40, .height = 24, .active = 1 }
    };
    const int roomObstacleCount = sizeof(roomObstacles) / sizeof(roomObstacles[0]);

    // Create a simple player with a fixed walkable spawn and fixed speed.
    Player player = {
        .x = 40,
        .y = 40,
        .speed = 1,
        .direction = DIRECTION_DOWN
    };

    // Track previous dynamic rectangles for incremental redraw.
    GameObject prevPlayerRect = {
        .x = player.x,
        .y = player.y,
        .width = rectWidth,
        .height = rectHeight,
        .active = 1
    };
    GameObject prevAttackRect = {
        .x = 0,
        .y = 0,
        .width = attackWidth,
        .height = attackHeight,
        .active = 0
    };
    int prevAttackWasActive = 0;

    // Draw the initial scene once.
    GameObject fullScreenRegion = {
        .x = 0,
        .y = 0,
        .width = SCREEN_WIDTH,
        .height = SCREEN_HEIGHT,
        .active = 1
    };
    redrawSceneRegion(
        &fullScreenRegion,
        roomObstacles,
        roomObstacleCount,
        toggleObstacles,
        interactiveCount,
        interactiveObjects,
        interactiveCount,
        interactiveOffColor,
        interactiveOnColor,
        &goalArea,
        hasWon,
        &enemyTarget
    );
    drawPlayer(&player, rectWidth, rectHeight, RGB5(31, 31, 31));

    // Basic game loop:
    // 1) wait for VBlank
    // 2) read input
    // 3) update/clamp position with obstacle collision
    // 4) handle interaction
    // 5) restore changed regions and draw current dynamic elements
    while (1) {
        // 1) Synchronize to VBlank so drawing happens between frames.
        VBlankIntrWait();

        // 2) Read the current input state for this frame.
        scanKeys();
        u16 keys = keysHeld();
        u16 keysPressed = keysDown();

        // Save current semi-static states so we can detect visual changes.
        int previousHasWon = hasWon;
        int previousEnemyActive = enemyTarget.active;
        int previousInteractiveState[interactiveCount];
        int previousToggleState[interactiveCount];
        for (int i = 0; i < interactiveCount; i++) {
            previousInteractiveState[i] = interactiveObjects[i].active;
            previousToggleState[i] = toggleObstacles[i].active;
        }

        // 3) Update movement, bounds, and obstacle collision.
        updatePlayer(
            &player,
            keys,
            rectWidth,
            rectHeight,
            roomObstacles,
            roomObstacleCount,
            toggleObstacles,
            interactiveCount
        );

        // B button attack:
        // Spawn a short attack hitbox in front of the player based on direction.
        if ((keysPressed & KEY_B) && attackTimer == 0) {
            attackTimer = attackDurationFrames;
            attackHitbox.active = 1;
            attackHitbox.width = attackWidth;
            attackHitbox.height = attackHeight;

            if (player.direction == DIRECTION_UP) {
                attackHitbox.x = player.x + (rectWidth - attackWidth) / 2;
                attackHitbox.y = player.y - attackHeight;
            } else if (player.direction == DIRECTION_DOWN) {
                attackHitbox.x = player.x + (rectWidth - attackWidth) / 2;
                attackHitbox.y = player.y + rectHeight;
            } else if (player.direction == DIRECTION_LEFT) {
                attackHitbox.x = player.x - attackWidth;
                attackHitbox.y = player.y + (rectHeight - attackHeight) / 2;
            } else { // DIRECTION_RIGHT
                attackHitbox.x = player.x + rectWidth;
                attackHitbox.y = player.y + (rectHeight - attackHeight) / 2;
            }
        }

        // Keep attack active for a few frames and resolve enemy hit.
        if (attackTimer > 0) {
            if (enemyTarget.active && attackHitbox.active && isCollidingAABB(&attackHitbox, &enemyTarget)) {
                enemyTarget.active = 0;
            }

            attackTimer--;
            if (attackTimer == 0) {
                attackHitbox.active = 0;
            }
        }

        // 4) Handle interaction with each object independently.
        for (int i = 0; i < interactiveCount; i++) {
            if ((keysPressed & KEY_A) && isPlayerNearObject(&player, rectWidth, rectHeight, &interactiveObjects[i], interactionRange)) {
                int nextState = !interactiveObjects[i].active;

                // Inverted logic:
                // object ON  -> linked obstacle OFF (path opens)
                // object OFF -> linked obstacle ON  (path closes)
                if (nextState) {
                    // Activating the object removes its obstacle.
                    interactiveObjects[i].active = 1;
                    toggleObstacles[i].active = 0;
                } else {
                    // Deactivating the object restores its obstacle.
                    // Keep the overlap guard to avoid trapping the player.
                    GameObject playerRect = {
                        .x = player.x,
                        .y = player.y,
                        .width = rectWidth,
                        .height = rectHeight,
                        .active = 1
                    };

                    if (!isCollidingAABB(&playerRect, &toggleObstacles[i])) {
                        interactiveObjects[i].active = 0;
                        toggleObstacles[i].active = 1;
                    }
                }
            }
        }

        // Check win condition:
        // 1) all interactive objects are active
        // 2) player rectangle overlaps the goal area
        if (!hasWon) {
            int allInteractionsActive = 1;
            for (int i = 0; i < interactiveCount; i++) {
                if (!interactiveObjects[i].active) {
                    allInteractionsActive = 0;
                    break;
                }
            }

            if (allInteractionsActive) {
                GameObject playerRect = {
                    .x = player.x,
                    .y = player.y,
                    .width = rectWidth,
                    .height = rectHeight,
                    .active = 1
                };

                if (isCollidingAABB(&playerRect, &goalArea)) {
                    hasWon = 1;
                }
            }
        }

        // 5) Restore the regions where dynamic elements were previously drawn.
        redrawSceneRegion(
            &prevPlayerRect,
            roomObstacles,
            roomObstacleCount,
            toggleObstacles,
            interactiveCount,
            interactiveObjects,
            interactiveCount,
            interactiveOffColor,
            interactiveOnColor,
            &goalArea,
            hasWon,
            &enemyTarget
        );
        if (prevAttackWasActive) {
            redrawSceneRegion(
                &prevAttackRect,
                roomObstacles,
                roomObstacleCount,
                toggleObstacles,
                interactiveCount,
                interactiveObjects,
                interactiveCount,
                interactiveOffColor,
                interactiveOnColor,
                &goalArea,
                hasWon,
                &enemyTarget
            );
        }

        // Restore regions for semi-static objects whose state changed this frame.
        for (int i = 0; i < interactiveCount; i++) {
            if (previousInteractiveState[i] != interactiveObjects[i].active) {
                redrawSceneRegion(
                    &interactiveObjects[i],
                    roomObstacles,
                    roomObstacleCount,
                    toggleObstacles,
                    interactiveCount,
                    interactiveObjects,
                    interactiveCount,
                    interactiveOffColor,
                    interactiveOnColor,
                    &goalArea,
                    hasWon,
                    &enemyTarget
                );
            }
            if (previousToggleState[i] != toggleObstacles[i].active) {
                redrawSceneRegion(
                    &toggleObstacles[i],
                    roomObstacles,
                    roomObstacleCount,
                    toggleObstacles,
                    interactiveCount,
                    interactiveObjects,
                    interactiveCount,
                    interactiveOffColor,
                    interactiveOnColor,
                    &goalArea,
                    hasWon,
                    &enemyTarget
                );
            }
        }
        if (previousHasWon != hasWon) {
            redrawSceneRegion(
                &goalArea,
                roomObstacles,
                roomObstacleCount,
                toggleObstacles,
                interactiveCount,
                interactiveObjects,
                interactiveCount,
                interactiveOffColor,
                interactiveOnColor,
                &goalArea,
                hasWon,
                &enemyTarget
            );
        }
        if (previousEnemyActive != enemyTarget.active) {
            redrawSceneRegion(
                &enemyTarget,
                roomObstacles,
                roomObstacleCount,
                toggleObstacles,
                interactiveCount,
                interactiveObjects,
                interactiveCount,
                interactiveOffColor,
                interactiveOnColor,
                &goalArea,
                hasWon,
                &enemyTarget
            );
        }

        // Draw attack hitbox while it is active.
        if (attackHitbox.active) {
            drawFilledRect(attackHitbox.x, attackHitbox.y, attackHitbox.width, attackHitbox.height, RGB5(31, 31, 0));
        }

        // Draw the white rectangle at the updated position.
        drawPlayer(&player, rectWidth, rectHeight, RGB5(31, 31, 31));

        // Store current dynamic rectangles for the next frame erase/restore step.
        prevPlayerRect.x = player.x;
        prevPlayerRect.y = player.y;
        prevPlayerRect.width = rectWidth;
        prevPlayerRect.height = rectHeight;
        prevPlayerRect.active = 1;

        if (attackHitbox.active) {
            prevAttackRect = attackHitbox;
            prevAttackWasActive = 1;
        } else {
            prevAttackWasActive = 0;
        }
    }

    return 0;
}
