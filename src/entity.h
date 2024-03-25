#ifndef ENTITY_H
#define ENTITY_H

// @Todo: We are using poor man's id generation.
GLOBAL u32 entity_id_counter;

struct Entity
{
    String8 name;
    u32 id;
    
    V3           position;
    Quaternion   orientation;
    V3           scale;
    M4x4_Inverse object_to_world_matrix;
    
    Triangle_Mesh *mesh;
    
    Animation_Player *animation_player;
};

struct Entity_Manager
{
    // Maps entity ID to entity data.
    Table<u32, Entity> entity_table;
    
    // Stores IDs of entities.
    Array<u32> all_entities;
    
#if DEVELOPER
    // Editor stuff.
    
    // Stores IDs.
    Array<u32> selected_entities;
    Entity *selected_entity; // Last one we seleceted: selected_entities[selected_entities.count-1]
#endif
};

#endif //ENTITY_H
