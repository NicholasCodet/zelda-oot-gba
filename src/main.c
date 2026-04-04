#include <gba_interrupt.h>
#include <gba_input.h>
#include <gba_systemcalls.h>
#include <gba_video.h>

#include "player.h"
#include "enemy.h"
#include "combat.h"
#include "world.h"
#include "render.h"

typedef enum {
    GAME_STATE_PLAYING = 0,
    GAME_STATE_WIN,
    GAME_STATE_DEAD
} GameState;

// Orient and slightly nudge the player inside the room after a door transition.
// This makes entry position/direction feel consistent with the doorway side.
static void applyDoorEntryPose(Player *player, int playerWidth, int playerHeight)
{
    const int edgeThreshold = 24;
    const int inwardNudge = 3;
    const int maxX = 240 - playerWidth;
    const int maxY = 160 - playerHeight;

    if (player->x <= edgeThreshold) {
        player->x += inwardNudge;
        player->direction = DIRECTION_RIGHT;
    } else if (player->x >= (maxX - edgeThreshold)) {
        player->x -= inwardNudge;
        player->direction = DIRECTION_LEFT;
    } else if (player->y <= edgeThreshold) {
        player->y += inwardNudge;
        player->direction = DIRECTION_DOWN;
    } else if (player->y >= (maxY - edgeThreshold)) {
        player->y -= inwardNudge;
        player->direction = DIRECTION_UP;
    } else {
        player->direction = DIRECTION_DOWN;
    }

    // Clamp once after nudge.
    if (player->x < 0) {
        player->x = 0;
    }
    if (player->x > maxX) {
        player->x = maxX;
    }
    if (player->y < 0) {
        player->y = 0;
    }
    if (player->y > maxY) {
        player->y = maxY;
    }
}

// Reset the run to a clean dungeon start (room 1, full player health,
// default room puzzle states, fresh enemy/attack/render state).
static void resetDungeonRun(
    World *world,
    Player *player,
    Enemy *enemy,
    Attack *attack,
    RenderState *renderState,
    int playerWidth,
    int playerHeight,
    int enemyWidth,
    int enemyHeight,
    int enemyMoveSpeed,
    int attackWidth,
    int attackHeight,
    int attackDuration
)
{
    initWorld(world);

    initPlayer(player, world->playerSpawnX, world->playerSpawnY, 1, 3, 45);

    initEnemy(
        enemy,
        world->enemySpawnX,
        world->enemySpawnY,
        enemyWidth,
        enemyHeight,
        world->enemyMaxHealth,
        world->enemyMoveRange,
        enemyMoveSpeed,
        world->enemyMoveAxis,
        world->enemyType
    );

    initAttack(attack, attackWidth, attackHeight, attackDuration);

    initRenderState(renderState, world, player, enemy, attack, playerWidth, playerHeight);
    drawInitialFrame(world, player, enemy, attack, playerWidth, playerHeight);

    // Initial draw already happened.
    world->requestFullPlayfieldRedraw = 0;
}

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
    Player player;
    Enemy enemy;
    Attack attack;
    RenderState renderState;

    resetDungeonRun(
        &world,
        &player,
        &enemy,
        &attack,
        &renderState,
        playerWidth,
        playerHeight,
        enemyWidth,
        enemyHeight,
        enemyMoveSpeed,
        attackWidth,
        attackHeight,
        attackDuration
    );

    GameState gameState = GAME_STATE_PLAYING;
    int endScreenDrawn = 0;
    int roomEntryStabilizeTimer = 0;

    while (1) {
        VBlankIntrWait();

        scanKeys();
        u16 keys = keysHeld();
        u16 keysPressed = keysDown();

        // Fully separate game-state loop:
        // PLAYING runs gameplay update+render.
        // WIN/DEAD run only end-screen render + restart input.
        switch (gameState) {
            case GAME_STATE_WIN:
            case GAME_STATE_DEAD:
                // Draw static end screen only once on state entry.
                if (!endScreenDrawn) {
                    drawEndStateScreen(gameState == GAME_STATE_WIN);
                    endScreenDrawn = 1;
                }

                if (keysPressed & (KEY_START | KEY_A)) {
                    resetDungeonRun(
                        &world,
                        &player,
                        &enemy,
                        &attack,
                        &renderState,
                        playerWidth,
                        playerHeight,
                        enemyWidth,
                        enemyHeight,
                        enemyMoveSpeed,
                        attackWidth,
                        attackHeight,
                        attackDuration
                    );
                    gameState = GAME_STATE_PLAYING;
                    endScreenDrawn = 0;
                }
                break;

            case GAME_STATE_PLAYING:
            default:
                // Short pause after room swaps to reduce disorientation
                // and prevent immediate accidental re-trigger of a doorway.
                if (roomEntryStabilizeTimer > 0) {
                    roomEntryStabilizeTimer--;
                    renderFrame(&world, &player, &enemy, &attack, playerWidth, playerHeight, &renderState);
                    break;
                }

                tickPlayerInvulnerability(&player);

                updatePlayerMovement(
                    &player,
                    keys,
                    keysPressed,
                    playerWidth,
                    playerHeight,
                    world.roomObstacles,
                    world.roomObstacleCount,
                    world.toggleObstacles,
                    world.interactiveCount,
                    world.lockedDoors,
                    world.lockedDoorCount
                );

                updateEnemyMovement(
                    &enemy,
                    &player,
                    world.roomObstacles,
                    world.roomObstacleCount,
                    world.toggleObstacles,
                    world.interactiveCount
                );

                // Detect enemy death transitions to trigger simple item drops.
                int enemyWasActive = enemy.active;
                // Keep attack rendering stable with dash:
                // when attack starts, stop any ongoing dash movement first.
                if (keysPressed & KEY_B) {
                    player.dashTimer = 0;
                    player.dashX = 0;
                    player.dashY = 0;
                }
                tryStartPlayerAttack(&attack, &player, keysPressed, playerWidth, playerHeight);
                updateCombat(&player, &enemy, &attack, playerWidth, playerHeight);
                if (enemyWasActive && !enemy.active) {
                    trySpawnHeartDrop(&world, enemy.x, enemy.y, enemy.width, enemy.height);
                }
                updateBossRoomGate(&world, enemy.active);

                // Death stops gameplay and switches to the dedicated dead state.
                if (player.isDead) {
                    gameState = GAME_STATE_DEAD;
                    endScreenDrawn = 0;
                    break;
                }

                updateWorldInteractions(&world, &player, keysPressed, playerWidth, playerHeight);
                updateWorldKeyDoor(&world, &player, playerWidth, playerHeight);
                updateWorldHeartPickup(&world, &player, playerWidth, playerHeight);
                updateWorldWinState(&world, &player, playerWidth, playerHeight);

                // Final room goal ends the run and switches to dedicated win state.
                if (world.hasWon && world.currentRoomIndex == (world.roomCount - 1)) {
                    gameState = GAME_STATE_WIN;
                    endScreenDrawn = 0;
                    break;
                }

                {
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
                        shouldTransitionRoom = 1;
                    }

                    if (shouldTransitionRoom) {
                        // Load target room, then place player either at the door target
                        // spawn or at the room's default spawn for goal transitions.
                        loadWorldRoom(&world, transitionRoomIndex);

                        if (useDoorSpawn) {
                            player.x = transitionSpawnX;
                            player.y = transitionSpawnY;
                            applyDoorEntryPose(&player, playerWidth, playerHeight);
                        } else {
                            player.x = world.playerSpawnX;
                            player.y = world.playerSpawnY;
                            player.direction = DIRECTION_DOWN;
                        }

                        player.invulnerabilityTimer = 0;
                        player.knockbackX = 0;
                        player.knockbackY = 0;
                        player.knockbackTimer = 0;
                        roomEntryStabilizeTimer = 6;

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
                            world.enemyMoveAxis,
                            world.enemyType
                        );
                        initAttack(&attack, attackWidth, attackHeight, attackDuration);

                        // Reinitialize render cache and redraw full scene after room swap.
                        initRenderState(&renderState, &world, &player, &enemy, &attack, playerWidth, playerHeight);
                        drawInitialFrame(&world, &player, &enemy, &attack, playerWidth, playerHeight);
                        // Prevent a second full-playfield redraw on the next frame.
                        world.requestFullPlayfieldRedraw = 0;
                        break;
                    }
                }

                // Gameplay render path is called only in PLAYING state.
                renderFrame(&world, &player, &enemy, &attack, playerWidth, playerHeight, &renderState);
                break;
        }
    }

    return 0;
}
