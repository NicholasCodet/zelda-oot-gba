#include <gba_video.h>

#include "world.h"
#include "player.h"
#include "enemy.h"

#define WORLD_PLAYER_RECT_WIDTH 12
#define WORLD_PLAYER_RECT_HEIGHT 12
#define WORLD_ENEMY_RECT_WIDTH 14
#define WORLD_ENEMY_RECT_HEIGHT 14
#define WORLD_SCAN_MIN_X 20
#define WORLD_SCAN_MIN_Y 20
#define WORLD_SCAN_MAX_X 220
#define WORLD_SCAN_MAX_Y 140
#define HEART_DROP_CHANCE_PERCENT 40
#define REWARD_FLASH_FRAMES 18
#define REWARD_WIN_DELAY_FRAMES 18

// Simple deterministic RNG for lightweight drop chance checks.
static unsigned int gDropRngState = 0x1A2B3C4Du;

static int nextDropRoll100(void)
{
    gDropRngState = (gDropRngState * 1103515245u) + 12345u;
    return (int)((gDropRngState >> 16) % 100u);
}

// Returns 1 if the rectangle overlaps any active room geometry.
static int overlapsRoomGeometry(const World *world, const GameObject *rect)
{
    int overlaps = isCollidingWithActiveObjects(rect, world->roomObstacles, world->roomObstacleCount);
    if (!overlaps) {
        overlaps = isCollidingWithActiveObjects(rect, world->toggleObstacles, world->interactiveCount);
    }
    if (!overlaps) {
        overlaps = isCollidingWithActiveObjects(rect, world->lockedDoors, world->lockedDoorCount);
    }

    return overlaps;
}

// Find a simple walkable placement for a rectangle inside room bounds.
static int findWalkablePlacement(const World *world, int width, int height, int *outX, int *outY)
{
    for (int y = WORLD_SCAN_MIN_Y; y <= (WORLD_SCAN_MAX_Y - height); y += 2) {
        for (int x = WORLD_SCAN_MIN_X; x <= (WORLD_SCAN_MAX_X - width); x += 2) {
            GameObject testRect = {
                .x = x,
                .y = y,
                .width = width,
                .height = height,
                .active = 1
            };

            if (!overlapsRoomGeometry(world, &testRect)) {
                *outX = x;
                *outY = y;
                return 1;
            }
        }
    }

    return 0;
}

// Lightweight room validation:
// - detect invalid overlaps for key room objects
// - auto-correct player/enemy spawn if they start inside geometry
static void validateRoomLayout(World *world)
{
    world->layoutValidationIssueCount = 0;

    GameObject playerSpawnRect = {
        .x = world->playerSpawnX,
        .y = world->playerSpawnY,
        .width = WORLD_PLAYER_RECT_WIDTH,
        .height = WORLD_PLAYER_RECT_HEIGHT,
        .active = 1
    };
    if (overlapsRoomGeometry(world, &playerSpawnRect)) {
        world->layoutValidationIssueCount++;
        (void)findWalkablePlacement(
            world,
            WORLD_PLAYER_RECT_WIDTH,
            WORLD_PLAYER_RECT_HEIGHT,
            &world->playerSpawnX,
            &world->playerSpawnY
        );
    }

    GameObject enemySpawnRect = {
        .x = world->enemySpawnX,
        .y = world->enemySpawnY,
        .width = WORLD_ENEMY_RECT_WIDTH,
        .height = WORLD_ENEMY_RECT_HEIGHT,
        .active = 1
    };
    if (overlapsRoomGeometry(world, &enemySpawnRect)) {
        world->layoutValidationIssueCount++;
        (void)findWalkablePlacement(
            world,
            WORLD_ENEMY_RECT_WIDTH,
            WORLD_ENEMY_RECT_HEIGHT,
            &world->enemySpawnX,
            &world->enemySpawnY
        );
    }

    for (int i = 0; i < world->interactiveCount; i++) {
        if (overlapsRoomGeometry(world, &world->interactiveObjects[i])) {
            world->layoutValidationIssueCount++;
        }
    }

    if (world->goalArea.active && overlapsRoomGeometry(world, &world->goalArea)) {
        world->layoutValidationIssueCount++;
    }

    if (world->keyObject.active && overlapsRoomGeometry(world, &world->keyObject)) {
        world->layoutValidationIssueCount++;
    }

    if (world->bigKeyObject.active && overlapsRoomGeometry(world, &world->bigKeyObject)) {
        world->layoutValidationIssueCount++;
    }

    for (int i = 0; i < world->lockedDoorCount; i++) {
        if (world->lockedDoors[i].active && overlapsRoomGeometry(world, &world->lockedDoors[i])) {
            world->layoutValidationIssueCount++;
        }
    }
}

// Save current room interaction state into persistent room storage.
static void saveCurrentRoomPersistentState(World *world)
{
    if (world->currentRoomIndex < 0 || world->currentRoomIndex >= world->roomCount) {
        return;
    }

    RoomPersistentState *state = &world->roomStates[world->currentRoomIndex];
    state->initialized = 1;

    for (int i = 0; i < world->interactiveCount; i++) {
        state->interactiveActive[i] = world->interactiveObjects[i].active;
        state->toggleObstacleActive[i] = world->toggleObstacles[i].active;
    }
    state->keyActive = world->keyObject.active;
    state->bigKeyActive = world->bigKeyObject.active;
    for (int i = 0; i < WORLD_LOCKED_DOOR_COUNT; i++) {
        state->lockedDoorActive[i] = world->lockedDoors[i].active;
    }
}

// Load persistent state for the current room, or capture defaults on first load.
static void applyCurrentRoomPersistentState(World *world)
{
    if (world->currentRoomIndex < 0 || world->currentRoomIndex >= world->roomCount) {
        return;
    }

    RoomPersistentState *state = &world->roomStates[world->currentRoomIndex];

    if (!state->initialized) {
        saveCurrentRoomPersistentState(world);
        return;
    }

    for (int i = 0; i < world->interactiveCount; i++) {
        world->interactiveObjects[i].active = state->interactiveActive[i];
        world->toggleObstacles[i].active = state->toggleObstacleActive[i];
    }
    world->keyObject.active = state->keyActive;
    world->bigKeyObject.active = state->bigKeyActive;
    for (int i = 0; i < WORLD_LOCKED_DOOR_COUNT; i++) {
        world->lockedDoors[i].active = state->lockedDoorActive[i];
    }
}

static void loadRoom0(World *world)
{
    // Interaction room (progression room 2):
    // primary exit is a spatial passage to room 3 on the right side.
    // Compared to room 1, this room increases pressure with a wider center
    // blocker and a stronger enemy patrol.
    world->interactiveObjects[0] = (GameObject){ .x = 34, .y = 34, .width = 16, .height = 16, .active = 0 };
    world->interactiveObjects[1] = (GameObject){ .x = 130, .y = 104, .width = 16, .height = 16, .active = 0 };

    world->toggleObstacles[0] = (GameObject){ .x = 152, .y = 28, .width = 12, .height = 104, .active = 1 };
    world->toggleObstacles[1] = (GameObject){ .x = 164, .y = 28, .width = 12, .height = 104, .active = 1 };

    // Outer room bounds with a single right-side opening at y=64..95.
    world->roomObstacles[0] = (GameObject){ .x = 20, .y = 20, .width = 200, .height = 8, .active = 1 };   // top
    world->roomObstacles[1] = (GameObject){ .x = 20, .y = 132, .width = 200, .height = 8, .active = 1 };  // bottom
    world->roomObstacles[2] = (GameObject){ .x = 20, .y = 20, .width = 8, .height = 120, .active = 1 };   // left wall (closed)
    world->roomObstacles[3] = (GameObject){ .x = 212, .y = 20, .width = 8, .height = 44, .active = 1 };   // right upper
    world->roomObstacles[4] = (GameObject){ .x = 212, .y = 96, .width = 8, .height = 44, .active = 1 };   // right lower
    world->roomObstacles[5] = (GameObject){ .x = 92, .y = 48, .width = 36, .height = 64, .active = 1 };
    world->roomObstacles[6] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };

    // Room 2 uses passage transition only: no active goal teleporter here.
    world->goalArea = (GameObject){ .x = 190, .y = 66, .width = 18, .height = 18, .active = 0 };
    world->keyObject = (GameObject){ .x = 56, .y = 76, .width = 8, .height = 8, .active = 1 };
    world->bigKeyObject = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->lockedDoorCount = 1;
    world->bossDoorIndex = -1;
    // Locked door blocks the right opening until one key is spent.
    world->lockedDoors[0] = (GameObject){ .x = 212, .y = 64, .width = 8, .height = 32, .active = 1 };
    world->lockedDoors[1] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };

    world->playerSpawnX = 36;
    world->playerSpawnY = 72;
    world->enemySpawnX = 68;
    world->enemySpawnY = 94;
    world->enemyMaxHealth = 2;
    world->enemyMoveRange = 16;
    world->enemyMoveAxis = ENEMY_MOVE_AXIS_X;
    world->enemyType = ENEMY_TYPE_PATROL;

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
    // Keep interactions close and readable to reduce early friction.
    world->interactiveObjects[0] = (GameObject){ .x = 34, .y = 56, .width = 16, .height = 16, .active = 0 };
    world->interactiveObjects[1] = (GameObject){ .x = 90, .y = 104, .width = 16, .height = 16, .active = 0 };

    world->toggleObstacles[0] = (GameObject){ .x = 120, .y = 28, .width = 10, .height = 104, .active = 1 };
    world->toggleObstacles[1] = (GameObject){ .x = 130, .y = 28, .width = 10, .height = 104, .active = 1 };

    // Closed room bounds: no spatial passage in this room.
    world->roomObstacles[0] = (GameObject){ .x = 20, .y = 20, .width = 200, .height = 8, .active = 1 };   // top
    world->roomObstacles[1] = (GameObject){ .x = 20, .y = 132, .width = 200, .height = 8, .active = 1 };  // bottom
    world->roomObstacles[2] = (GameObject){ .x = 20, .y = 20, .width = 8, .height = 120, .active = 1 };   // left
    world->roomObstacles[3] = (GameObject){ .x = 212, .y = 20, .width = 8, .height = 120, .active = 1 };  // right
    world->roomObstacles[4] = (GameObject){ .x = 56, .y = 50, .width = 24, .height = 60, .active = 1 };
    world->roomObstacles[5] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->roomObstacles[6] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };

    world->goalArea = (GameObject){ .x = 186, .y = 102, .width = 18, .height = 18, .active = 1 };
    world->keyObject = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->bigKeyObject = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->lockedDoorCount = 0;
    world->bossDoorIndex = -1;
    world->lockedDoors[0] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->lockedDoors[1] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };

    world->playerSpawnX = 34;
    world->playerSpawnY = 34;
    world->enemySpawnX = 150;
    world->enemySpawnY = 84;
    world->enemyMaxHealth = 1;
    world->enemyMoveRange = 4;
    world->enemyMoveAxis = ENEMY_MOVE_AXIS_X;
    world->enemyType = ENEMY_TYPE_CHASER;

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
    world->keyObject = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->bigKeyObject = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->lockedDoorCount = 0;
    world->bossDoorIndex = -1;
    world->lockedDoors[0] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->lockedDoors[1] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };

    world->playerSpawnX = 34;
    world->playerSpawnY = 96;
    world->enemySpawnX = 90;
    world->enemySpawnY = 44;
    world->enemyMaxHealth = 2;
    world->enemyMoveRange = 18;
    world->enemyMoveAxis = ENEMY_MOVE_AXIS_X;
    world->enemyType = ENEMY_TYPE_PATROL;

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
    // Challenge room (progression room 4):
    // no teleporter here; progression continues through a spatial passage.
    // Sequence:
    // 1) Activate switch 0 to open the center gate into the enemy zone.
    // 2) Use the room key to open the locked gate into the right chamber.
    // 3) Collect the big key, then continue to room 5.
    world->interactiveObjects[0] = (GameObject){ .x = 36, .y = 104, .width = 16, .height = 16, .active = 0 };
    // Second switch is intentionally removed for clarity in this room.
    // Keep slot 1 pre-activated and non-visible so generic logic still works.
    world->interactiveObjects[1] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 1 };

    // Gate 0 blocks left-to-right traversal until switch 0 is activated.
    world->toggleObstacles[0] = (GameObject){ .x = 104, .y = 28, .width = 12, .height = 104, .active = 1 };
    // Gate 1 is disabled in Room 4; locked door now owns this progression gate.
    world->toggleObstacles[1] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };

    // Top wall has one opening that leads to room 5.
    world->roomObstacles[0] = (GameObject){ .x = 20, .y = 20, .width = 160, .height = 8, .active = 1 };   // top-left
    world->roomObstacles[1] = (GameObject){ .x = 20, .y = 132, .width = 200, .height = 8, .active = 1 };  // bottom
    world->roomObstacles[2] = (GameObject){ .x = 20, .y = 20, .width = 8, .height = 120, .active = 1 };   // left
    world->roomObstacles[3] = (GameObject){ .x = 212, .y = 20, .width = 8, .height = 120, .active = 1 };  // right
    // Bottom wall of the goal chamber: keeps it physically gated until gate 1 opens.
    world->roomObstacles[4] = (GameObject){ .x = 140, .y = 64, .width = 72, .height = 12, .active = 1 };
    // Extra center pillar tightens navigation in the final room.
    world->roomObstacles[5] = (GameObject){ .x = 72, .y = 64, .width = 20, .height = 48, .active = 1 };
    world->roomObstacles[6] = (GameObject){ .x = 204, .y = 20, .width = 16, .height = 8, .active = 1 };   // top-right

    // Teleporter goal is removed from room 4 to keep only spatial transitions.
    world->goalArea = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    // Place the key closer to the upper-right route so the objective is
    // easier to read, while still inside enemy patrol pressure.
    world->keyObject = (GameObject){ .x = 154, .y = 80, .width = 8, .height = 8, .active = 1 };
    // Big key keeps normal key size/shape and is isolated in the chamber.
    world->bigKeyObject = (GameObject){ .x = 188, .y = 40, .width = 8, .height = 8, .active = 1 };
    world->lockedDoorCount = 1;
    world->bossDoorIndex = -1;
    // Place the locked door on the goal-chamber entry lane so it becomes
    // a required gate instead of a side obstacle.
    world->lockedDoors[0] = (GameObject){ .x = 140, .y = 28, .width = 12, .height = 36, .active = 1 };
    world->lockedDoors[1] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };

    world->playerSpawnX = 34;
    world->playerSpawnY = 92;
    // Variation: this room uses a vertical patrol enemy with higher health.
    world->enemySpawnX = 150;
    world->enemySpawnY = 86;
    world->enemyMaxHealth = 3;
    world->enemyMoveRange = 26;
    world->enemyMoveAxis = ENEMY_MOVE_AXIS_Y;
    world->enemyType = ENEMY_TYPE_BRUTE;

    // Spatial exit to room 5 through the top opening.
    world->doorZones[0] = (DoorZone){
        .zone = { .x = 180, .y = 0, .width = 24, .height = 24, .active = 1 },
        .targetRoomIndex = 4,
        .targetSpawnX = 186,
        .targetSpawnY = 118,
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

static void loadRoom4(World *world)
{
    // Pre-boss pressure room (progression room 5):
    // this room now focuses on movement pressure + boss-door access only.
    // Trigger mechanics are fully removed for clarity.
    world->interactiveObjects[0] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->interactiveObjects[1] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };

    // Related trigger gates are removed from this room.
    world->toggleObstacles[0] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->toggleObstacles[1] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };

    // Top and bottom walls each have one opening for spatial passages.
    world->roomObstacles[0] = (GameObject){ .x = 20, .y = 20, .width = 160, .height = 8, .active = 1 };   // top-left
    world->roomObstacles[1] = (GameObject){ .x = 204, .y = 20, .width = 16, .height = 8, .active = 1 };   // top-right
    world->roomObstacles[2] = (GameObject){ .x = 20, .y = 132, .width = 160, .height = 8, .active = 1 };  // bottom-left
    world->roomObstacles[3] = (GameObject){ .x = 204, .y = 132, .width = 16, .height = 8, .active = 1 };  // bottom-right
    world->roomObstacles[4] = (GameObject){ .x = 20, .y = 20, .width = 8, .height = 120, .active = 1 };   // left
    world->roomObstacles[5] = (GameObject){ .x = 212, .y = 20, .width = 8, .height = 120, .active = 1 };  // right
    // Remove the non-essential center blocker to keep the boss-door route clearer.
    world->roomObstacles[6] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };

    // Teleporter goal is disabled here: room 5 -> room 6 now uses a physical passage.
    world->goalArea = (GameObject){ .x = 186, .y = 34, .width = 18, .height = 18, .active = 0 };
    world->keyObject = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->bigKeyObject = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->lockedDoorCount = 1;
    // Special boss door: this door blocks the boss-room teleporter lane.
    // It can only be opened by the big key, not by normal keys.
    world->bossDoorIndex = 0;
    world->lockedDoors[0] = (GameObject){ .x = 180, .y = 20, .width = 24, .height = 12, .active = 1 };
    world->lockedDoors[1] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };

    world->playerSpawnX = 32;
    world->playerSpawnY = 102;
    world->enemySpawnX = 146;
    world->enemySpawnY = 82;
    world->enemyMaxHealth = 3;
    world->enemyMoveRange = 28;
    world->enemyMoveAxis = ENEMY_MOVE_AXIS_Y;
    world->enemyType = ENEMY_TYPE_CHASER;

    // Spatial exit to room 6 through the top wall opening.
    world->doorZones[0] = (DoorZone){
        .zone = { .x = 180, .y = 0, .width = 24, .height = 24, .active = 1 },
        .targetRoomIndex = 5,
        .targetSpawnX = 186,
        .targetSpawnY = 118,
        .active = 1
    };
    world->doorZones[1] = (DoorZone){
        .zone = { .x = 180, .y = 136, .width = 24, .height = 24, .active = 1 },
        .targetRoomIndex = 3,
        .targetSpawnX = 186,
        .targetSpawnY = 34,
        .active = 1
    };
}

static void loadRoom5(World *world)
{
    // Boss room (progression room 6):
    // open combat layout with one simple center obstacle.
    world->interactiveObjects[0] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 1 };
    world->interactiveObjects[1] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 1 };

    world->toggleObstacles[0] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->toggleObstacles[1] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };

    // Top wall has an opening to room 7; this opening is blocked by gate [6]
    // until the room enemy (boss placeholder) is defeated.
    world->roomObstacles[0] = (GameObject){ .x = 20, .y = 20, .width = 88, .height = 8, .active = 1 };    // top-left
    world->roomObstacles[1] = (GameObject){ .x = 132, .y = 20, .width = 88, .height = 8, .active = 1 };   // top-right
    // Bottom wall has one opening aligned with room 5's top opening.
    world->roomObstacles[2] = (GameObject){ .x = 20, .y = 132, .width = 160, .height = 8, .active = 1 };  // bottom-left
    world->roomObstacles[3] = (GameObject){ .x = 204, .y = 132, .width = 16, .height = 8, .active = 1 };  // bottom-right
    world->roomObstacles[4] = (GameObject){ .x = 20, .y = 20, .width = 8, .height = 120, .active = 1 };   // left
    world->roomObstacles[5] = (GameObject){ .x = 212, .y = 20, .width = 8, .height = 120, .active = 1 };  // right
    // Gate to room 7 (opened by updateBossRoomGate when enemy is defeated).
    // It fully covers the transition zone while active.
    world->roomObstacles[6] = (GameObject){ .x = 108, .y = 0, .width = 24, .height = 28, .active = 1 };

    // No teleporter / goal marker in boss room.
    world->goalArea = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->keyObject = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->bigKeyObject = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->lockedDoorCount = 0;
    world->bossDoorIndex = -1;
    world->lockedDoors[0] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->lockedDoors[1] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };

    world->playerSpawnX = 110;
    world->playerSpawnY = 112;
    // Boss placeholder uses the existing enemy system (no new mechanics).
    world->enemySpawnX = 110;
    world->enemySpawnY = 64;
    world->enemyMaxHealth = 5;
    world->enemyMoveRange = 0;
    world->enemyMoveAxis = ENEMY_MOVE_AXIS_X;
    world->enemyType = ENEMY_TYPE_BOSS;

    // Room 6 -> room 7 passage (only usable once gate obstacle [6] is opened).
    world->doorZones[0] = (DoorZone){
        .zone = { .x = 108, .y = 0, .width = 24, .height = 24, .active = 1 },
        .targetRoomIndex = 6,
        .targetSpawnX = 110,
        .targetSpawnY = 118,
        .active = 1
    };
    // Return passage to room 5.
    world->doorZones[1] = (DoorZone){
        .zone = { .x = 180, .y = 136, .width = 24, .height = 24, .active = 1 },
        .targetRoomIndex = 4,
        .targetSpawnX = 186,
        .targetSpawnY = 34,
        .active = 1
    };
}

static void loadRoom6(World *world)
{
    // Room 7 (reward room):
    // reachable from room 6 only after the boss gate opens.
    world->interactiveObjects[0] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 1 };
    world->interactiveObjects[1] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 1 };

    world->toggleObstacles[0] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->toggleObstacles[1] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };

    world->roomObstacles[0] = (GameObject){ .x = 20, .y = 20, .width = 200, .height = 8, .active = 1 };   // top
    world->roomObstacles[1] = (GameObject){ .x = 20, .y = 132, .width = 88, .height = 8, .active = 1 };   // bottom-left
    world->roomObstacles[2] = (GameObject){ .x = 132, .y = 132, .width = 88, .height = 8, .active = 1 };  // bottom-right
    world->roomObstacles[3] = (GameObject){ .x = 20, .y = 20, .width = 8, .height = 120, .active = 1 };   // left
    world->roomObstacles[4] = (GameObject){ .x = 212, .y = 20, .width = 8, .height = 120, .active = 1 };  // right
    world->roomObstacles[5] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->roomObstacles[6] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };

    // Reward room win condition is collectible-based, not goal-platform based.
    world->goalArea = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->keyObject = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    // Place the reward clearly at the room center.
    world->bigKeyObject = (GameObject){ .x = 116, .y = 76, .width = 8, .height = 8, .active = 1 };
    world->lockedDoorCount = 0;
    world->bossDoorIndex = -1;
    world->lockedDoors[0] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->lockedDoors[1] = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };

    world->playerSpawnX = 110;
    world->playerSpawnY = 114;
    world->enemySpawnX = 0;
    world->enemySpawnY = 0;
    world->enemyMaxHealth = 0;
    world->enemyMoveRange = 0;
    world->enemyMoveAxis = ENEMY_MOVE_AXIS_X;
    world->enemyType = ENEMY_TYPE_PATROL;

    // Return passage to room 6.
    world->doorZones[0] = (DoorZone){
        .zone = { .x = 108, .y = 136, .width = 24, .height = 24, .active = 1 },
        .targetRoomIndex = 5,
        .targetSpawnX = 110,
        .targetSpawnY = 34,
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
    world->lockedDoorCount = 0;
    world->bossDoorIndex = -1;
    world->keyCount = 0;
    world->hasBigKey = 0;
    world->currentRoomIndex = 0;
    world->hasWon = 0;
    world->rewardFlashTimer = 0;
    world->rewardWinDelayTimer = 0;
    world->requestFullPlayfieldRedraw = 1;
    world->heartDrop = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };
    world->enemyType = ENEMY_TYPE_PATROL;
    gDropRngState = 0x1A2B3C4Du;

    // Persistence storage is initialized once; defaults are captured
    // on first load of each room.
    for (int roomIndex = 0; roomIndex < WORLD_ROOM_COUNT; roomIndex++) {
        world->roomStates[roomIndex].initialized = 0;
        for (int i = 0; i < WORLD_INTERACTIVE_COUNT; i++) {
            world->roomStates[roomIndex].interactiveActive[i] = 0;
            world->roomStates[roomIndex].toggleObstacleActive[i] = 0;
        }
        world->roomStates[roomIndex].keyActive = 0;
        world->roomStates[roomIndex].bigKeyActive = 0;
        for (int i = 0; i < WORLD_LOCKED_DOOR_COUNT; i++) {
            world->roomStates[roomIndex].lockedDoorActive[i] = 0;
        }
    }

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
    // 0 = intro -> 1 = interaction -> 2 = puzzle -> 3 = challenge
    // -> 4 = pre-boss room -> 5 = boss room -> 6 = reward room.
    // Room builders stay separate so layouts remain easy to tune.
    if (roomIndex == 0) {
        loadRoom1(world);
    } else if (roomIndex == 1) {
        loadRoom0(world);
    } else if (roomIndex == 2) {
        loadRoom2(world);
    } else if (roomIndex == 3) {
        loadRoom3(world);
    } else if (roomIndex == 4) {
        loadRoom4(world);
    } else if (roomIndex == 5) {
        loadRoom5(world);
    } else {
        loadRoom6(world);
    }

    // Heart drops are runtime combat drops, not authored room layout.
    world->heartDrop = (GameObject){ .x = 0, .y = 0, .width = 0, .height = 0, .active = 0 };

    // Apply per-room persistent interaction/obstacle states.
    applyCurrentRoomPersistentState(world);

    // Run a simple layout sanity check after all room objects are initialized.
    validateRoomLayout(world);

    // Every new room starts in a non-win state.
    world->hasWon = 0;
    world->rewardFlashTimer = 0;
    world->rewardWinDelayTimer = 0;
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

    int changedState = 0;

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
            changedState = 1;
        } else {
            // Keep overlap guard to avoid trapping the player.
            GameObject playerRect = getPlayerRect(player, playerWidth, playerHeight);

            if (isCollidingAABB(&playerRect, &world->toggleObstacles[i]) == 0) {
                world->interactiveObjects[i].active = 0;
                world->toggleObstacles[i].active = 1;
                changedState = 1;
            }
        }
    }

    if (changedState) {
        saveCurrentRoomPersistentState(world);
    }
}

void updateWorldWinState(
    World *world,
    const Player *player,
    int playerWidth,
    int playerHeight
)
{
    // Reward-room success beat:
    // keep gameplay in PLAYING for a short moment, then switch to WIN.
    if (world->rewardWinDelayTimer > 0) {
        world->rewardWinDelayTimer--;
        if (world->rewardFlashTimer > 0) {
            world->rewardFlashTimer--;
        }

        if (world->rewardWinDelayTimer == 0) {
            world->hasWon = 1;
        }

        return;
    }

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

    if (!world->goalArea.active) {
        return;
    }

    GameObject playerRect = getPlayerRect(player, playerWidth, playerHeight);
    if (isCollidingAABB(&playerRect, &world->goalArea)) {
        // Final room goal is locked until the big key has been collected.
        if ((world->currentRoomIndex == (world->roomCount - 1)) && !world->hasBigKey) {
            return;
        }
        world->hasWon = 1;
    }
}

void updateBossRoomGate(World *world, int bossAlive)
{
    // Room index 5 is the boss room.
    if (world->currentRoomIndex != 5) {
        return;
    }

    // Gate is obstacle slot [6] in loadRoom5.
    int shouldBlockPassage = bossAlive ? 1 : 0;
    if (world->roomObstacles[6].active != shouldBlockPassage) {
        world->roomObstacles[6].active = shouldBlockPassage;
        world->requestFullPlayfieldRedraw = 1;
    }
}

void updateWorldKeyDoor(
    World *world,
    const Player *player,
    int playerWidth,
    int playerHeight
)
{
    if (player->isDead) {
        return;
    }

    int changedState = 0;
    int rewardCollectedInFinalRoom = 0;
    GameObject playerRect = getPlayerRect(player, playerWidth, playerHeight);

    // Collect key on touch.
    if (world->keyObject.active && isCollidingAABB(&playerRect, &world->keyObject)) {
        world->keyObject.active = 0;
        world->keyCount++;
        changedState = 1;
    }

    // Collect the big key on touch.
    if (world->bigKeyObject.active && isCollidingAABB(&playerRect, &world->bigKeyObject)) {
        world->bigKeyObject.active = 0;
        // In the final room this object is the dungeon reward.
        if (world->currentRoomIndex == (world->roomCount - 1)) {
            world->rewardFlashTimer = REWARD_FLASH_FRAMES;
            world->rewardWinDelayTimer = REWARD_WIN_DELAY_FRAMES;
            rewardCollectedInFinalRoom = 1;
        } else {
            // In earlier rooms it remains the progression big key.
            world->hasBigKey = 1;
            changedState = 1;
        }
    }

    // Spend one key to open one nearby locked door.
    if (world->keyCount > 0) {
        for (int i = 0; i < world->lockedDoorCount; i++) {
            if (!world->lockedDoors[i].active) {
                continue;
            }

            // Big boss door cannot be opened with normal keys.
            if (i == world->bossDoorIndex) {
                continue;
            }

            if (isPlayerNearObject(player, playerWidth, playerHeight, &world->lockedDoors[i], 2)) {
                world->lockedDoors[i].active = 0;
                world->keyCount--;
                changedState = 1;
                break;
            }
        }
    }

    // Big key opens only the special boss door.
    if (world->hasBigKey && world->bossDoorIndex >= 0 && world->bossDoorIndex < world->lockedDoorCount) {
        int doorIndex = world->bossDoorIndex;

        if (world->lockedDoors[doorIndex].active &&
            isPlayerNearObject(player, playerWidth, playerHeight, &world->lockedDoors[doorIndex], 2)) {
            world->lockedDoors[doorIndex].active = 0;
            changedState = 1;
        }
    }

    if (changedState) {
        saveCurrentRoomPersistentState(world);
        world->requestFullPlayfieldRedraw = 1;
    } else if (rewardCollectedInFinalRoom) {
        // Reward collection uses incremental redraw + local flash effect.
        // No forced full-playfield redraw to avoid visible flash.
        saveCurrentRoomPersistentState(world);
    }
}

void trySpawnHeartDrop(
    World *world,
    int enemyX,
    int enemyY,
    int enemyWidth,
    int enemyHeight
)
{
    if (world->heartDrop.active) {
        return;
    }

    // Simple chance-based drop on enemy death.
    if (nextDropRoll100() >= HEART_DROP_CHANCE_PERCENT) {
        return;
    }

    world->heartDrop.x = enemyX + (enemyWidth / 2) - 3;
    world->heartDrop.y = enemyY + (enemyHeight / 2) - 3;
    world->heartDrop.width = 6;
    world->heartDrop.height = 6;
    world->heartDrop.active = 1;
    // Do not request full playfield redraw here.
    // Enemy death + heart spawn are handled by normal incremental redraw.
}

void updateWorldHeartPickup(
    World *world,
    Player *player,
    int playerWidth,
    int playerHeight
)
{
    if (player->isDead || !world->heartDrop.active) {
        return;
    }

    GameObject playerRect = getPlayerRect(player, playerWidth, playerHeight);
    if (!isCollidingAABB(&playerRect, &world->heartDrop)) {
        return;
    }

    world->heartDrop.active = 0;

    // Restore one health point without exceeding max health.
    if (player->health < player->maxHealth) {
        player->health++;
        if (player->health > player->maxHealth) {
            player->health = player->maxHealth;
        }
    }

    // Keep pickup redraw incremental to avoid visible full-playfield flash.
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
