#ifndef GAME_H
#define GAME_H

// World base vectors.
GLOBAL V3 V3ZERO = V3_ZERO;
GLOBAL V3 V3F    = V3_FORWARD;
GLOBAL V3 V3U    = V3_UP;
GLOBAL V3 V3R    = V3_RIGHT;

#include "mesh.h"
#include "animation.h"
#include "entity.h"
#include "catalog.h"

enum Game_Mode
{
    GameMode_GAME,
    GameMode_MENU,
    
    GameMode_DEBUG,
};

struct Camera
{
    M4x4       object_to_world;
#if DEVELOPER
    Quaternion orientation;
#endif
    V3         euler;
    V3         position;     // Current position.
    V3         start_offset; // Offset from player.
};

FUNCTION inline Point_Light default_point_light()
{
    Point_Light result = {};
    result.position    = {};
    result.intensity   = 1.0f;
    result.color       = {1.0f, 1.0f, 1.0f};
    result.range       = 3.5f;
    return result;
}
FUNCTION inline Directional_Light default_dir_light()
{
    Directional_Light result = {};
    result.direction                   = {-1.0f, -1.0f, -1.0f};
    result.intensity                   = 6.0f;
    result.color                       = {1.0f, 1.0f, 1.0f};
    result.indirect_lighting_intensity = 1.0f;
    return result;
}

struct Game_State
{
    Catalog<Texture>           texture_catalog;
    Catalog<Triangle_Mesh>     mesh_catalog;
    Catalog<Sampled_Animation> animation_catalog;
    
    Random_PCG rng;
    Entity_Manager entity_manager;
    
    Camera camera;
    V3 mouse_world;
    Game_Mode mode;
    
    
    s32               num_point_lights;
    Point_Light       point_lights[MAX_POINT_LIGHTS];
    s32               num_dir_lights;
	Directional_Light dir_lights[MAX_DIR_LIGHTS];
};
GLOBAL Game_State *game;

#endif //GAME_H
