#ifndef WORLD_H
#define WORLD_H

#include <gba_types.h>

#define WORLD_INTERACTIVE_COUNT 2
#define WORLD_ROOM_OBSTACLE_COUNT 5
#define WORLD_ROOM_COUNT 4

// Generic rectangle/object used for world layout and collision.
typedef struct {
    int x;
    int y;
    int width;
    int height;
    int active;
} GameObject;

struct Player;

// All simple world state used by this project.
typedef struct {
    GameObject interactiveObjects[WORLD_INTERACTIVE_COUNT];
    GameObject toggleObstacles[WORLD_INTERACTIVE_COUNT];
    GameObject roomObstacles[WORLD_ROOM_OBSTACLE_COUNT];
    GameObject goalArea;

    u16 interactiveOffColor[WORLD_INTERACTIVE_COUNT];
    u16 interactiveOnColor[WORLD_INTERACTIVE_COUNT];

    int interactiveCount;
    int roomObstacleCount;
    int interactionRange;
    int hasWon;
    // Set by world update code when environment state changes and a full
    // playfield redraw is needed on the next render frame.
    int requestFullPlayfieldRedraw;

    // Simple room-state tracking for room transitions.
    int currentRoomIndex;
    int roomCount;

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

void updateWorldWinState(
    World *world,
    const struct Player *player,
    int playerWidth,
    int playerHeight
);

#endif
