
FUNCTION void update_entity_transform(Entity *entity)
{
    V3 pos         = entity->position;
    Quaternion ori = entity->orientation;
    V3 scale       = entity->scale;
    
    M4x4 t = m4x4_identity();
    t._14  = pos.x;
    t._24  = pos.y;
    t._34  = pos.z;
    
    M4x4 r = m4x4_from_quaternion(ori);
    
    M4x4 s = m4x4_identity();
    s._11  = scale.x;
    s._22  = scale.y;
    s._33  = scale.z;
    
    entity->object_to_world_matrix.forward = t * r * s;
    invert(entity->object_to_world_matrix.forward, &entity->object_to_world_matrix.inverse);
}

FUNCTION Entity create_default_entity()
{
    Entity result = {};
    result.orientation = quaternion_identity();
    result.scale       = v3(1.0f);
    update_entity_transform(&result);
    
    // @Cleanup: First mesh in catalog is default for now...
    result.mesh = game->mesh_catalog.items[0];
    
    return result;
}

FUNCTION void entity_manager_init(Entity_Manager *manager)
{
    // @Todo: Still need to find a way to avoid reserving 8GB because of default ARENA_MAX_DEFAULT... ugh!
    array_init(&manager->entities);
    
#if DEVELOPER
    array_init(&manager->selected_entities);
#endif
}

FUNCTION Entity* create_new_entity(Entity_Manager *manager, Entity entity)
{
    Entity *new_entity = array_add(&game->entity_manager.entities, entity);
    return new_entity;
}
