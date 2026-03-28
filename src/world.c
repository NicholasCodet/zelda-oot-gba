#include <gba_video.h>

#include "world.h"
#include "player.h"

static void loadRoom0(World *world)
{
    // Room 1: starter puzzle room with two linked gate columns.
    world->interactiveObjects[0] = (GameObject){ .x = 34, .y = 34, .width = 16, .height = 16, .active = 0 };
    world->interactiveObjects[1] = (GameObject){ .x = 60, .y = 104, .width = 16, .height = 16, .active = 0 };

    world->toggleObstacles[0] = (GameObject){ .x = 156, .y = 28, .width = 12, .height = 104, .active = 1 };
    world->toggleObstacles[1] = (GameObject){ .x = 168, .y = 28, .width = 12, .height = 104, .active = 1 };

    world->roomObstacles[0] = (GameObject){ .x = 20, .y = 20, .width = 200, .height = 8, .active = 1 };
    world->roomObstacles[1] = (GameObject){ .x = 20, .y = 132, .width = 200, .height = 8, .active = 1 };
    world->roomObstacles[2] = (GameObject){ .x = 20, .y = 20, .width = 8, .height = 120, .active = 1 };
    world->roomObstacles[3] = (GameObject){ .x = 212, .y = 20, .width = 8, .height = 120, .active = 1 };
    world->roomObstacles[4] = (GameObject){ .x = 96, .y = 52, .width = 28, .height = 56, .active = 1 };

    world->goalArea = (GameObject){ .x = 190, .y = 66, .width = 18, .height = 18, .active = 1 };

    world->playerSpawnX = 36;
    world->playerSpawnY = 72;
    world->enemySpawnX = 86;
    world->enemySpawnY = 112;
    world->enemyMoveRange = 28;
}

static void loadRoom1(World *world)
{
    // Room 2: alternate layout with a left-side block and mid-room gate barrier.
    world->interactiveObjects[0] = (GameObject){ .x = 34, .y = 56, .width = 16, .height = 16, .active = 0 };
    world->interactiveObjects[1] = (GameObject){ .x = 88, .y = 108, .width = 16, .height = 16, .active = 0 };

    world->toggleObstacles[0] = (GameObject){ .x = 116, .y = 28, .width = 12, .height = 104, .active = 1 };
    world->toggleObstacles[1] = (GameObject){ .x = 128, .y = 28, .width = 12, .height = 104, .active = 1 };

    world->roomObstacles[0] = (GameObject){ .x = 20, .y = 20, .width = 200, .height = 8, .active = 1 };
    world->roomObstacles[1] = (GameObject){ .x = 20, .y = 132, .width = 200, .height = 8, .active = 1 };
    world->roomObstacles[2] = (GameObject){ .x = 20, .y = 20, .width = 8, .height = 120, .active = 1 };
    world->roomObstacles[3] = (GameObject){ .x = 212, .y = 20, .width = 8, .height = 120, .active = 1 };
    world->roomObstacles[4] = (GameObject){ .x = 52, .y = 44, .width = 36, .height = 72, .active = 1 };

    world->goalArea = (GameObject){ .x = 186, .y = 102, .width = 18, .height = 18, .active = 1 };

    world->playerSpawnX = 34;
    world->playerSpawnY = 34;
    world->enemySpawnX = 98;
    world->enemySpawnY = 84;
    world->enemyMoveRange = 8;
}

void initWorld(World *world)
{
    world->interactiveOffColor[0] = RGB5(0, 0, 31);
    world->interactiveOffColor[1] = RGB5(0, 31, 0);
    world->interactiveOnColor[0] = RGB5(0, 31, 31);
    world->interactiveOnColor[1] = RGB5(31, 31, 0);

    world->roomCount = WORLD_ROOM_COUNT;
    world->interactiveCount = WORLD_INTERACTIVE_COUNT;
    world->roomObstacleCount = WORLD_ROOM_OBSTACLE_COUNT;
    world->interactionRange = 10;
    world->currentRoomIndex = 0;
    world->hasWon = 0;

    loadWorldRoom(world, 0);
}

void loadWorldRoom(World *world, int roomIndex)
{
    if (world->roomCount <= 0) {
        world->roomCount = WORLD_ROOM_COUNT;
    }

    // Keep room index wrapped to the available room count.
    if (roomIndex < 0 || roomIndex >= world->roomCount) {
        roomIndex = 0;
    }

    world->currentRoomIndex = roomIndex;

    if (roomIndex == 0) {
        loadRoom0(world);
    } else {
        loadRoom1(world);
    }

    // Every new room starts in a non-win state.
    world->hasWon = 0;
}

int isCollidingAABB(const GameObject *a, const GameObject *b)
{
    return (a->x < (b->x + b->width)) &&
           ((a->x + a->width) > b->x) &&
           (a->y < (b->y + b->height)) &&
           ((a->y + a->height) > b->y);
}

int isCollidingWithActiveObjects(const GameObject *object, const GameObject *objects, int objectCount)
{
    for (int i = 0; i < objectCount; i++) {
        if (objects[i].active && isCollidingAABB(object, &objects[i])) {
            return 1;
        }
    }

    return 0;
}

int isPlayerNearObject(
    const Player *player,
    int playerWidth,
    int playerHeight,
    const GameObject *object,
    int range
)
{
    GameObject playerRect = getPlayerRect(player, playerWidth, playerHeight);

    // Expand the object rectangle to create a simple interaction zone.
    GameObject interactionZone = {
        .x = object->x - range,
        .y = object->y - range,
        .width = object->width + (range * 2),
        .height = object->height + (range * 2),
        .active = 1
    };

    return isCollidingAABB(&playerRect, &interactionZone);
}

void updateWorldInteractions(
    World *world,
    const Player *player,
    u16 keysPressed,
    int playerWidth,
    int playerHeight
)
{
    if (player->isDead) {
        return;
    }

    for (int i = 0; i < world->interactiveCount; i++) {
        if ((keysPressed & KEY_A) == 0) {
            continue;
        }

        if (isPlayerNearObject(player, playerWidth, playerHeight, &world->interactiveObjects[i], world->interactionRange) == 0) {
            continue;
        }

        int nextState = (world->interactiveObjects[i].active == 0) ? 1 : 0;

        // Inverted logic:
        // object ON  -> linked obstacle OFF (path opens)
        // object OFF -> linked obstacle ON  (path closes)
        if (nextState) {
            world->interactiveObjects[i].active = 1;
            world->toggleObstacles[i].active = 0;
        } else {
            // Keep overlap guard to avoid trapping the player.
            GameObject playerRect = getPlayerRect(player, playerWidth, playerHeight);

            if (isCollidingAABB(&playerRect, &world->toggleObstacles[i]) == 0) {
                world->interactiveObjects[i].active = 0;
                world->toggleObstacles[i].active = 1;
            }
        }
    }
}

void updateWorldWinState(
    World *world,
    const Player *player,
    int playerWidth,
    int playerHeight
)
{
    // Dead players cannot trigger win progression.
    if (player->isDead || world->hasWon) {
        return;
    }

    int allInteractionsActive = 1;
    for (int i = 0; i < world->interactiveCount; i++) {
        if (world->interactiveObjects[i].active == 0) {
            allInteractionsActive = 0;
            break;
        }
    }

    if (allInteractionsActive == 0) {
        return;
    }

    GameObject playerRect = getPlayerRect(player, playerWidth, playerHeight);
    if (isCollidingAABB(&playerRect, &world->goalArea)) {
        world->hasWon = 1;
    }
}
