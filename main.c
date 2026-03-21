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

// Simple rectangle type used for obstacle and collision checks.
typedef struct {
    int x;
    int y;
    int width;
    int height;
} Rect;

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
static int isCollidingAABB(const Rect *a, const Rect *b)
{
    return (a->x < (b->x + b->width)) &&
           ((a->x + a->width) > b->x) &&
           (a->y < (b->y + b->height)) &&
           ((a->y + a->height) > b->y);
}

// Update player position from input, screen bounds, and obstacle collision.
static void updatePlayer(Player *player, u16 keys, int rectWidth, int rectHeight, const Rect *obstacle)
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
        Rect nextPlayerRectX = {
            .x = nextX,
            .y = player->y,
            .width = rectWidth,
            .height = rectHeight
        };

        if (!isCollidingAABB(&nextPlayerRectX, obstacle)) {
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
        Rect nextPlayerRectY = {
            .x = player->x,
            .y = nextY,
            .width = rectWidth,
            .height = rectHeight
        };

        if (!isCollidingAABB(&nextPlayerRectY, obstacle)) {
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

    // Fixed obstacle rectangle (drawn in red).
    const Rect obstacle = {
        .x = 60,
        .y = 50,
        .width = 48,
        .height = 28
    };

    // Create a simple player with centered start position and fixed speed.
    Player player = {
        .x = (SCREEN_WIDTH - rectWidth) / 2,
        .y = (SCREEN_HEIGHT - rectHeight) / 2,
        .speed = 1,
        .direction = DIRECTION_DOWN
    };

    // Track previous position so we can erase only the old rectangle.
    int prevRectX = player.x;
    int prevRectY = player.y;

    // Draw an initial frame so the rectangle is visible immediately.
    clearScreen(RGB5(0, 0, 0));
    drawFilledRect(obstacle.x, obstacle.y, obstacle.width, obstacle.height, RGB5(31, 0, 0));
    drawPlayer(&player, rectWidth, rectHeight, RGB5(31, 31, 31));

    // Basic game loop:
    // 1) wait for VBlank
    // 2) read input
    // 3) update/clamp position with obstacle collision
    // 4) erase old rectangle and draw new one
    while (1) {
        // 1) Synchronize to VBlank so drawing happens between frames.
        VBlankIntrWait();

        // 2) Read the current input state for this frame.
        scanKeys();
        u16 keys = keysHeld();

        // 3) Update movement, bounds, and obstacle collision.
        updatePlayer(&player, keys, rectWidth, rectHeight, &obstacle);

        // 4) Erase the previous rectangle only if it moved.
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

        // Draw the white rectangle at the updated position.
        drawPlayer(&player, rectWidth, rectHeight, RGB5(31, 31, 31));

        // Save position for the next frame's erase step.
        prevRectX = player.x;
        prevRectY = player.y;
    }

    return 0;
}
