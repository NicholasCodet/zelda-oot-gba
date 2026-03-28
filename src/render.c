#include <gba_video.h>

#include "render.h"

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160

// Shared colors used to improve scene readability.
#define COLOR_BG RGB5(0, 0, 0)
#define COLOR_ROOM_OBSTACLE RGB5(8, 8, 10)
#define COLOR_ROOM_OBSTACLE_BORDER RGB5(14, 14, 16)
#define COLOR_TOGGLE_OBSTACLE RGB5(13, 11, 9)
#define COLOR_TOGGLE_OBSTACLE_BORDER RGB5(20, 18, 14)
#define COLOR_GOAL RGB5(0, 20, 28)
#define COLOR_GOAL_BORDER RGB5(31, 31, 31)
#define COLOR_GOAL_WIN RGB5(0, 28, 10)
#define COLOR_ENEMY RGB5(31, 3, 3)
#define COLOR_ENEMY_BORDER RGB5(31, 28, 0)
#define COLOR_ENEMY_CORE RGB5(20, 0, 0)
#define COLOR_ENEMY_EYE RGB5(31, 31, 0)
#define COLOR_SWITCH_BORDER RGB5(31, 31, 31)
#define COLOR_SWITCH_CORE_ON RGB5(31, 31, 31)
#define COLOR_SWITCH_CORE_OFF RGB5(6, 6, 6)
#define GOAL_VISUAL_PADDING 3

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

// Draw a simple 1-pixel border around a filled rectangle.
static void drawOutlinedRect(int x, int y, int width, int height, u16 fillColor, u16 borderColor)
{
    drawFilledRect(x, y, width, height, fillColor);

    if (width <= 1 || height <= 1) {
        return;
    }

    drawFilledRect(x, y, width, 1, borderColor);
    drawFilledRect(x, y + height - 1, width, 1, borderColor);
    drawFilledRect(x, y, 1, height, borderColor);
    drawFilledRect(x + width - 1, y, 1, height, borderColor);
}

// Goal is drawn larger than its collision box to improve readability.
static GameObject getGoalVisualRect(const World *world)
{
    GameObject visualRect = {
        .x = world->goalArea.x - GOAL_VISUAL_PADDING,
        .y = world->goalArea.y - GOAL_VISUAL_PADDING,
        .width = world->goalArea.width + (GOAL_VISUAL_PADDING * 2),
        .height = world->goalArea.height + (GOAL_VISUAL_PADDING * 2),
        .active = world->goalArea.active
    };

    return visualRect;
}

// Room walls are neutral so they do not compete with gameplay objects.
static void drawRoomObstacle(const GameObject *obstacle)
{
    drawOutlinedRect(
        obstacle->x,
        obstacle->y,
        obstacle->width,
        obstacle->height,
        COLOR_ROOM_OBSTACLE,
        COLOR_ROOM_OBSTACLE_BORDER
    );
}

// Toggle obstacles are warm and bright so blocked paths are obvious.
static void drawToggleObstacle(const GameObject *obstacle)
{
    drawOutlinedRect(
        obstacle->x,
        obstacle->y,
        obstacle->width,
        obstacle->height,
        COLOR_TOGGLE_OBSTACLE,
        COLOR_TOGGLE_OBSTACLE_BORDER
    );

    // Add simple inner stripes so gates read as "blocked barriers".
    if (obstacle->width >= 8) {
        int stripeWidth = 2;
        int leftStripeX = obstacle->x + 3;
        int rightStripeX = obstacle->x + obstacle->width - 5;
        drawFilledRect(leftStripeX, obstacle->y + 1, stripeWidth, obstacle->height - 2, COLOR_TOGGLE_OBSTACLE_BORDER);
        drawFilledRect(rightStripeX, obstacle->y + 1, stripeWidth, obstacle->height - 2, COLOR_TOGGLE_OBSTACLE_BORDER);
    }
}

static void drawInteractiveObject(const World *world, int index)
{
    u16 borderColor = world->interactiveObjects[index].active
        ? world->interactiveOnColor[index]
        : world->interactiveOffColor[index];
    u16 coreColor = world->interactiveObjects[index].active ? COLOR_SWITCH_CORE_ON : COLOR_SWITCH_CORE_OFF;

    // Hollow double outline gives triggers a clear shape distinct from obstacles.
    drawOutlinedRect(
        world->interactiveObjects[index].x,
        world->interactiveObjects[index].y,
        world->interactiveObjects[index].width,
        world->interactiveObjects[index].height,
        COLOR_BG,
        borderColor
    );

    if (world->interactiveObjects[index].width >= 8 && world->interactiveObjects[index].height >= 8) {
        drawOutlinedRect(
            world->interactiveObjects[index].x + 2,
            world->interactiveObjects[index].y + 2,
            world->interactiveObjects[index].width - 4,
            world->interactiveObjects[index].height - 4,
            COLOR_BG,
            COLOR_SWITCH_BORDER
        );
    }

    // ON/OFF state remains visible via center brightness.
    drawFilledRect(
        world->interactiveObjects[index].x + (world->interactiveObjects[index].width / 2) - 2,
        world->interactiveObjects[index].y + (world->interactiveObjects[index].height / 2) - 2,
        4,
        4,
        coreColor
    );
}

static void drawGoalArea(const World *world)
{
    GameObject goalVisualRect = getGoalVisualRect(world);
    u16 goalColor = world->hasWon ? COLOR_GOAL_WIN : COLOR_GOAL;

    // Solid larger platform so the goal is easy to identify from far away.
    drawFilledRect(
        goalVisualRect.x,
        goalVisualRect.y,
        goalVisualRect.width,
        goalVisualRect.height,
        goalColor
    );

    drawOutlinedRect(
        goalVisualRect.x,
        goalVisualRect.y,
        goalVisualRect.width,
        goalVisualRect.height,
        goalColor,
        COLOR_GOAL_BORDER
    );

    // Simple cross mark makes the goal shape recognizable without color.
    drawFilledRect(goalVisualRect.x + (goalVisualRect.width / 2) - 1, goalVisualRect.y + 2, 2, goalVisualRect.height - 4, COLOR_GOAL_BORDER);
    drawFilledRect(goalVisualRect.x + 2, goalVisualRect.y + (goalVisualRect.height / 2) - 1, goalVisualRect.width - 4, 2, COLOR_GOAL_BORDER);
}

static void drawEnemyRect(const Enemy *enemy)
{
    drawOutlinedRect(
        enemy->x,
        enemy->y,
        enemy->width,
        enemy->height,
        COLOR_ENEMY,
        COLOR_ENEMY_BORDER
    );

    // Add a darker center and two bright "eyes" for quick danger recognition.
    if (enemy->width >= 8 && enemy->height >= 8) {
        drawFilledRect(enemy->x + 2, enemy->y + 2, enemy->width - 4, enemy->height - 4, COLOR_ENEMY_CORE);
        drawFilledRect(enemy->x + 3, enemy->y + 3, 2, 2, COLOR_ENEMY_EYE);
        drawFilledRect(enemy->x + enemy->width - 5, enemy->y + 3, 2, 2, COLOR_ENEMY_EYE);
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
    drawFilledRect(region->x, region->y, region->width, region->height, COLOR_BG);

    for (int i = 0; i < world->roomObstacleCount; i++) {
        if (world->roomObstacles[i].active && isCollidingAABB(region, &world->roomObstacles[i])) {
            drawRoomObstacle(&world->roomObstacles[i]);
        }
    }

    for (int i = 0; i < world->interactiveCount; i++) {
        if (world->toggleObstacles[i].active && isCollidingAABB(region, &world->toggleObstacles[i])) {
            drawToggleObstacle(&world->toggleObstacles[i]);
        }
    }

    for (int i = 0; i < world->interactiveCount; i++) {
        if (isCollidingAABB(region, &world->interactiveObjects[i])) {
            drawInteractiveObject(world, i);
        }
    }

    GameObject goalVisualRect = getGoalVisualRect(world);
    if (world->goalArea.active && isCollidingAABB(region, &goalVisualRect)) {
        drawGoalArea(world);
    }

    if (enemy->active) {
        GameObject enemyRect = getEnemyRect(enemy);
        if (isCollidingAABB(region, &enemyRect)) {
            drawEnemyRect(enemy);
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
        drawEnemyRect(enemy);
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
        GameObject goalVisualRect = getGoalVisualRect(world);
        redrawSceneRegion(&goalVisualRect, world, enemy);
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
