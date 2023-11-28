#ifndef EDITOR_H
#define EDITOR_H

struct Selected_Entity
{
    Entity *ptr;
};

struct Entity_Selection_Group
{
    Array<Selected_Entity> entities;
};

#endif //EDITOR_H
