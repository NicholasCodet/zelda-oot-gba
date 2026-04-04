#ifndef WORLD_H
#define WORLD_H

#include <gba_types.h>

#define WORLD_INTERACTIVE_COUNT 2
#define WORLD_ROOM_OBSTACLE_COUNT 7
#define WORLD_LOCKED_DOOR_COUNT 2
#define WORLD_DOOR_COUNT 2
#define WORLD_ROOM_COUNT 7

// Generic rectangle/object used for world layout and collision.
typedef struct {
    int x;
    int y;
    int width;
    int height;
    int active;
} GameObject;

struct Player;

// Simple instant room transition zone.
typedef struct {
    GameObject zone;
    int targetRoomIndex;
    int targetSpawnX;
    int targetSpawnY;
    int active;
} DoorZone;

// Minimal persistent puzzle state stored per room.
typedef struct {
    int initialized;
    int interactiveActive[WORLD_INTERACTIVE_COUNT];
    int toggleObstacleActive[WORLD_INTERACTIVE_COUNT];
    int keyActive;
    int bigKeyActive;
    int lockedDoorActive[WORLD_LOCKED_DOOR_COUNT];
} RoomPersistentState;

// All simple world state used by this project.
typedef struct {
    GameObject interactiveObjects[WORLD_INTERACTIVE_COUNT];
    GameObject toggleObstacles[WORLD_INTERACTIVE_COUNT];
    GameObject roomObstacles[WORLD_ROOM_OBSTACLE_COUNT];
    GameObject keyObject;
    GameObject bigKeyObject;
    GameObject heartDrop;
    GameObject lockedDoors[WORLD_LOCKED_DOOR_COUNT];
    GameObject goalArea;
    DoorZone doorZones[WORLD_DOOR_COUNT];

    u16 interactiveOffColor[WORLD_INTERACTIVE_COUNT];
    u16 interactiveOnColor[WORLD_INTERACTIVE_COUNT];

    int interactiveCount;
    int roomObstacleCount;
    int doorCount;
    int interactionRange;
    int lockedDoorCount;
    int bossDoorIndex;
    int keyCount;
    int hasBigKey;
    int hasWon;
    // Debug-oriented: number of invalid layout placements detected
    // when the current room was initialized.
    int layoutValidationIssueCount;
    // Set by world update code when environment state changes and a full
    // playfield redraw is needed on the next render frame.
    int requestFullPlayfieldRedraw;

    // Simple room-state tracking for room transitions.
    int currentRoomIndex;
    int roomCount;
    RoomPersistentState roomStates[WORLD_ROOM_COUNT];

    // Spawn data for the current room.
    int playerSpawnX;
    int playerSpawnY;
    int enemySpawnX;
    int enemySpawnY;
    int enemyMaxHealth;
    int enemyMoveRange;
    int enemyMoveAxis;
} World;

void initWorld(World *world);
void loadWorldRoom(World *world, int roomIndex);

int isCollidingAABB(const GameObject *a, const GameObject *b);
int isCollidingWithActiveObjects(const GameObject *object, const GameObject *objects, int objectCount);

int isPlayerNearObject(
    const struct Player *player,
    int playerWidth,
    int playerHeight,
    const GameObject *object,
    int range
);

void updateWorldInteractions(
    World *world,
    const struct Player *player,
    u16 keysPressed,
    int playerWidth,
    int playerHeight
);

void updateWorldKeyDoor(
    World *world,
    const struct Player *player,
    int playerWidth,
    int playerHeight
);

void trySpawnHeartDrop(
    World *world,
    int enemyX,
    int enemyY,
    int enemyWidth,
    int enemyHeight
);

void updateWorldHeartPickup(
    World *world,
    struct Player *player,
    int playerWidth,
    int playerHeight
);

void updateWorldWinState(
    World *world,
    const struct Player *player,
    int playerWidth,
    int playerHeight
);

void updateBossRoomGate(World *world, int bossAlive);

int checkDoorZoneTransition(
    const World *world,
    const struct Player *player,
    int playerWidth,
    int playerHeight,
    int *targetRoomIndex,
    int *targetSpawnX,
    int *targetSpawnY
);

#endif
