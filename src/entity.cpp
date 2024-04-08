
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

FUNCTION void update_entity_transform(Entity *entity)
{
    V3 pos         = entity->position;
    Quaternion ori = entity->orientation;
    V3 scale       = entity->scale;
    
    entity->object_to_world.forward = m4x4_from_translation_rotation_scale(pos, ori, scale);
    invert(entity->object_to_world.forward, &entity->object_to_world.inverse);
}

FUNCTION Animation_Player* create_animation_player_for_entity(Entity *e)
{
    if (!e->mesh) {
        debug_print("Entity %S: Can't create animation without mesh.", e->name);
        return 0;
    }
    
    Animation_Player *new_player = (Animation_Player *) malloc(sizeof(Animation_Player));
    ASSERT(new_player);
    MEMORY_ZERO(new_player, sizeof(Animation_Player));
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
                                 Entity_Type type = EntityType_NONE,
                                 String8 name     = S8ZERO,
                                 V3 pos           = V3ZERO, 
                                 Quaternion ori   = quaternion_identity(), 
                                 V3 scale         = v3(1.0f))
{
    ASSERT(mesh);
    
    Entity entity = {};
    entity.type        = type;
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
    register_new_entity(manager, player_mesh, EntityType_PLAYER, S8LIT("player"));
    Entity *player = find_entity(manager, 0);
    
#if DEVELOPER
    array_init(&manager->selected_entities);
    manager->selected_entity = 0;
#endif
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

FUNCTION void update_entity(Entity *e)
{
    Input_State *input = &os->tick_input;
    f32 dt = os->dt;
    
    switch (e->type) {
        case EntityType_PLAYER: {
            
#if DEVELOPER
            if (game->mode == GameMode_DEBUG) break;
#endif
            
            //~ Movement
            V2 move = {};
            
            if (key_held(input, Key_S)) move.y = -1;
            if (key_held(input, Key_W)) move.y =  1;
            if (key_held(input, Key_A)) move.x = -1;
            if (key_held(input, Key_D)) 
                move.x = 1;
            
            // We will move along the camera's forward and right vectors.
            V3 r = get_right(game->camera.object_to_world);
            V3 f = get_forward(game->camera.object_to_world);
            
            // Ignore pitch of camera forward.
            f.y  = 0.0f;
            f    = normalize_or_forward(f);
            
            // Apply movement speed and friction.
            V3 acceleration = f*move.y + r*move.x; 
            acceleration    = normalize_or_zero(acceleration);
            
            V3 facing_dir   = acceleration;
            
            acceleration   *= 80.0f; // @Hack: Movement speed.
            acceleration   += -8.0f*e->velocity; // @Hack: Friction.
            
            V3 move_delta = e->velocity*dt + 0.5f*acceleration*SQUARE(dt);
            e->velocity  += acceleration*dt;
            e->position  += move_delta;
            
            // Orient rotation to movement.
            if (length2(facing_dir) > KINDA_SMALL_NUMBER) {
                // yaw starts from z-axis NOT forward, so negate facing direction.
                f32 yaw               = get_euler(-facing_dir).yaw;
                Quaternion target_ori = quaternion_from_axis_angle(V3U, yaw);
                e->orientation        = rotate_towards(e->orientation, target_ori, dt, 10.0f);
            }
            
            // Also add a bit of yaw to the camera we moving left/right.
            // Only if we're not trying to turn the camera ourselves.
            if (!os->tick_input.mouse_delta.x)
                game->camera.euler.yaw += 2.0f * -move.x * dt;
            
            
            //~ Animation.
            
            Sampled_Animation *anim_to_play = find(&game->animation_catalog, S8LIT("guy_idle"));
            if (key_pressed(input, Key_MLEFT))
                anim_to_play = find(&game->animation_catalog, S8LIT("guy_hit"));
            else if (move.x || move.y)
                anim_to_play = find(&game->animation_catalog, S8LIT("guy_run"));
            
            b32 loop = anim_to_play->name != S8LIT("guy_hit");
            if (anim_to_play)
                play_animation(e, anim_to_play, loop);
        } break;
        
        case EntityType_BOT_CIRCLE: {
            // Move in circular motion.
            V3 centripetal_accel = normalize_or_zero(e->pivot - e->position) * 10.0f;
            V3 forward_accel     = normalize_or_zero(e->velocity) * 60.0f;
            V3 acceleration      = forward_accel + centripetal_accel;
            acceleration        += -8.0f*e->velocity; // @Hack: Friction.
            
            V3 delta     = e->velocity*dt + 0.5f*acceleration*SQUARE(dt);
            e->velocity += acceleration*dt;
            e->position += delta;
            
            // Orient rotation to movement.
            if (length2(e->velocity) > KINDA_SMALL_NUMBER) {
                // yaw starts from z-axis NOT forward, so negate facing direction.
                f32 yaw               = get_euler(-e->velocity).yaw;
                Quaternion target_ori = quaternion_from_axis_angle(V3U, yaw);
                e->orientation        = rotate_towards(e->orientation, target_ori, dt, 10.0f);
            }
        } break;
        
        case EntityType_BOT_LEVITATE: {
            f32 yaw          = 20.0f * DEGS_TO_RADS;
            Quaternion q_yaw = quaternion_from_axis_angle(V3U, yaw * dt);
            e->orientation   = q_yaw * e->orientation;
            e->position      = move_towards(e->position, e->position + V3U*5000.0f, dt, 1.0f);
        } break;
    }
    
    update_entity_transform(e);
    
    advance_time(e->animation_player, os->dt);
    eval(e->animation_player);
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