#ifndef GAME_H
#define GAME_H

GLOBAL V3 V3F = {0,  0, -1};
GLOBAL V3 V3U = {0,  1,  0};
GLOBAL V3 V3R = {1,  0,  0};

#include "mesh.h"
#include "entity.h"

struct Camera
{
    V3 position;
    V3 forward;
    M4x4_Inverse matrix; // look_at matrix.
};

struct Game_State
{
    Camera camera;
    
    Table<String8, Texture>       texture_catalog;
    Table<String8, Triangle_Mesh> mesh_catalog;
};
GLOBAL Game_State *game;

#endif //GAME_H
