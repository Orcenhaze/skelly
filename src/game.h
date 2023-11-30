#ifndef GAME_H
#define GAME_H

GLOBAL V3 V3F = {0,  0, -1};
GLOBAL V3 V3U = {0,  1,  0};
GLOBAL V3 V3R = {1,  0,  0};

#include "mesh.h"
#include "entity.h"

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
    Camera camera;
    
    // @Todo: Catalogs need to have both table and array...
    Table<String8, Texture>       texture_catalog;
    Table<String8, Triangle_Mesh> mesh_catalog;
    
    Entity_Manager entity_manager;
    
    s32               num_point_lights;
    Point_Light       point_lights[MAX_POINT_LIGHTS];
    s32               num_dir_lights;
	Directional_Light dir_lights[MAX_DIR_LIGHTS];
    
    
#if DEVELOPER
    // Editor stuff.
    // @Temporary: We need proper group selection and stuff!
    Selected_Entity selected_entity;
#endif
};
GLOBAL Game_State *game;

#endif //GAME_H
