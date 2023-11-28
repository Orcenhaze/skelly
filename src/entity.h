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
};

#endif //ENTITY_H
