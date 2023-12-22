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
    
    // @Todo: This should be a pointer?
    Animation_Player animation_player;
};

struct Entity_Manager
{
    Array<Entity> entities;
    
#if DEVELOPER
    // Editor stuff.
    Array<Entity*> selected_entities;
    Entity *selected_entity; // Last one we seleceted: selected_entities[selected_entities.count-1]
#endif
};

#endif //ENTITY_H
