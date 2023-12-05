#ifndef ENTITY_H
#define ENTITY_H

struct Entity
{
    String8 name;
    u32 idx;
    
    V3           position;
    Quaternion   orientation;
    V3           scale;
    M4x4_Inverse object_to_world_matrix;
    
    Triangle_Mesh *mesh;
};

struct Entity_Manager
{
    Array<Entity> entities;
    
#if DEVELOPER
    // Editor stuff.
    // @Temporary: We need proper group selection and stuff!
    Entity *selected_entity;
    
    // Entity we are currently hovering over (closest to camera).
    Entity *hovered_entity;
#endif
};

#endif //ENTITY_H
