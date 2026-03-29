#include "combat.h"

static int signValue(int value)
{
    if (value > 0) {
        return 1;
    }
    if (value < 0) {
        return -1;
    }
    return 0;
}

static void clampEnemyToScreen(Enemy *enemy)
{
    if (enemy->x < 0) {
        enemy->x = 0;
    }
    if (enemy->x > (240 - enemy->width)) {
        enemy->x = 240 - enemy->width;
    }

    if (enemy->y < 0) {
        enemy->y = 0;
    }
    if (enemy->y > (160 - enemy->height)) {
        enemy->y = 160 - enemy->height;
    }
}

static void resolveBossOverlapWithPlayer(
    Enemy *enemy,
    const Player *player,
    int playerWidth,
    int playerHeight,
    int pushX,
    int pushY
)
{
    if (pushX == 0 && pushY == 0) {
        return;
    }

    GameObject playerRect = getPlayerRect(player, playerWidth, playerHeight);
    for (int i = 0; i < 10; i++) {
        GameObject enemyRect = getEnemyRect(enemy);
        if (!isCollidingAABB(&playerRect, &enemyRect)) {
            break;
        }

        enemy->x += pushX;
        enemy->y += pushY;
        clampEnemyToScreen(enemy);
    }
}

void initAttack(Attack *attack, int width, int height, int duration)
{
    attack->x = 0;
    attack->y = 0;
    attack->width = width;
    attack->height = height;

    attack->active = 0;
    attack->timer = 0;
    attack->duration = duration;
    attack->hasHitEnemy = 0;
}

GameObject getAttackRect(const Attack *attack)
{
    GameObject rect = {
        .x = attack->x,
        .y = attack->y,
        .width = attack->width,
        .height = attack->height,
        .active = attack->active
    };

    return rect;
}

void tryStartPlayerAttack(
    Attack *attack,
    const Player *player,
    u16 keysPressed,
    int playerWidth,
    int playerHeight
)
{
    if (player->isDead) {
        return;
    }

    if (!(keysPressed & KEY_B) || attack->timer != 0) {
        return;
    }

    attack->timer = attack->duration;
    attack->hasHitEnemy = 0;
    attack->active = 1;

    // Spawn the hitbox in front of the player based on current direction.
    if (player->direction == DIRECTION_UP) {
        attack->x = player->x + (playerWidth - attack->width) / 2;
        attack->y = player->y - attack->height;
    } else if (player->direction == DIRECTION_DOWN) {
        attack->x = player->x + (playerWidth - attack->width) / 2;
        attack->y = player->y + playerHeight;
    } else if (player->direction == DIRECTION_LEFT) {
        attack->x = player->x - attack->width;
        attack->y = player->y + (playerHeight - attack->height) / 2;
    } else { // DIRECTION_RIGHT
        attack->x = player->x + playerWidth;
        attack->y = player->y + (playerHeight - attack->height) / 2;
    }
}

void updateCombat(
    Player *player,
    Enemy *enemy,
    Attack *attack,
    int playerWidth,
    int playerHeight
)
{
    // Tick down enemy hit feedback timer.
    if (enemy->hitFlashTimer > 0) {
        enemy->hitFlashTimer--;
    }

    // Dead player cannot attack or take further contact damage.
    if (player->isDead) {
        attack->active = 0;
        attack->timer = 0;
        attack->hasHitEnemy = 0;
        player->knockbackTimer = 0;
        return;
    }

    // Keep attack active for a few frames and resolve enemy hit.
    if (attack->timer > 0) {
        if (enemy->active && attack->active && !attack->hasHitEnemy) {
            GameObject attackRect = getAttackRect(attack);
            GameObject enemyRect = getEnemyRect(enemy);

            if (isCollidingAABB(&attackRect, &enemyRect)) {
                // One attack swing removes one enemy health point.
                enemy->health--;
                enemy->hitFlashTimer = enemy->hitFlashDuration;
                attack->hasHitEnemy = 1;

                // Remove the enemy only when health reaches zero.
                if (enemy->health <= 0) {
                    enemy->health = 0;
                    enemy->active = 0;
                }
            }
        }

        attack->timer--;
        if (attack->timer == 0) {
            attack->active = 0;
        }
    }

    // Enemy contact damage:
    // damage the player once, then wait for invulnerability frames.
    if (enemy->active && player->invulnerabilityTimer == 0 && player->health > 0) {
        GameObject playerRect = getPlayerRect(player, playerWidth, playerHeight);
        GameObject enemyRect = getEnemyRect(enemy);

        if (isCollidingAABB(&playerRect, &enemyRect)) {
            player->health--;
            if (player->health < 0) {
                player->health = 0;
            }
            player->invulnerabilityTimer = player->invulnerabilityFrames;

            // Apply a short knockback away from the enemy.
            // Keep it simple: fixed speed and duration.
            {
                const int knockbackSpeed = 2;
                const int knockbackFrames = 6;
                int playerCenterX = player->x + (playerWidth / 2);
                int playerCenterY = player->y + (playerHeight / 2);
                int enemyCenterX = enemy->x + (enemy->width / 2);
                int enemyCenterY = enemy->y + (enemy->height / 2);
                int centerDeltaX = enemyCenterX - playerCenterX;
                int centerDeltaY = enemyCenterY - playerCenterY;
                int retreatDirX = signValue(centerDeltaX);
                int retreatDirY = signValue(centerDeltaY);

                player->knockbackX = (playerCenterX >= enemyCenterX) ? knockbackSpeed : -knockbackSpeed;
                player->knockbackY = (playerCenterY >= enemyCenterY) ? knockbackSpeed : -knockbackSpeed;
                player->knockbackTimer = knockbackFrames;

                // Boss contact handling:
                // 1) immediately separate rectangles
                // 2) enforce a short retreat + recovery window
                if (enemy->isBoss) {
                    const int bossRetreatStep = 2;
                    const int bossRetreatFrames = 6;

                    // Avoid zero-vector when centers match exactly.
                    if (retreatDirX == 0 && retreatDirY == 0) {
                        retreatDirY = -1;
                    }

                    enemy->bossRetreatX = retreatDirX * bossRetreatStep;
                    enemy->bossRetreatY = retreatDirY * bossRetreatStep;
                    enemy->bossRetreatTimer = bossRetreatFrames;
                    enemy->bossRecoverTimer = 16;
                    enemy->bossChaseTimer = 0;
                    enemy->bossPauseTimer = 10;

                    resolveBossOverlapWithPlayer(
                        enemy,
                        player,
                        playerWidth,
                        playerHeight,
                        enemy->bossRetreatX,
                        enemy->bossRetreatY
                    );
                }
            }

            // Enter dead state immediately when health reaches zero.
            if (player->health == 0) {
                player->isDead = 1;
                attack->timer = 0;
                attack->hasHitEnemy = 0;
                attack->active = 0;
                player->knockbackTimer = 0;
            }
        }
    }
}
