
FUNCTION void update_entity_transform(Entity *entity)
{
    V3 pos         = entity->position;
    Quaternion ori = entity->orientation;
    V3 scale       = entity->scale;
    
    M4x4 m_non = m4x4_identity();
    m_non._14 = pos.x;
    m_non._24 = pos.y;
    m_non._34 = pos.z;
    m_non._11 = scale.x;
    m_non._22 = scale.y;
    m_non._33 = scale.z;
    
    M4x4 m_inv = m4x4_identity();
    m_inv._14 = -pos.x;
    m_inv._24 = -pos.y;
    m_inv._34 = -pos.z;
    m_inv._11 = 1.0f/scale.x;
    m_inv._22 = 1.0f/scale.y;
    m_inv._33 = 1.0f/scale.z;
    
    M4x4 r = m4x4_from_quaternion(ori);
    
    entity->object_to_world_matrix.forward = m_non * r;
    entity->object_to_world_matrix.inverse = transpose(r) * m_inv;
    
    if (entity->mesh) {
        V3 p0 = entity->object_to_world_matrix.forward * entity->mesh->bounding_box_p0;
        V3 p1 = entity->object_to_world_matrix.forward * entity->mesh->bounding_box_p1;
        entity->bounding_box = { min_v3(p0, p1), max_v3(p0, p1) };
    }
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
}

FUNCTION Entity* create_new_entity(Entity_Manager *manager, Entity entity)
{
    Entity *new_entity = array_add(&game->entity_manager.entities, entity);
    return new_entity;
}

#if DEVELOPER
FUNCTION void create_entity_and_select(Entity_Manager *manager, Entity entity)
{
    Entity *e = create_new_entity(manager, entity);
    manager->selected_entity = e;
}

#endif