#ifndef COMBAT_H
#define COMBAT_H

#include <gba_input.h>

#include "player.h"
#include "enemy.h"

// Short-lived player attack hitbox.
typedef struct {
    int x;
    int y;
    int width;
    int height;

    int active;
    int timer;
    int duration;
    int hasHitEnemy;
} Attack;

void initAttack(Attack *attack, int width, int height, int duration);

GameObject getAttackRect(const Attack *attack);

void tryStartPlayerAttack(
    Attack *attack,
    const Player *player,
    u16 keysPressed,
    int playerWidth,
    int playerHeight
);

void updateCombat(
    Player *player,
    Enemy *enemy,
    Attack *attack,
    int playerWidth,
    int playerHeight
);

#endif
