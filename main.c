#include <gba_systemcalls.h>
#include <gba_video.h>
#include <gba_interrupt.h>

int main(void)
{
    // Initialize the interrupt system so VBlank waiting works correctly.
    irqInit();
    irqEnable(IRQ_VBLANK);

    // Set the display to Mode 3 with background 2 enabled.
    // Mode 3 is a simple bitmap mode we can clear to a solid color.
    REG_DISPCNT = MODE_3 | BG2_ON;

    // Fill the entire screen with black (a blank screen).
    // 240 * 160 pixels in Mode 3.
    u16 *videoBuffer = (u16 *)MODE3_FB;
    for (int i = 0; i < (240 * 160); i++) {
        videoBuffer[i] = RGB5(0, 0, 0);
    }

    // Basic game loop: wait for each vertical blank.
    while (1) {
        VBlankIntrWait();
    }

    return 0;
}
