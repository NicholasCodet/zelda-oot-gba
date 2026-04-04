#ifndef RENDER_H
#define RENDER_H

#include "world.h"
#include "player.h"
#include "enemy.h"
#include "combat.h"

// Cached rectangles/state used by incremental rendering.
typedef struct {
    GameObject prevPlayerRect;
    GameObject prevAttackRect;
    int prevAttackWasActive;

    GameObject prevEnemyRect;
    int prevEnemyWasActive;
    GameObject prevHeartRect;
    int prevHeartWasActive;

    int prevHasWon;
    int prevInteractiveState[WORLD_INTERACTIVE_COUNT];
    int prevToggleState[WORLD_INTERACTIVE_COUNT];
} RenderState;

void initRenderState(
    RenderState *state,
    const World *world,
    const Player *player,
    const Enemy *enemy,
    const Attack *attack,
    int playerWidth,
    int playerHeight
);

void drawInitialFrame(
    const World *world,
    const Player *player,
    const Enemy *enemy,
    const Attack *attack,
    int playerWidth,
    int playerHeight
);

void renderFrame(
    World *world,
    const Player *player,
    const Enemy *enemy,
    const Attack *attack,
    int playerWidth,
    int playerHeight,
    RenderState *state
);

// Draw a simple full-screen end state (win/lose) overlay.
void drawEndStateScreen(int didWin);

#endif
