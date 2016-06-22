#if !defined(GAME_H)

#include "ludus_audio.cpp"
#include "ludus_entity.h"
#include "ludus_platform.h"
#include "ludus_renderer.h"
#include "ludus_shared.h"

#define TILE_SIZE 64
#define MAP_WIDTH_IN_TILES 21
#define MAP_HEIGHT_IN_TILES 13

enum PLAYSTATE
{
    PLAYSTATE_LOGO,
    PLAYSTATE_TITLE,
    PLAYSTATE_GAME,
    PLAYSTATE_CREDITS
};

struct game_state
{
    b32 IsInitialized;

    u32 Playstate;
    u32 Statetimer;

    char* StartMessage;

    u32 EntityCount;
    u32 OnePastLastCollisionArea;
    u32 MapCollisionAreaCount;
    entity Entities[1000];
    collision_area CollisionAreas[1000];    // TODO(fierydrake): How to manage this memory?
    collision_area MapCollisionAreas[1000]; // TODO(fierydrake): How to manage this memory?
    u32 FreeEntityCount;
    u32 FirstFreeEntityIndex;
    collision_area* FirstFreeCollisionArea;

    r32 PhysicsTimeToCarry;
    char EntityInfo[256];

    audio_state AudioState;
};

#define UNSET_COST 999999
#define WALL_COST -1
struct pathfinding_info
{
    vec2 Pos;
    s32 Cost;
    pathfinding_info* Previous;
};

struct map_tile
{
    u8 Tile;
    u8 Base;
    u8 Overlay1;
    u8 Overlay2;
    u8 Overlay3;
    b32 Solid;
};

struct transient_state
{
    b32 IsInitialized;
    b32 AllTexturesLoaded;

    u32 MapEntityIndex;
    u32 PlayerEntityIndex;

    r32 EnemyX;
    r32 EnemyY;

    r32 EnemyWidth;
    r32 EnemyHeight;

    int CurrentTextureIndex[4];
    int CurrentOverlayIndex;
    texture_slot* Textures[50];
    font_slot* MainFont;

    texture_slot* MapBackground[34];

    map_tile Map[MAP_WIDTH_IN_TILES][MAP_HEIGHT_IN_TILES];
    pathfinding_info PathInfo[MAP_WIDTH_IN_TILES][MAP_HEIGHT_IN_TILES];

    memory_arena Arena;
};

// NOTE(kiljacken): Beware, touch this and ye shall die a fiery death.
// (Not really, but everything will look weird)
global_variable uint32 TransparentPixel = 0x00000000;

#define GAME_H
#endif
