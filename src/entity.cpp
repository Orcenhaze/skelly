
FUNCTION void update_entity_transform(Entity *entity)
{
    V3 pos         = entity->position;
    Quaternion ori = entity->orientation;
    V3 scale       = entity->scale;
    
    entity->object_to_world_matrix.forward = m4x4_from_translation_rotation_scale(pos, ori, scale);
    invert(entity->object_to_world_matrix.forward, &entity->object_to_world_matrix.inverse);
}

FUNCTION Animation_Player* create_animation_player_for_entity(Entity *e)
{
    if (!e->mesh) {
        debug_print("Entity %S: Can't create animation without mesh.", e->name);
        return 0;
    }
    
    Animation_Player *new_player = (Animation_Player *) malloc(sizeof(Animation_Player));
    ASSERT(new_player);
    init(new_player);
    set_mesh(new_player, e->mesh);
    
    e->animation_player = new_player;
    return new_player;
}

FUNCTION void set_mesh_on_entity(Entity *entity, Triangle_Mesh *mesh)
{
    if (!mesh) return;
    
    Triangle_Mesh *old_mesh = entity->mesh;
    entity->mesh = mesh;
    
    if (mesh->flags & MeshFlags_ANIMATED) {
        // If first time setting mesh, then create animation player.
        if (!old_mesh || !entity->animation_player)
            create_animation_player_for_entity(entity);
        else
            set_mesh(entity->animation_player, mesh);
    } else {
        if (entity->animation_player)
            destroy(&entity->animation_player);
    }
}

FUNCTION u32 register_new_entity(Entity_Manager *manager, Triangle_Mesh *mesh, 
                                 String8 name   = S8ZERO,
                                 V3 pos         = V3ZERO, 
                                 Quaternion ori = quaternion_identity(), 
                                 V3 scale       = v3(1.0f))
{
    ASSERT(mesh);
    
    Entity entity = {};
    entity.position    = pos;
    entity.orientation = ori;
    entity.scale       = scale;
    update_entity_transform(&entity);
    
    entity.id = entity_id_counter++;
    
    set_mesh_on_entity(&entity, mesh);
    
    // Make sure newly created entity always has a unique name.
    String8 n = name.data? name : entity.mesh? entity.mesh->name : S8LIT("unnamed");
    entity.name  = sprint(os->permanent_arena, "%S_%u", n, entity.id);
    
    table_add(&manager->entity_table, entity.id, entity);
    array_add(&game->entity_manager.all_entities, entity.id);
    
    return entity.id;
}

FUNCTION void entity_manager_init(Entity_Manager *manager, Triangle_Mesh *player_mesh)
{
    // @Todo: Still need to find a way to avoid reserving 8GB because of default ARENA_MAX_DEFAULT... ugh!
    table_init(&manager->entity_table);
    array_init(&manager->all_entities);
    
    // Create player entity.
    register_new_entity(manager, player_mesh, S8LIT("player"));
    
#if DEVELOPER
    array_init(&manager->selected_entities);
    manager->selected_entity = 0;
#endif
}

FUNCTION Entity* find_entity(Entity_Manager *manager, u32 entity_id)
{
    Entity *e = table_find_pointer(&manager->entity_table, entity_id);
    return e;
}

FUNCTION Entity* get_player(Entity_Manager *manager)
{
    Entity *e = table_find_pointer(&manager->entity_table, 0U);
    return e;
}

FUNCTION Animation_Channel* play_animation(Entity *e, Sampled_Animation *anim, b32 loop = TRUE, f64 blend_duration = 0.2)
{
    ASSERT(e);
    
    Animation_Player *player = e->animation_player;
    if (!player) return 0;
    if (!anim)   return 0;
    
    for (s32 i = 0; i < player->channels.count; i++) {
        if (player->channels[i]->animation == anim) {
            // The passed animation is already playing.
            //debug_print("Animation %S already playing on Entity %S\n", anim->name, e->name);
            return 0;
        }
    }
    
    // Blend out current animations.
    for (s32 i = 0; i < player->channels.count; i++) {
        Animation_Channel *channel = player->channels[i];
        channel->blend_duration = blend_duration;
        channel->blending_out   = TRUE;
        channel->blend_t        = 0;
    }
    
    // Blend in new animation.
    Animation_Channel *new_channel = add_animation_channel(player);
    set_animation(new_channel, anim, 0);
    new_channel->blend_duration = blend_duration;
    new_channel->blending_in    = TRUE;
    new_channel->blend_t        = 0;
    new_channel->is_looping     = loop;
    
    return new_channel;
}

#if DEVELOPER
FUNCTION void select_single_entity(Entity_Manager *manager, u32 entity_id)
{
    array_reset(&manager->selected_entities);
    array_add(&manager->selected_entities, entity_id);
    manager->selected_entity = find_entity(manager, entity_id);
}

FUNCTION void add_entity_selection(Entity_Manager *manager, u32 entity_id)
{
    s32 find_index = array_find_index(&manager->selected_entities, entity_id);
    if (find_index == -1)
        array_add(&manager->selected_entities, entity_id);
    else
        array_unordered_remove_by_index(&manager->selected_entities, find_index);
    
    u32 last_entity = manager->selected_entities[manager->selected_entities.count-1];
    manager->selected_entity = find_entity(manager, last_entity);
}

FUNCTION void clear_entity_selection(Entity_Manager *manager)
{
    array_reset(&manager->selected_entities);
    manager->selected_entity = 0;
}
#endif