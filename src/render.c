#include <gba_video.h>

#include "render.h"

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160

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

    if (startX >= endX || startY >= endY) {
        return;
    }

    for (int drawY = startY; drawY < endY; drawY++) {
        for (int drawX = startX; drawX < endX; drawX++) {
            videoBuffer[drawY * SCREEN_WIDTH + drawX] = color;
        }
    }
}

static void drawPlayerRect(const Player *player, int rectWidth, int rectHeight, u16 color)
{
    drawFilledRect(player->x, player->y, rectWidth, rectHeight, color);
}

// Draw a tiny health display in the top-left corner.
static void drawPlayerHealthUI(int health, int maxHealth)
{
    const int uiX = 4;
    const int uiY = 4;
    const int segmentWidth = 6;
    const int segmentHeight = 4;
    const int segmentGap = 2;

    int uiWidth = (maxHealth * (segmentWidth + segmentGap)) - segmentGap + 4;
    int uiHeight = segmentHeight + 4;
    drawFilledRect(uiX - 2, uiY - 2, uiWidth, uiHeight, RGB5(0, 0, 0));

    for (int i = 0; i < maxHealth; i++) {
        int segmentX = uiX + (i * (segmentWidth + segmentGap));
        u16 segmentColor = (i < health) ? RGB5(0, 31, 0) : RGB5(8, 8, 8);
        drawFilledRect(segmentX, uiY, segmentWidth, segmentHeight, segmentColor);
    }
}

// Redraw only one region of the scene.
static void redrawSceneRegion(const GameObject *region, const World *world, const Enemy *enemy)
{
    drawFilledRect(region->x, region->y, region->width, region->height, RGB5(0, 0, 0));

    for (int i = 0; i < world->roomObstacleCount; i++) {
        if (world->roomObstacles[i].active && isCollidingAABB(region, &world->roomObstacles[i])) {
            drawFilledRect(
                world->roomObstacles[i].x,
                world->roomObstacles[i].y,
                world->roomObstacles[i].width,
                world->roomObstacles[i].height,
                RGB5(31, 0, 0)
            );
        }
    }

    for (int i = 0; i < world->interactiveCount; i++) {
        if (world->toggleObstacles[i].active && isCollidingAABB(region, &world->toggleObstacles[i])) {
            drawFilledRect(
                world->toggleObstacles[i].x,
                world->toggleObstacles[i].y,
                world->toggleObstacles[i].width,
                world->toggleObstacles[i].height,
                RGB5(31, 0, 0)
            );
        }
    }

    for (int i = 0; i < world->interactiveCount; i++) {
        if (isCollidingAABB(region, &world->interactiveObjects[i])) {
            drawFilledRect(
                world->interactiveObjects[i].x,
                world->interactiveObjects[i].y,
                world->interactiveObjects[i].width,
                world->interactiveObjects[i].height,
                world->interactiveObjects[i].active ? world->interactiveOnColor[i] : world->interactiveOffColor[i]
            );
        }
    }

    if (world->goalArea.active && isCollidingAABB(region, &world->goalArea)) {
        if (world->hasWon) {
            drawFilledRect(world->goalArea.x, world->goalArea.y, world->goalArea.width, world->goalArea.height, RGB5(31, 31, 31));
        } else {
            drawFilledRect(world->goalArea.x, world->goalArea.y, world->goalArea.width, world->goalArea.height, RGB5(31, 0, 31));
        }
    }

    if (enemy->active) {
        GameObject enemyRect = getEnemyRect(enemy);
        if (isCollidingAABB(region, &enemyRect)) {
            drawFilledRect(enemy->x, enemy->y, enemy->width, enemy->height, RGB5(31, 16, 0));
        }
    }
}

static void drawDynamicObjects(
    const Player *player,
    const Enemy *enemy,
    const Attack *attack,
    int playerWidth,
    int playerHeight
)
{
    if (enemy->active) {
        drawFilledRect(enemy->x, enemy->y, enemy->width, enemy->height, RGB5(31, 16, 0));
    }

    if (attack->active) {
        drawFilledRect(attack->x, attack->y, attack->width, attack->height, RGB5(31, 31, 0));
    }

    if (player->health > 0) {
        u16 playerColor = RGB5(31, 31, 31);

        // Flash red while temporary invulnerability is active.
        if (player->invulnerabilityTimer > 0) {
            if (((player->invulnerabilityTimer / 4) & 1) == 0) {
                playerColor = RGB5(31, 0, 0);
            }
        }

        drawPlayerRect(player, playerWidth, playerHeight, playerColor);
    }

    drawPlayerHealthUI(player->health, player->maxHealth);

    // Keep the existing simple death indicator.
    if (player->health <= 0) {
        drawFilledRect(84, 70, 72, 20, RGB5(20, 0, 0));
    }
}

void initRenderState(
    RenderState *state,
    const World *world,
    const Player *player,
    const Enemy *enemy,
    const Attack *attack,
    int playerWidth,
    int playerHeight
)
{
    state->prevPlayerRect = getPlayerRect(player, playerWidth, playerHeight);
    state->prevAttackRect = getAttackRect(attack);
    state->prevAttackWasActive = attack->active;
    state->prevEnemyRect = getEnemyRect(enemy);
    state->prevEnemyWasActive = enemy->active;

    state->prevHasWon = world->hasWon;
    for (int i = 0; i < world->interactiveCount; i++) {
        state->prevInteractiveState[i] = world->interactiveObjects[i].active;
        state->prevToggleState[i] = world->toggleObstacles[i].active;
    }
}

void drawInitialFrame(
    const World *world,
    const Player *player,
    const Enemy *enemy,
    const Attack *attack,
    int playerWidth,
    int playerHeight
)
{
    GameObject fullScreenRegion = {
        .x = 0,
        .y = 0,
        .width = SCREEN_WIDTH,
        .height = SCREEN_HEIGHT,
        .active = 1
    };

    redrawSceneRegion(&fullScreenRegion, world, enemy);
    drawDynamicObjects(player, enemy, attack, playerWidth, playerHeight);
}

void renderFrame(
    const World *world,
    const Player *player,
    const Enemy *enemy,
    const Attack *attack,
    int playerWidth,
    int playerHeight,
    RenderState *state
)
{
    redrawSceneRegion(&state->prevPlayerRect, world, enemy);

    if (state->prevAttackWasActive) {
        redrawSceneRegion(&state->prevAttackRect, world, enemy);
    }

    if (state->prevEnemyWasActive) {
        redrawSceneRegion(&state->prevEnemyRect, world, enemy);
    }

    for (int i = 0; i < world->interactiveCount; i++) {
        if (state->prevInteractiveState[i] != world->interactiveObjects[i].active) {
            redrawSceneRegion(&world->interactiveObjects[i], world, enemy);
        }
        if (state->prevToggleState[i] != world->toggleObstacles[i].active) {
            redrawSceneRegion(&world->toggleObstacles[i], world, enemy);
        }
    }

    if (state->prevHasWon != world->hasWon) {
        redrawSceneRegion(&world->goalArea, world, enemy);
    }

    drawDynamicObjects(player, enemy, attack, playerWidth, playerHeight);

    state->prevPlayerRect = getPlayerRect(player, playerWidth, playerHeight);

    state->prevAttackRect = getAttackRect(attack);
    state->prevAttackWasActive = attack->active;

    state->prevEnemyRect = getEnemyRect(enemy);
    state->prevEnemyWasActive = enemy->active;

    state->prevHasWon = world->hasWon;
    for (int i = 0; i < world->interactiveCount; i++) {
        state->prevInteractiveState[i] = world->interactiveObjects[i].active;
        state->prevToggleState[i] = world->toggleObstacles[i].active;
    }
}
