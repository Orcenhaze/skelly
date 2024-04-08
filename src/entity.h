#ifndef ENTITY_H
#define ENTITY_H

// @Todo: We are using poor man's id generation.
GLOBAL u32 entity_id_counter;

enum Entity_Type
{
    EntityType_NONE,
    
    EntityType_PLAYER,
    EntityType_BOT_CIRCLE,
    EntityType_BOT_LEVITATE,
};

struct Entity
{
    M4x4_Inverse object_to_world;
    String8 name;
    Triangle_Mesh *mesh;
    
    // @Note: Every entity with an animated mesh must have an animation player.
    Animation_Player *animation_player;
    
    Quaternion orientation;
    V3         position;
    V3         scale;
    V3         velocity;
    
    // @Temporary: For BOTs
    V4         color;
    V3         pivot; // Circles around this.
    
    Entity_Type type;
    u32 id;
};

struct Entity_Manager
{
    // @Note: In this program, entities are always "alive" in memory, stored in this table. 
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
