#include "combat.h"

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

                player->knockbackX = (playerCenterX >= enemyCenterX) ? knockbackSpeed : -knockbackSpeed;
                player->knockbackY = (playerCenterY >= enemyCenterY) ? knockbackSpeed : -knockbackSpeed;
                player->knockbackTimer = knockbackFrames;
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
