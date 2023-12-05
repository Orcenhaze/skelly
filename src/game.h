#ifndef GAME_H
#define GAME_H

GLOBAL V3 V3F = {0,  0, -1};
GLOBAL V3 V3U = {0,  1,  0};
GLOBAL V3 V3R = {1,  0,  0};

#include "mesh.h"
#include "entity.h"
#include "catalog.h"

#if DEVELOPER
#include "editor.h"
#endif

struct Camera
{
    V3 position;
    V3 forward;
    M4x4_Inverse matrix; // look_at matrix.
};

struct Game_State
{
    Catalog<String8, Texture>       texture_catalog;
    Catalog<String8, Triangle_Mesh> mesh_catalog;
    
    Entity_Manager entity_manager;
    
    Camera camera;
    V3 delta_mouse; // current - previous mouse_ndc.
    V3 mouse_world;
    
    s32               num_point_lights;
    Point_Light       point_lights[MAX_POINT_LIGHTS];
    s32               num_dir_lights;
	Directional_Light dir_lights[MAX_DIR_LIGHTS];
};
GLOBAL Game_State *game;

#endif //GAME_H
