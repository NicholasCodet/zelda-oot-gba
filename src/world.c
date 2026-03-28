#include <gba_video.h>

#include "world.h"
#include "player.h"
#include "enemy.h"

static void loadRoom0(World *world)
{
    // Interaction room (progression room 2):
    // primary exit is a spatial passage to room 3 on the right side.
    world->interactiveObjects[0] = (GameObject){ .x = 34, .y = 34, .width = 16, .height = 16, .active = 0 };
    world->interactiveObjects[1] = (GameObject){ .x = 60, .y = 104, .width = 16, .height = 16, .active = 0 };

    world->toggleObstacles[0] = (GameObject){ .x = 156, .y = 28, .width = 12, .height = 104, .active = 1 };
    world->toggleObstacles[1] = (GameObject){ .x = 168, .y = 28, .width = 12, .height = 104, .active = 1 };

    // Outer room bounds with a single right-side opening at y=64..95.
    world->roomObstacles[0] = (GameObject){ .x = 20, .y = 20, .width = 200, .height = 8, .active = 1 };   // top
    world->roomObstacles[1] = (GameObject){ .x = 20, .y = 132, .width = 200, .height = 8, .active = 1 };  // bottom
    world->roomObstacles[2] = (GameObject){ .x = 20, .y = 20, .width = 8, .height = 120, .active = 1 };   // left wall (closed)
    world->roomObstacles[3] = (GameObject){ .x = 212, .y = 20, .width = 8, .height = 44, .active = 1 };   // right upper
    world->roomObstacles[4] = (GameObject){ .x = 212, .y = 96, .width = 8, .height = 44, .active = 1 };   // right lower
    world->roomObstacles[5] = (GameObject){ .x = 96, .y = 52, .width = 28, .height = 56, .active = 1 };
    world->roomObstacles[6] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };

    // Room 2 uses passage transition only: no active goal teleporter here.
    world->goalArea = (GameObject){ .x = 190, .y = 66, .width = 18, .height = 18, .active = 0 };

    world->playerSpawnX = 36;
    world->playerSpawnY = 72;
    world->enemySpawnX = 86;
    world->enemySpawnY = 112;
    world->enemyMaxHealth = 2;
    world->enemyMoveRange = 20;
    world->enemyMoveAxis = ENEMY_MOVE_AXIS_X;

    // Single spatial door: room 2 -> room 3.
    world->doorZones[0] = (DoorZone){
        .zone = { .x = 220, .y = 64, .width = 20, .height = 32, .active = 1 },
        .targetRoomIndex = 2,
        .targetSpawnX = 30,
        .targetSpawnY = 74,
        .active = 1
    };
    world->doorZones[1] = (DoorZone){
        .zone = { .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 },
        .targetRoomIndex = 0,
        .targetSpawnX = 0,
        .targetSpawnY = 0,
        .active = 0
    };
}

static void loadRoom1(World *world)
{
    // Intro room (progression room 1):
    // primary exit is the goal teleporter to room 2.
    world->interactiveObjects[0] = (GameObject){ .x = 34, .y = 56, .width = 16, .height = 16, .active = 0 };
    world->interactiveObjects[1] = (GameObject){ .x = 88, .y = 108, .width = 16, .height = 16, .active = 0 };

    world->toggleObstacles[0] = (GameObject){ .x = 116, .y = 28, .width = 12, .height = 104, .active = 1 };
    world->toggleObstacles[1] = (GameObject){ .x = 128, .y = 28, .width = 12, .height = 104, .active = 1 };

    // Closed room bounds: no spatial passage in this room.
    world->roomObstacles[0] = (GameObject){ .x = 20, .y = 20, .width = 200, .height = 8, .active = 1 };   // top
    world->roomObstacles[1] = (GameObject){ .x = 20, .y = 132, .width = 200, .height = 8, .active = 1 };  // bottom
    world->roomObstacles[2] = (GameObject){ .x = 20, .y = 20, .width = 8, .height = 120, .active = 1 };   // left
    world->roomObstacles[3] = (GameObject){ .x = 212, .y = 20, .width = 8, .height = 120, .active = 1 };  // right
    world->roomObstacles[4] = (GameObject){ .x = 52, .y = 44, .width = 36, .height = 72, .active = 1 };
    world->roomObstacles[5] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->roomObstacles[6] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };

    world->goalArea = (GameObject){ .x = 186, .y = 102, .width = 18, .height = 18, .active = 1 };

    world->playerSpawnX = 34;
    world->playerSpawnY = 34;
    world->enemySpawnX = 98;
    world->enemySpawnY = 84;
    world->enemyMaxHealth = 1;
    world->enemyMoveRange = 6;
    world->enemyMoveAxis = ENEMY_MOVE_AXIS_X;

    // No spatial exits in this room.
    world->doorZones[0] = (DoorZone){
        .zone = { .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 },
        .targetRoomIndex = 0,
        .targetSpawnX = 0,
        .targetSpawnY = 0,
        .active = 0
    };
    world->doorZones[1] = (DoorZone){
        .zone = { .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 },
        .targetRoomIndex = 0,
        .targetSpawnX = 0,
        .targetSpawnY = 0,
        .active = 0
    };
}

static void loadRoom2(World *world)
{
    // Puzzle room (progression room 3): goal teleporter to room 4.
    // 1) Lower gate opens first to reach the second switch.
    // 2) Upper gate then opens to physically access the goal chamber.
    world->interactiveObjects[0] = (GameObject){ .x = 172, .y = 104, .width = 16, .height = 16, .active = 0 };
    world->interactiveObjects[1] = (GameObject){ .x = 36, .y = 104, .width = 16, .height = 16, .active = 0 };

    // Upper and lower passage gates on the center split.
    world->toggleObstacles[0] = (GameObject){ .x = 120, .y = 28, .width = 12, .height = 44, .active = 1 };
    world->toggleObstacles[1] = (GameObject){ .x = 120, .y = 88, .width = 12, .height = 36, .active = 1 };

    // Room bounds with a matching left-side opening for Room 2 <-> Room 3 passage.
    world->roomObstacles[0] = (GameObject){ .x = 20, .y = 20, .width = 200, .height = 8, .active = 1 };   // top
    world->roomObstacles[1] = (GameObject){ .x = 20, .y = 132, .width = 200, .height = 8, .active = 1 };  // bottom
    world->roomObstacles[2] = (GameObject){ .x = 20, .y = 20, .width = 8, .height = 44, .active = 1 };    // left upper
    world->roomObstacles[3] = (GameObject){ .x = 20, .y = 96, .width = 8, .height = 44, .active = 1 };    // left lower
    world->roomObstacles[4] = (GameObject){ .x = 212, .y = 20, .width = 8, .height = 120, .active = 1 };  // right
    // Divider blocks climbing from right-lower to right-upper without opening upper gate.
    world->roomObstacles[5] = (GameObject){ .x = 132, .y = 72, .width = 80, .height = 12, .active = 1 };
    world->roomObstacles[6] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };

    world->goalArea = (GameObject){ .x = 186, .y = 36, .width = 18, .height = 18, .active = 1 };

    world->playerSpawnX = 34;
    world->playerSpawnY = 96;
    world->enemySpawnX = 72;
    world->enemySpawnY = 36;
    world->enemyMaxHealth = 2;
    world->enemyMoveRange = 20;
    world->enemyMoveAxis = ENEMY_MOVE_AXIS_X;

    // Matching return passage: room 3 -> room 2.
    world->doorZones[0] = (DoorZone){
        .zone = { .x = 0, .y = 64, .width = 20, .height = 32, .active = 1 },
        .targetRoomIndex = 1,
        .targetSpawnX = 198,
        .targetSpawnY = 74,
        .active = 1
    };
    world->doorZones[1] = (DoorZone){
        .zone = { .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 },
        .targetRoomIndex = 0,
        .targetSpawnX = 0,
        .targetSpawnY = 0,
        .active = 0
    };
}

static void loadRoom3(World *world)
{
    // Challenge room (progression room 4): goal teleporter back to room 1.
    // Sequence:
    // 1) Activate switch 0 to open the center gate into the enemy zone.
    // 2) Reach switch 1 on the right side to open the goal chamber gate.
    world->interactiveObjects[0] = (GameObject){ .x = 36, .y = 104, .width = 16, .height = 16, .active = 0 };
    world->interactiveObjects[1] = (GameObject){ .x = 166, .y = 102, .width = 16, .height = 16, .active = 0 };

    // Gate 0 blocks left-to-right traversal until switch 0 is activated.
    world->toggleObstacles[0] = (GameObject){ .x = 104, .y = 28, .width = 12, .height = 104, .active = 1 };
    // Gate 1 closes the top-right goal chamber entrance until switch 1 is activated.
    world->toggleObstacles[1] = (GameObject){ .x = 140, .y = 28, .width = 12, .height = 36, .active = 1 };

    // Closed room bounds: no spatial passage in this room.
    world->roomObstacles[0] = (GameObject){ .x = 20, .y = 20, .width = 200, .height = 8, .active = 1 };   // top
    world->roomObstacles[1] = (GameObject){ .x = 20, .y = 132, .width = 200, .height = 8, .active = 1 };  // bottom
    world->roomObstacles[2] = (GameObject){ .x = 20, .y = 20, .width = 8, .height = 120, .active = 1 };   // left
    world->roomObstacles[3] = (GameObject){ .x = 212, .y = 20, .width = 8, .height = 120, .active = 1 };  // right
    // Bottom wall of the goal chamber: keeps it physically gated until gate 1 opens.
    world->roomObstacles[4] = (GameObject){ .x = 140, .y = 64, .width = 72, .height = 12, .active = 1 };
    world->roomObstacles[5] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->roomObstacles[6] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };

    world->goalArea = (GameObject){ .x = 184, .y = 36, .width = 18, .height = 18, .active = 1 };

    world->playerSpawnX = 34;
    world->playerSpawnY = 92;
    // Variation: this room uses a vertical patrol enemy with higher health.
    world->enemySpawnX = 152;
    world->enemySpawnY = 100;
    world->enemyMaxHealth = 3;
    world->enemyMoveRange = 18;
    world->enemyMoveAxis = ENEMY_MOVE_AXIS_Y;

    // No spatial exits in this room.
    world->doorZones[0] = (DoorZone){
        .zone = { .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 },
        .targetRoomIndex = 0,
        .targetSpawnX = 0,
        .targetSpawnY = 0,
        .active = 0
    };
    world->doorZones[1] = (DoorZone){
        .zone = { .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 },
        .targetRoomIndex = 0,
        .targetSpawnX = 0,
        .targetSpawnY = 0,
        .active = 0
    };
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
    world->doorCount = WORLD_DOOR_COUNT;
    world->interactionRange = 10;
    world->currentRoomIndex = 0;
    world->hasWon = 0;
    world->requestFullPlayfieldRedraw = 1;

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

    // Progression order:
    // 0 = intro -> 1 = interaction -> 2 = puzzle -> 3 = challenge.
    // Room builders stay separate so layouts remain easy to tune.
    if (roomIndex == 0) {
        loadRoom1(world);
    } else if (roomIndex == 1) {
        loadRoom0(world);
    } else if (roomIndex == 2) {
        loadRoom2(world);
    } else {
        loadRoom3(world);
    }

    // Every new room starts in a non-win state.
    world->hasWon = 0;
    // Force a full playfield redraw after room/world state replacement.
    world->requestFullPlayfieldRedraw = 1;
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

int checkDoorZoneTransition(
    const World *world,
    const Player *player,
    int playerWidth,
    int playerHeight,
    int *targetRoomIndex,
    int *targetSpawnX,
    int *targetSpawnY
)
{
    if (player->isDead) {
        return 0;
    }

    GameObject playerRect = getPlayerRect(player, playerWidth, playerHeight);

    for (int i = 0; i < world->doorCount; i++) {
        if (!world->doorZones[i].active) {
            continue;
        }

        if (isCollidingAABB(&playerRect, &world->doorZones[i].zone)) {
            *targetRoomIndex = world->doorZones[i].targetRoomIndex;
            *targetSpawnX = world->doorZones[i].targetSpawnX;
            *targetSpawnY = world->doorZones[i].targetSpawnY;
            return 1;
        }
    }

    return 0;
}
