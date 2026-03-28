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

    // Keep Mode 3 bitmap rendering for rooms, and enable OBJ for player sprite.
    REG_DISPCNT = MODE_3 | BG2_ON | OBJ_ON | OBJ_1D_MAP;

    const int playerWidth = 12;
    const int playerHeight = 12;
    const int enemyWidth = 14;
    const int enemyHeight = 14;
    const int enemyMoveSpeed = 1;
    const int attackWidth = 10;
    const int attackHeight = 10;
    const int attackDuration = 6;

    World world;
    initWorld(&world);

    Player player;
    initPlayer(&player, world.playerSpawnX, world.playerSpawnY, 1, 3, 45);

    Enemy enemy;
    initEnemy(
        &enemy,
        world.enemySpawnX,
        world.enemySpawnY,
        enemyWidth,
        enemyHeight,
        world.enemyMaxHealth,
        world.enemyMoveRange,
        enemyMoveSpeed,
        world.enemyMoveAxis
    );

    Attack attack;
    initAttack(&attack, attackWidth, attackHeight, attackDuration);

    RenderState renderState;
    initRenderState(&renderState, &world, &player, &enemy, &attack, playerWidth, playerHeight);
    drawInitialFrame(&world, &player, &enemy, &attack, playerWidth, playerHeight);
    // Initial full scene draw already happened, so no extra full redraw is needed.
    world.requestFullPlayfieldRedraw = 0;

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

        int shouldTransitionRoom = 0;
        int transitionRoomIndex = 0;
        int transitionSpawnX = 0;
        int transitionSpawnY = 0;
        int useDoorSpawn = 0;

        // Door zones provide spatial transitions between connected rooms.
        if (checkDoorZoneTransition(
                &world,
                &player,
                playerWidth,
                playerHeight,
                &transitionRoomIndex,
                &transitionSpawnX,
                &transitionSpawnY
            )) {
            shouldTransitionRoom = 1;
            useDoorSpawn = 1;
        } else if (world.hasWon && world.currentRoomIndex != 1) {
            // Keep existing goal-based progression transitions.
            transitionRoomIndex = world.currentRoomIndex + 1;
            if (transitionRoomIndex >= world.roomCount) {
                transitionRoomIndex = 0;
            }
            shouldTransitionRoom = 1;
        }

        if (shouldTransitionRoom) {
            // Load target room, then place player either at the door target
            // spawn or at the room's default spawn for goal transitions.
            loadWorldRoom(&world, transitionRoomIndex);

            if (useDoorSpawn) {
                player.x = transitionSpawnX;
                player.y = transitionSpawnY;
            } else {
                player.x = world.playerSpawnX;
                player.y = world.playerSpawnY;
            }

            player.direction = DIRECTION_DOWN;
            player.invulnerabilityTimer = 0;
            player.knockbackX = 0;
            player.knockbackY = 0;
            player.knockbackTimer = 0;

            // Reset enemy and attack state for the new room.
            initEnemy(
                &enemy,
                world.enemySpawnX,
                world.enemySpawnY,
                enemyWidth,
                enemyHeight,
                world.enemyMaxHealth,
                world.enemyMoveRange,
                enemyMoveSpeed,
                world.enemyMoveAxis
            );
            initAttack(&attack, attackWidth, attackHeight, attackDuration);

            // Reinitialize render cache and redraw full scene after room swap.
            initRenderState(&renderState, &world, &player, &enemy, &attack, playerWidth, playerHeight);
            drawInitialFrame(&world, &player, &enemy, &attack, playerWidth, playerHeight);
            // Prevent a second full-playfield redraw on the next frame.
            world.requestFullPlayfieldRedraw = 0;
            continue;
        }

        renderFrame(&world, &player, &enemy, &attack, playerWidth, playerHeight, &renderState);
    }

    return 0;
}
