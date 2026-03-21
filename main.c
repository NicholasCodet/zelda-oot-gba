#include <gba_systemcalls.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_input.h>

// GBA screen size in Mode 3.
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160

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

int main(void)
{
    // Initialize the interrupt system so VBlank waiting works correctly.
    irqInit();
    irqEnable(IRQ_VBLANK);

    // Set the display to Mode 3 with background 2 enabled.
    // Mode 3 is a simple bitmap mode we can clear to a solid color.
    REG_DISPCNT = MODE_3 | BG2_ON;

    // Player test rectangle size and start position (center of screen).
    const int rectWidth = 12;
    const int rectHeight = 12;
    int rectX = (SCREEN_WIDTH - rectWidth) / 2;
    int rectY = (SCREEN_HEIGHT - rectHeight) / 2;

    // Draw an initial frame so the rectangle is visible immediately.
    clearScreen(RGB5(0, 0, 0));
    drawFilledRect(rectX, rectY, rectWidth, rectHeight, RGB5(31, 31, 31));

    // Basic game loop:
    // 1) read input
    // 2) update position
    // 3) clear screen
    // 4) draw rectangle
    // 5) wait for VBlank
    while (1) {
        // 1) Read the current input state for this frame.
        scanKeys();
        u16 keys = keysHeld();

        // 2) Move one pixel per frame while a direction is held.
        if (keys & KEY_UP) {
            rectY--;
        }
        if (keys & KEY_DOWN) {
            rectY++;
        }
        if (keys & KEY_LEFT) {
            rectX--;
        }
        if (keys & KEY_RIGHT) {
            rectX++;
        }

        // Keep the rectangle inside the visible screen bounds.
        if (rectX < 0) {
            rectX = 0;
        }
        if (rectY < 0) {
            rectY = 0;
        }
        if (rectX > (SCREEN_WIDTH - rectWidth)) {
            rectX = SCREEN_WIDTH - rectWidth;
        }
        if (rectY > (SCREEN_HEIGHT - rectHeight)) {
            rectY = SCREEN_HEIGHT - rectHeight;
        }

        // 3) Clear background to black.
        clearScreen(RGB5(0, 0, 0));

        // 4) Draw the white rectangle.
        drawFilledRect(rectX, rectY, rectWidth, rectHeight, RGB5(31, 31, 31));

        // 5) Wait for VBlank before the next frame.
        VBlankIntrWait();
    }

    return 0;
}
