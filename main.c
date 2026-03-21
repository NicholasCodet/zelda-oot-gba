#include <gba_systemcalls.h>
#include <gba_video.h>
#include <gba_interrupt.h>

// GBA screen size in Mode 3.
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160

// Clear the full Mode 3 framebuffer to a single color.
static void clearScreen(u16 color)
{
    u16 *videoBuffer = (u16 *)MODE3_FB;
    for (int i = 0; i < (SCREEN_WIDTH * SCREEN_HEIGHT); i++) {
        videoBuffer[i] = color;
    }
}

// Draw a filled rectangle in Mode 3.
static void drawFilledRect(int x, int y, int width, int height, u16 color)
{
    u16 *videoBuffer = (u16 *)MODE3_FB;

    for (int row = 0; row < height; row++) {
        int drawY = y + row;
        for (int col = 0; col < width; col++) {
            int drawX = x + col;
            videoBuffer[drawY * SCREEN_WIDTH + drawX] = color;
        }
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

    // Clear the entire screen to black.
    clearScreen(RGB5(0, 0, 0));

    // Rectangle size used for all three test markers.
    const int rectWidth = 20;
    const int rectHeight = 12;

    // Keep all markers on the same horizontal row.
    const int rectY = (SCREEN_HEIGHT - rectHeight) / 2;

    // Left marker (red).
    const int leftRectX = 40;
    drawFilledRect(leftRectX, rectY, rectWidth, rectHeight, RGB5(31, 0, 0));

    // Center marker (white).
    const int centerRectX = (SCREEN_WIDTH - rectWidth) / 2;
    drawFilledRect(centerRectX, rectY, rectWidth, rectHeight, RGB5(31, 31, 31));

    // Right marker (green).
    const int rightRectX = SCREEN_WIDTH - rectWidth - 40;
    drawFilledRect(rightRectX, rectY, rectWidth, rectHeight, RGB5(0, 31, 0));

    // Basic game loop: wait for each vertical blank.
    while (1) {
        VBlankIntrWait();
    }

    return 0;
}
