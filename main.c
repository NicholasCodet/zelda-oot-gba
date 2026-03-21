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

// Update player position from input and keep it inside screen bounds.
static void updatePlayer(Player *player, u16 keys, int rectWidth, int rectHeight)
{
    // Move one pixel per frame while a direction is held.
    if (keys & KEY_UP) {
        player->y -= player->speed;
        player->direction = DIRECTION_UP;
    }
    if (keys & KEY_DOWN) {
        player->y += player->speed;
        player->direction = DIRECTION_DOWN;
    }
    if (keys & KEY_LEFT) {
        player->x -= player->speed;
        player->direction = DIRECTION_LEFT;
    }
    if (keys & KEY_RIGHT) {
        player->x += player->speed;
        player->direction = DIRECTION_RIGHT;
    }

    // Keep the rectangle inside the visible screen bounds.
    if (player->x < 0) {
        player->x = 0;
    }
    if (player->y < 0) {
        player->y = 0;
    }
    if (player->x > (SCREEN_WIDTH - rectWidth)) {
        player->x = SCREEN_WIDTH - rectWidth;
    }
    if (player->y > (SCREEN_HEIGHT - rectHeight)) {
        player->y = SCREEN_HEIGHT - rectHeight;
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
    drawPlayer(&player, rectWidth, rectHeight, RGB5(31, 31, 31));

    // Basic game loop:
    // 1) wait for VBlank
    // 2) read input
    // 3) update/clamp position
    // 4) erase old rectangle and draw new one
    while (1) {
        // 1) Synchronize to VBlank so drawing happens between frames.
        VBlankIntrWait();

        // 2) Read the current input state for this frame.
        scanKeys();
        u16 keys = keysHeld();

        // 3) Update movement and bounds.
        updatePlayer(&player, keys, rectWidth, rectHeight);

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
