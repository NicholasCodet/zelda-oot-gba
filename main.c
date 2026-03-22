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

// Clear the full Mode 3 framebuffer to a single color.
static void clearScreen(u16 color)
{
    volatile u16 *videoBuffer = (volatile u16 *)MODE3_FB;
    for (int i = 0; i < (SCREEN_WIDTH * SCREEN_HEIGHT); i++) {
        videoBuffer[i] = color;
    }
}

// Draw a filled rectangle in Mode 3.
static void drawFilledRect(int x, int y, int width, int height, u16 color)
{
    volatile u16 *videoBuffer = (volatile u16 *)MODE3_FB;

    for (int row = 0; row < height; row++) {
        int drawY = y + row;
        for (int col = 0; col < width; col++) {
            int drawX = x + col;
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
    GameObject interactiveObjects[] = {
        { .x = 170, .y = 80, .width = 16, .height = 16, .active = 0 },
        { .x = 40, .y = 100, .width = 16, .height = 16, .active = 0 }
    };
    GameObject toggleObstacles[] = {
        { .x = 150, .y = 40, .width = 24, .height = 24, .active = 0 },
        { .x = 68, .y = 88, .width = 28, .height = 20, .active = 0 }
    };
    const int interactiveCount = sizeof(interactiveObjects) / sizeof(interactiveObjects[0]);

    // Distinct OFF/ON colors for each interactive object.
    const u16 interactiveOffColor[] = { RGB5(0, 0, 31), RGB5(0, 31, 0) };
    const u16 interactiveOnColor[] = { RGB5(0, 31, 31), RGB5(31, 31, 0) };

    // Shared interaction distance.
    const int interactionRange = 10;

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

    // Track previous position so we can erase only the old rectangle.
    int prevRectX = player.x;
    int prevRectY = player.y;

    // Draw an initial frame so the rectangle is visible immediately.
    clearScreen(RGB5(0, 0, 0));
    for (int i = 0; i < roomObstacleCount; i++) {
        if (roomObstacles[i].active) {
            drawFilledRect(
                roomObstacles[i].x,
                roomObstacles[i].y,
                roomObstacles[i].width,
                roomObstacles[i].height,
                RGB5(31, 0, 0)
            );
        }
    }
    for (int i = 0; i < interactiveCount; i++) {
        drawFilledRect(
            interactiveObjects[i].x,
            interactiveObjects[i].y,
            interactiveObjects[i].width,
            interactiveObjects[i].height,
            interactiveOffColor[i]
        );
    }
    drawPlayer(&player, rectWidth, rectHeight, RGB5(31, 31, 31));

    // Basic game loop:
    // 1) wait for VBlank
    // 2) read input
    // 3) update/clamp position with obstacle collision
    // 4) handle interaction
    // 5) erase old rectangle and redraw dynamic objects
    while (1) {
        // 1) Synchronize to VBlank so drawing happens between frames.
        VBlankIntrWait();

        // 2) Read the current input state for this frame.
        scanKeys();
        u16 keys = keysHeld();
        u16 keysPressed = keysDown();

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

        // 4) Handle interaction with each object independently.
        for (int i = 0; i < interactiveCount; i++) {
            if ((keysPressed & KEY_A) && isPlayerNearObject(&player, rectWidth, rectHeight, &interactiveObjects[i], interactionRange)) {
                int nextState = !interactiveObjects[i].active;

                // Prevent activation if its obstacle would overlap the player.
                if (nextState) {
                    GameObject playerRect = {
                        .x = player.x,
                        .y = player.y,
                        .width = rectWidth,
                        .height = rectHeight,
                        .active = 1
                    };

                    if (!isCollidingAABB(&playerRect, &toggleObstacles[i])) {
                        interactiveObjects[i].active = nextState;
                        toggleObstacles[i].active = nextState;
                    }
                } else {
                    // Turning OFF is always safe.
                    interactiveObjects[i].active = nextState;
                    toggleObstacles[i].active = nextState;
                }
            }
        }

        // 5) Erase the previous rectangle only if it moved.
        // This avoids full-screen redraws that cause visible flicker in Mode 3.
        if (player.x != prevRectX || player.y != prevRectY) {
            Player previousPlayer = {
                .x = prevRectX,
                .y = prevRectY,
                .speed = player.speed,
                .direction = player.direction
            };
            drawPlayer(&previousPlayer, rectWidth, rectHeight, RGB5(0, 0, 0));
        }

        // Draw interactive objects with OFF/ON colors.
        for (int i = 0; i < interactiveCount; i++) {
            drawFilledRect(
                interactiveObjects[i].x,
                interactiveObjects[i].y,
                interactiveObjects[i].width,
                interactiveObjects[i].height,
                interactiveObjects[i].active ? interactiveOnColor[i] : interactiveOffColor[i]
            );
        }

        // Draw each interaction-controlled obstacle only when active.
        for (int i = 0; i < interactiveCount; i++) {
            if (toggleObstacles[i].active) {
                drawFilledRect(
                    toggleObstacles[i].x,
                    toggleObstacles[i].y,
                    toggleObstacles[i].width,
                    toggleObstacles[i].height,
                    RGB5(31, 0, 0)
                );
            } else {
                drawFilledRect(
                    toggleObstacles[i].x,
                    toggleObstacles[i].y,
                    toggleObstacles[i].width,
                    toggleObstacles[i].height,
                    RGB5(0, 0, 0)
                );
            }
        }

        // Draw the white rectangle at the updated position.
        drawPlayer(&player, rectWidth, rectHeight, RGB5(31, 31, 31));

        // Save position for the next frame's erase step.
        prevRectX = player.x;
        prevRectY = player.y;
    }

    return 0;
}
