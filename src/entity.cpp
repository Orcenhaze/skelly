
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
}

FUNCTION Entity create_default_entity()
{
    Entity result = {};
    result.orientation = quaternion_identity();
    result.scale       = v3(1.0f);
    update_entity_transform(&result);
    
    // @Hardcode:
    // @Hardcode:
    // @Hardcode:
    // @Cleanup: Just Testing...
    result.mesh = table_find_pointer(&game->mesh_catalog, S8LIT("sphere"));
    if (result.mesh)
        result.name = result.mesh->name;
    
    return result;
}

FUNCTION void entity_manager_init(Entity_Manager *manager)
{
    // @Todo: Still need to find a way to avoid reserving 8GB because of default ARENA_MAX_DEFAULT... ugh!
    array_init(&manager->entities);
}
