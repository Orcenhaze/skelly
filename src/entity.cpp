
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
    
    // @Todo: Shouldn't use permanent_arena here.
    Animation_Player *new_player = PUSH_STRUCT_ZERO(os->permanent_arena, Animation_Player);
    init(new_player);
    set_mesh(new_player, e->mesh);
    
    e->animation_player = new_player;
    return new_player;
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
    table_init(&manager->entity_table);
    array_init(&manager->entities);
    
#if DEVELOPER
    array_init(&manager->selected_entities);
#endif
}

FUNCTION Entity* register_new_entity(Entity_Manager *manager, Entity entity)
{
    entity.idx = (u32)manager->entities.count;
    
    // @Todo: Shouldn't use permanent_arena here...
    if (!entity.name.data) {
        String8 name = entity.mesh? entity.mesh->name : S8LIT("unnamed");
        entity.name  = sprint(os->permanent_arena, "%S (%u)", name, entity.idx);
    }
    
    Entity *new_entity = table_add(&manager->entity_table, entity.name, entity);
    array_add(&game->entity_manager.entities, new_entity);
    return new_entity;
}

FUNCTION Entity* find_entity(Entity_Manager *manager, String8 name)
{
    Entity *e = table_find_pointer(&manager->entity_table, name);
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