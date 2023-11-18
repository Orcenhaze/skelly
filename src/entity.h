#ifndef ENTITY_H
#define ENTITY_H

struct Entity
{
    String8 name;
    
    V3           position;
    Quaternion   orientation;
    V3           scale;
    M4x4_Inverse object_to_world_matrix;
    
    Triangle_Mesh *mesh;
};

#endif //ENTITY_H
