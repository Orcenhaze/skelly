#ifndef GAME_H
#define GAME_H

GLOBAL V3 V3F = {0,  0, -1};
GLOBAL V3 V3U = {0,  1,  0};
GLOBAL V3 V3R = {1,  0,  0};

#include "animation.h"
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
