#include <gba_video.h>
#include <gba_sprites.h>

#include "render.h"

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160
#define HUD_X 4
#define HUD_Y 4
#define HUD_SEGMENT_WIDTH 6
#define HUD_SEGMENT_HEIGHT 4
#define HUD_SEGMENT_GAP 2
#define HUD_AREA_HEIGHT 14
#define PLAYFIELD_TOP HUD_AREA_HEIGHT
#define PLAYFIELD_HEIGHT (SCREEN_HEIGHT - PLAYFIELD_TOP)
#define PLAYER_SPRITE_OAM_INDEX 0
#define PLAYER_SPRITE_TILE_BASE 512
#define PLAYER_SPRITE_TILE_WHITE (PLAYER_SPRITE_TILE_BASE + 0)
#define PLAYER_SPRITE_TILE_RED (PLAYER_SPRITE_TILE_BASE + 4)

// Shared colors used to improve scene readability.
#define COLOR_BG RGB5(0, 0, 0)
#define COLOR_ROOM_OBSTACLE RGB5(8, 8, 10)
#define COLOR_ROOM_OBSTACLE_BORDER RGB5(14, 14, 16)
#define COLOR_TOGGLE_OBSTACLE RGB5(13, 11, 9)
#define COLOR_TOGGLE_OBSTACLE_BORDER RGB5(20, 18, 14)
#define COLOR_LOCKED_DOOR RGB5(14, 8, 3)
#define COLOR_LOCKED_DOOR_BORDER RGB5(24, 18, 8)
#define COLOR_LOCKED_DOOR_LOCK RGB5(31, 31, 0)
#define COLOR_KEY RGB5(31, 31, 0)
#define COLOR_KEY_BORDER RGB5(31, 20, 0)
#define COLOR_GOAL RGB5(0, 20, 28)
#define COLOR_GOAL_BORDER RGB5(31, 31, 31)
#define COLOR_GOAL_WIN RGB5(0, 28, 10)
#define COLOR_ENEMY RGB5(31, 3, 3)
#define COLOR_ENEMY_BORDER RGB5(31, 28, 0)
#define COLOR_ENEMY_CORE RGB5(20, 0, 0)
#define COLOR_ENEMY_EYE RGB5(31, 31, 0)
#define COLOR_ENEMY_FLASH RGB5(31, 31, 31)
#define COLOR_ENEMY_FLASH_BORDER RGB5(31, 16, 0)
#define COLOR_ENEMY_FLASH_CORE RGB5(31, 20, 0)
#define COLOR_ENEMY_FLASH_EYE RGB5(31, 0, 0)
#define COLOR_SWITCH_BORDER RGB5(31, 31, 31)
#define COLOR_SWITCH_CORE_ON RGB5(31, 31, 31)
#define COLOR_SWITCH_CORE_OFF RGB5(6, 6, 6)
#define COLOR_SWITCH_FILL_ON RGB5(0, 9, 12)
#define GOAL_VISUAL_PADDING 3
#define COLOR_END_WIN_BG RGB5(4, 8, 16)
#define COLOR_END_LOSE_BG RGB5(12, 0, 0)
#define COLOR_END_INDICATOR_FILL RGB5(20, 20, 20)
#define COLOR_END_INDICATOR_BORDER RGB5(31, 31, 31)

// Player sprite setup state.
static int gPlayerSpriteReady = 0;

// Full-screen clear used by end-state screens.
// This path is independent from gameplay redraw helpers.
static void clearFullScreen(u16 color)
{
    volatile u16 *videoBuffer = (volatile u16 *)MODE3_FB;

    for (int i = 0; i < (SCREEN_WIDTH * SCREEN_HEIGHT); i++) {
        videoBuffer[i] = color;
    }
}

// Draw a centered indicator block so win/lose screens are visually clear.
static void drawCenteredIndicator(u16 fillColor, u16 borderColor)
{
    volatile u16 *videoBuffer = (volatile u16 *)MODE3_FB;
    const int boxWidth = 96;
    const int boxHeight = 40;
    const int startX = (SCREEN_WIDTH - boxWidth) / 2;
    const int startY = (SCREEN_HEIGHT - boxHeight) / 2;
    const int endX = startX + boxWidth;
    const int endY = startY + boxHeight;

    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            int isBorder = (x == startX) || (x == endX - 1) || (y == startY) || (y == endY - 1);
            videoBuffer[y * SCREEN_WIDTH + x] = isBorder ? borderColor : fillColor;
        }
    }
}

// Set one pixel inside a 16x16 4bpp sprite tile set (2x2 tiles).
static void setPlayerSpritePixel(u32 *tileWords, int x, int y, u32 paletteIndex)
{
    int tileIndex = (y / 8) * 2 + (x / 8);
    int localX = x & 7;
    int localY = y & 7;
    u32 *word = &tileWords[(tileIndex * 8) + localY];
    u32 shift = (u32)(localX * 4);

    *word = (*word & ~(0xFu << shift)) | ((paletteIndex & 0xF) << shift);
}

// Build one 16x16 sprite variant for the player (top-left aligned 12x12 block).
static void buildPlayerSpriteTiles(u32 *tileWords, u32 paletteIndex)
{
    for (int i = 0; i < 32; i++) {
        tileWords[i] = 0;
    }

    for (int y = 0; y < 12; y++) {
        for (int x = 0; x < 12; x++) {
            setPlayerSpritePixel(tileWords, x, y, paletteIndex);
        }
    }
}

// Upload player sprite graphics and configure one OAM entry.
static void initPlayerSprite(void)
{
    if (gPlayerSpriteReady) {
        return;
    }

    u32 whiteTiles[32];
    u32 redTiles[32];
    volatile u32 *objTileMemory = (volatile u32 *)BITMAP_OBJ_BASE_ADR;

    buildPlayerSpriteTiles(whiteTiles, 1);
    buildPlayerSpriteTiles(redTiles, 2);

    for (int i = 0; i < 32; i++) {
        objTileMemory[i] = whiteTiles[i];
        objTileMemory[32 + i] = redTiles[i];
    }

    // Sprite palette:
    // 0 = transparent, 1 = player normal, 2 = player damage flash.
    SPRITE_PALETTE[0] = RGB5(0, 0, 0);
    SPRITE_PALETTE[1] = RGB5(31, 31, 31);
    SPRITE_PALETTE[2] = RGB5(31, 0, 0);

    // Hide all sprite entries by default for a known starting state.
    for (int i = 0; i < 128; i++) {
        OAM[i].attr0 = ATTR0_DISABLED;
        OAM[i].attr1 = 0;
        OAM[i].attr2 = 0;
    }

    gPlayerSpriteReady = 1;
}

// Update player sprite position/visibility each frame.
static void updatePlayerSprite(const Player *player)
{
    if (!gPlayerSpriteReady) {
        return;
    }

    // Keep existing death behavior: hide player when dead.
    if (player->health <= 0 || player->isDead) {
        OAM[PLAYER_SPRITE_OAM_INDEX].attr0 = ATTR0_DISABLED;
        OAM[PLAYER_SPRITE_OAM_INDEX].attr1 = 0;
        OAM[PLAYER_SPRITE_OAM_INDEX].attr2 = 0;
        return;
    }

    u16 tileIndex = PLAYER_SPRITE_TILE_WHITE;

    // Keep invulnerability flash feedback by switching sprite graphics.
    if (player->invulnerabilityTimer > 0) {
        if (((player->invulnerabilityTimer / 4) & 1) == 0) {
            tileIndex = PLAYER_SPRITE_TILE_RED;
        }
    }

    OAM[PLAYER_SPRITE_OAM_INDEX].attr0 = OBJ_Y(player->y) | ATTR0_COLOR_16 | ATTR0_SQUARE;
    OAM[PLAYER_SPRITE_OAM_INDEX].attr1 = OBJ_X(player->x) | ATTR1_SIZE_16;
    OAM[PLAYER_SPRITE_OAM_INDEX].attr2 = OBJ_CHAR(tileIndex) | ATTR2_PRIORITY(0) | ATTR2_PALETTE(0);
}

// Draw a filled rectangle in Mode 3 with a top clip.
// `minY` lets us reserve the HUD band and keep world rendering below it.
static void drawFilledRectClipped(int x, int y, int width, int height, u16 color, int minY)
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
    if (startY < minY) {
        startY = minY;
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

// Generic fill (no HUD clipping).
static void drawFilledRect(int x, int y, int width, int height, u16 color)
{
    drawFilledRectClipped(x, y, width, height, color, 0);
}

// Playfield fill: never draw inside the reserved HUD area.
static void drawPlayfieldRect(int x, int y, int width, int height, u16 color)
{
    drawFilledRectClipped(x, y, width, height, color, PLAYFIELD_TOP);
}

// Draw a simple 1-pixel border around a filled rectangle.
static void drawOutlinedRect(int x, int y, int width, int height, u16 fillColor, u16 borderColor)
{
    drawPlayfieldRect(x, y, width, height, fillColor);

    if (width <= 1 || height <= 1) {
        return;
    }

    drawPlayfieldRect(x, y, width, 1, borderColor);
    drawPlayfieldRect(x, y + height - 1, width, 1, borderColor);
    drawPlayfieldRect(x, y, 1, height, borderColor);
    drawPlayfieldRect(x + width - 1, y, 1, height, borderColor);
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

// Key is a compact bright marker to remain easy to identify.
static void drawKeyObject(const GameObject *keyObject)
{
    drawOutlinedRect(
        keyObject->x,
        keyObject->y,
        keyObject->width,
        keyObject->height,
        COLOR_KEY,
        COLOR_KEY_BORDER
    );

    if (keyObject->width >= 6 && keyObject->height >= 6) {
        drawPlayfieldRect(keyObject->x + 2, keyObject->y + 2, 2, 2, COLOR_BG);
    }
}

// Locked door has a warmer block shape with a bright "lock" marker.
static void drawLockedDoorObject(const GameObject *doorObject)
{
    drawOutlinedRect(
        doorObject->x,
        doorObject->y,
        doorObject->width,
        doorObject->height,
        COLOR_LOCKED_DOOR,
        COLOR_LOCKED_DOOR_BORDER
    );

    if (doorObject->width >= 4 && doorObject->height >= 6) {
        int lockX = doorObject->x + (doorObject->width / 2) - 1;
        int lockY = doorObject->y + (doorObject->height / 2) - 2;
        drawPlayfieldRect(lockX, lockY, 2, 4, COLOR_LOCKED_DOOR_LOCK);
    }
}

static void drawInteractiveObject(const World *world, int index)
{
    int isActive = world->interactiveObjects[index].active;
    u16 borderColor = isActive
        ? world->interactiveOnColor[index]
        : world->interactiveOffColor[index];
    u16 fillColor = isActive ? COLOR_SWITCH_FILL_ON : COLOR_BG;

    // Triggers are drawn with a distinctive shape and clear ON/OFF fill change.
    drawOutlinedRect(
        world->interactiveObjects[index].x,
        world->interactiveObjects[index].y,
        world->interactiveObjects[index].width,
        world->interactiveObjects[index].height,
        fillColor,
        borderColor
    );

    int centerX = world->interactiveObjects[index].x + (world->interactiveObjects[index].width / 2);
    int centerY = world->interactiveObjects[index].y + (world->interactiveObjects[index].height / 2);

    if (isActive) {
        // ON: brighter center block.
        drawFilledRect(centerX - 3, centerY - 3, 6, 6, COLOR_SWITCH_CORE_ON);
    } else {
        // OFF: hollow look with small dark center.
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
        drawFilledRect(centerX - 2, centerY - 2, 4, 4, COLOR_SWITCH_CORE_OFF);
    }
}

static void drawGoalArea(const World *world)
{
    GameObject goalVisualRect = getGoalVisualRect(world);
    u16 goalColor = world->hasWon ? COLOR_GOAL_WIN : COLOR_GOAL;

    // Solid larger platform so the goal is easy to identify from far away.
    drawPlayfieldRect(
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
    drawPlayfieldRect(goalVisualRect.x + (goalVisualRect.width / 2) - 1, goalVisualRect.y + 2, 2, goalVisualRect.height - 4, COLOR_GOAL_BORDER);
    drawPlayfieldRect(goalVisualRect.x + 2, goalVisualRect.y + (goalVisualRect.height / 2) - 1, goalVisualRect.width - 4, 2, COLOR_GOAL_BORDER);
}

static void drawEnemyRect(const Enemy *enemy)
{
    int enemyFlashOn = (enemy->hitFlashTimer > 0) && (((enemy->hitFlashTimer / 2) & 1) == 0);
    u16 enemyBodyColor = enemyFlashOn ? COLOR_ENEMY_FLASH : COLOR_ENEMY;
    u16 enemyBorderColor = enemyFlashOn ? COLOR_ENEMY_FLASH_BORDER : COLOR_ENEMY_BORDER;
    u16 enemyCoreColor = enemyFlashOn ? COLOR_ENEMY_FLASH_CORE : COLOR_ENEMY_CORE;
    u16 enemyEyeColor = enemyFlashOn ? COLOR_ENEMY_FLASH_EYE : COLOR_ENEMY_EYE;

    drawOutlinedRect(
        enemy->x,
        enemy->y,
        enemy->width,
        enemy->height,
        enemyBodyColor,
        enemyBorderColor
    );

    // Add a darker center and two bright "eyes" for quick danger recognition.
    if (enemy->width >= 8 && enemy->height >= 8) {
        drawFilledRect(enemy->x + 2, enemy->y + 2, enemy->width - 4, enemy->height - 4, enemyCoreColor);
        drawFilledRect(enemy->x + 3, enemy->y + 3, 2, 2, enemyEyeColor);
        drawFilledRect(enemy->x + enemy->width - 5, enemy->y + 3, 2, 2, enemyEyeColor);
    }
}

// Draw a tiny HUD strip with health and key count in the top-left area.
static void drawPlayerHealthUI(int health, int maxHealth, int keyCount)
{
    // Clear the full reserved HUD strip every frame for a clean overlay.
    drawFilledRect(0, 0, SCREEN_WIDTH, HUD_AREA_HEIGHT, COLOR_BG);

    for (int i = 0; i < maxHealth; i++) {
        int segmentX = HUD_X + (i * (HUD_SEGMENT_WIDTH + HUD_SEGMENT_GAP));
        u16 segmentColor = (i < health) ? RGB5(0, 31, 0) : RGB5(8, 8, 8);
        drawFilledRect(segmentX, HUD_Y, HUD_SEGMENT_WIDTH, HUD_SEGMENT_HEIGHT, segmentColor);
    }

    // Draw simple yellow key squares near the right side of the HUD.
    // Limit the drawn amount to keep the HUD compact and stable.
    int clampedKeyCount = keyCount;
    if (clampedKeyCount > 8) {
        clampedKeyCount = 8;
    }

    for (int i = 0; i < clampedKeyCount; i++) {
        int keyX = 186 + (i * 6);
        drawFilledRect(keyX, HUD_Y, 4, 4, COLOR_KEY);
        drawFilledRect(keyX, HUD_Y, 4, 1, COLOR_KEY_BORDER);
        drawFilledRect(keyX, HUD_Y + 3, 4, 1, COLOR_KEY_BORDER);
        drawFilledRect(keyX, HUD_Y, 1, 4, COLOR_KEY_BORDER);
        drawFilledRect(keyX + 3, HUD_Y, 1, 4, COLOR_KEY_BORDER);
    }
}

// Redraw only one region of the scene.
static void redrawSceneRegion(const GameObject *region, const World *world, const Enemy *enemy)
{
    drawPlayfieldRect(region->x, region->y, region->width, region->height, COLOR_BG);

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

    for (int i = 0; i < world->lockedDoorCount; i++) {
        if (world->lockedDoors[i].active && isCollidingAABB(region, &world->lockedDoors[i])) {
            drawLockedDoorObject(&world->lockedDoors[i]);
        }
    }

    if (world->keyObject.active && isCollidingAABB(region, &world->keyObject)) {
        drawKeyObject(&world->keyObject);
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
    const Attack *attack
)
{
    if (enemy->active) {
        drawEnemyRect(enemy);
    }

    if (attack->active) {
        drawPlayfieldRect(attack->x, attack->y, attack->width, attack->height, RGB5(31, 31, 0));
    }

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
    initPlayerSprite();
    updatePlayerSprite(player);

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
    (void)playerWidth;
    (void)playerHeight;

    GameObject fullPlayfieldRegion = {
        .x = 0,
        .y = PLAYFIELD_TOP,
        .width = SCREEN_WIDTH,
        .height = PLAYFIELD_HEIGHT,
        .active = 1
    };

    // Draw HUD first so top scanlines are updated early.
    drawPlayerHealthUI(player->health, player->maxHealth, world->keyCount);
    redrawSceneRegion(&fullPlayfieldRegion, world, enemy);
    drawDynamicObjects(player, enemy, attack);
}

void renderFrame(
    World *world,
    const Player *player,
    const Enemy *enemy,
    const Attack *attack,
    int playerWidth,
    int playerHeight,
    RenderState *state
)
{
    // Update player sprite first each frame (separate from bitmap redraw path).
    updatePlayerSprite(player);

    // Draw HUD first in the frame.
    // In Mode 3, scanout starts at the top, so early HUD updates reduce top-line artifacts.
    drawPlayerHealthUI(player->health, player->maxHealth, world->keyCount);

    if (world->requestFullPlayfieldRedraw) {
        // World state changed (switch/obstacle/goal/etc): redraw the full
        // playfield once to avoid dirty-rect edge cases and partial artifacts.
        GameObject fullPlayfieldRegion = {
            .x = 0,
            .y = PLAYFIELD_TOP,
            .width = SCREEN_WIDTH,
            .height = PLAYFIELD_HEIGHT,
            .active = 1
        };

        redrawSceneRegion(&fullPlayfieldRegion, world, enemy);
        world->requestFullPlayfieldRedraw = 0;
    } else {
        // Normal frame path: use incremental redraw for previous dynamic regions.
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
    }

    drawDynamicObjects(player, enemy, attack);

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

void drawEndStateScreen(int didWin)
{
    // Hide the player sprite while showing a static end-state screen.
    if (gPlayerSpriteReady) {
        OAM[PLAYER_SPRITE_OAM_INDEX].attr0 = ATTR0_DISABLED;
        OAM[PLAYER_SPRITE_OAM_INDEX].attr1 = 0;
        OAM[PLAYER_SPRITE_OAM_INDEX].attr2 = 0;
    }

    // Full redraw only: no partial/incremental world path used here.
    if (didWin) {
        clearFullScreen(COLOR_END_WIN_BG);
    } else {
        clearFullScreen(COLOR_END_LOSE_BG);
    }

    drawCenteredIndicator(COLOR_END_INDICATOR_FILL, COLOR_END_INDICATOR_BORDER);
}
