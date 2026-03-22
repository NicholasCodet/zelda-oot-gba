#include <gba_interrupt.h>
#include <gba_input.h>
#include <gba_systemcalls.h>
#include <gba_video.h>

#include "player.h"
#include "enemy.h"
#include "combat.h"
#include "world.h"
#include "render.h"

int main(void)
{
    irqInit();
    irqEnable(IRQ_VBLANK);

    // Keep Mode 3 bitmap rendering with BG2 enabled.
    REG_DISPCNT = MODE_3 | BG2_ON;

    const int playerWidth = 12;
    const int playerHeight = 12;

    World world;
    initWorld(&world);

    Player player;
    initPlayer(&player, 40, 40, 1, 3, 45);

    Enemy enemy;
    initEnemy(&enemy, 116, 40, 14, 14, 2, 24, 1);

    Attack attack;
    initAttack(&attack, 10, 10, 6);

    RenderState renderState;
    initRenderState(&renderState, &world, &player, &enemy, &attack, playerWidth, playerHeight);
    drawInitialFrame(&world, &player, &enemy, &attack, playerWidth, playerHeight);

    while (1) {
        VBlankIntrWait();

        scanKeys();
        u16 keys = keysHeld();
        u16 keysPressed = keysDown();

        tickPlayerInvulnerability(&player);

        updatePlayerMovement(
            &player,
            keys,
            playerWidth,
            playerHeight,
            world.roomObstacles,
            world.roomObstacleCount,
            world.toggleObstacles,
            world.interactiveCount
        );

        updateEnemyMovement(
            &enemy,
            world.roomObstacles,
            world.roomObstacleCount,
            world.toggleObstacles,
            world.interactiveCount
        );

        tryStartPlayerAttack(&attack, &player, keysPressed, playerWidth, playerHeight);
        updateCombat(&player, &enemy, &attack, playerWidth, playerHeight);

        updateWorldInteractions(&world, &player, keysPressed, playerWidth, playerHeight);
        updateWorldWinState(&world, &player, playerWidth, playerHeight);

        renderFrame(&world, &player, &enemy, &attack, playerWidth, playerHeight, &renderState);
    }

    return 0;
}
