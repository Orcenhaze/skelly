
#include "mesh.cpp"
#include "animation.cpp"
#include "entity.cpp"
#include "draw.cpp"
#include "gizmo.cpp"

#if DEVELOPER
#include "editor.cpp"
#endif

FUNCTION void load_textures(Arena *arena, Catalog<Texture> *catalog)
{
    Arena_Temp scratch = get_scratch(0, 0);
    defer(free_scratch(scratch));
    
    String8 path_wild = sprint(scratch.arena, "%Stextures/*.*", os->data_folder);
    
    File_Group file_group = os->get_all_files_in_path(scratch.arena, path_wild);
    File_Info *info       = file_group.first_file_info;
    
    while (info) {
        String8 name = str8_copy(arena, info->base_name);
        String8 path = str8_copy(arena, info->full_path);
        
        Texture tex  = {};
        d3d11_load_texture(&tex, path);
        
        add(catalog, name, tex);
        
        info = info->next;
    }
}

FUNCTION void load_meshes(Arena *arena, Catalog<Triangle_Mesh> *catalog)
{
    Arena_Temp scratch = get_scratch(0, 0);
    defer(free_scratch(scratch));
    
    String8 path_wild = sprint(scratch.arena, "%Smeshes/*.mesh", os->data_folder);
    
    File_Group file_group = os->get_all_files_in_path(scratch.arena, path_wild);
    File_Info *info       = file_group.first_file_info;
    
    while (info) {
        String8 name = str8_copy(arena, info->base_name);
        String8 path = str8_copy(arena, info->full_path);
        
        Triangle_Mesh mesh = {};
        mesh.name = name;
        load_triangle_mesh(arena, &mesh, path);
        
        add(catalog, name, mesh);
        
        info = info->next;
    }
}

FUNCTION void load_animations(Arena *arena, Catalog<Sampled_Animation> *catalog)
{
    Arena_Temp scratch = get_scratch(0, 0);
    defer(free_scratch(scratch));
    
    String8 path_wild = sprint(scratch.arena, "%Sanimations/*.sampled_animation", os->data_folder);
    
    File_Group file_group = os->get_all_files_in_path(scratch.arena, path_wild);
    File_Info *info       = file_group.first_file_info;
    
    while (info) {
        String8 name = str8_copy(arena, info->base_name);
        String8 path = str8_copy(arena, info->full_path);
        
        Sampled_Animation anim = {};
        anim.name = name;
        load_sampled_animation(arena, &anim, path);
        
        add(catalog, name, anim);
        
        info = info->next;
    }
}

FUNCTION void control_camera(Camera *cam)
{
    V2 delta_mouse = os->tick_input.mouse_delta;
    f32 dt         = os->dt;
    
    Entity *player = get_player(&game->entity_manager);
    
    if (game->mode == GameMode_GAME) {
        // Camera will focus on this point.
        V3 target = (player->position + 2.0f*V3U);
        
        // Add camera rotation, and clamp pitch to min and max.
        f32 turn_speed    = 1.5f;
        cam->euler.yaw   += -delta_mouse.x * turn_speed;
        cam->euler.pitch +=  delta_mouse.y * turn_speed;
        cam->euler.pitch  = clamp_angle(-65.0f * DEGS_TO_RADS, cam->euler.pitch, 30.0f * DEGS_TO_RADS);
        cam->euler        = normalize_euler(cam->euler);
        
        Quaternion q      = quaternion_from_euler(cam->euler);
        cam->position     = rotate_point_around_pivot(target + cam->start_offset, target, q);
        
        //debug_print("camera: %v3\n", cam->euler * RADS_TO_DEGS);
        
        Quaternion q_look = quaternion_look_at(target - cam->position, V3U);
        cam->object_to_world = m4x4_from_translation_rotation_scale(cam->position, q_look, v3(1));
    }
#if DEVELOPER
    else if (game->mode == GameMode_DEBUG) {
        f32 turn_speed = 2.5f;
        f32 move_speed = 2.0f;
        if (key_held(&os->tick_input, Key_SHIFT))
            move_speed *= 3.0f;
        
        if (key_held(&os->tick_input, Key_MMIDDLE)) {
            Quaternion q_yaw     = quaternion_from_axis_angle(V3U,   turn_speed * -delta_mouse.x);
            cam->orientation     = q_yaw * cam->orientation;
            cam->object_to_world = m4x4_from_translation_rotation_scale(cam->position, cam->orientation, v3(1));
            
            Quaternion q_pitch = quaternion_from_axis_angle(get_right(cam->object_to_world), turn_speed *  delta_mouse.y);
            if(ABS(dot(V3U, q_pitch*get_forward(cam->object_to_world))) < 0.98f) {
                cam->orientation     = q_pitch * cam->orientation;
                cam->object_to_world = m4x4_from_translation_rotation_scale(cam->position, cam->orientation, v3(1));
            }
        }
        
        V3 cam_r = get_right(cam->object_to_world);
        V3 cam_f = get_forward(cam->object_to_world);
        
        if (key_held(&os->tick_input, Key_W)) 
            cam->position +=  cam_f * move_speed * dt;
        if (key_held(&os->tick_input, Key_S))
            cam->position += -cam_f * move_speed * dt;
        if (key_held(&os->tick_input, Key_D))
            cam->position +=  cam_r * move_speed * dt;
        if (key_held(&os->tick_input, Key_A))
            cam->position += -cam_r * move_speed * dt;
        if (key_held(&os->tick_input, Key_E))
            cam->position +=  V3U * move_speed * dt;
        if (key_held(&os->tick_input, Key_Q))
            cam->position += -V3U * move_speed * dt;
        
        cam->object_to_world = m4x4_from_translation_rotation_scale(cam->position, cam->orientation, v3(1));
    }
#endif
    
    set_world_to_view_from_camera(game->camera.object_to_world);
}

FUNCTION void set_game_mode(Game_Mode mode)
{
    Game_Mode current_mode = game->mode;
    
    if (mode == GameMode_GAME) {
#if DEVELOPER
        clear_entity_selection(&game->entity_manager);
        gizmo_clear();
#endif
        
        os->set_cursor_mode(CursorMode_DISABLED);
    }
#if DEVELOPER
    else if (mode == GameMode_DEBUG) {
        if (current_mode == mode) {
            set_game_mode(GameMode_GAME);
            return;
        }
        
        os->set_cursor_mode(CursorMode_NORMAL);
        
        game->camera.orientation = quaternion_from_m4x4(game->camera.object_to_world);
    } 
#endif
    
    game->mode = mode;
}

FUNCTION void game_init()
{
    game = PUSH_STRUCT_ZERO(os->permanent_arena, Game_State);
    
    catalog_init(&game->texture_catalog);
    catalog_init(&game->mesh_catalog);
    catalog_init(&game->animation_catalog);
    
    // Load assets. Textures first because meshes reference them.
    //
    // @Note: Right now we have no maps and no map arena, so we'll just keep everything in memory.
    load_textures(os->permanent_arena, &game->texture_catalog);
    load_meshes(os->permanent_arena, &game->mesh_catalog);
    load_animations(os->permanent_arena, &game->animation_catalog);
    
    game->rng = random_seed(123);
    
    Triangle_Mesh *bot_mesh = find(&game->mesh_catalog, S8LIT("bot"));
    entity_manager_init(&game->entity_manager, bot_mesh);
    
    // @Temporary: Spawn floor.
    Triangle_Mesh *floor_mesh = find(&game->mesh_catalog, S8LIT("floor"));
    register_new_entity(&game->entity_manager, floor_mesh);
    Entity *player   = get_player(&game->entity_manager);
    player->position = {0.0f, 10.0f, 0.0f};
    
    // @Temporary: Line up some bots.
    Sampled_Animation *bot_idle = find(&game->animation_catalog, S8LIT("bot_idle"));
    for (s32 r = 0; r < 5; r++) {
        for (s32 c = 0; c < 3; c++) {
            if (r == 2 && c == 1) continue;
            f32 x     = -3.0f + c * 3.0f;
            f32 z     = -6.0f + r * 3.0f;
            u32 id    = register_new_entity(&game->entity_manager, bot_mesh, EntityType_BOT, S8ZERO, v3(x,0,z));
            Entity *e = find_entity(&game->entity_manager, id);
            play_animation(e, bot_idle);
        }
    }
    
    // @Temporary: Spawn some claws.
    String8 claw_anims[]     = {S8LIT("claw_variation0"), S8LIT("claw_variation1"), S8LIT("claw_variation2"), S8LIT("claw_variation3"), S8LIT("claw_variation4")};
    Triangle_Mesh *claw_mesh = find(&game->mesh_catalog, S8LIT("claw"));
    for (s32 r = 0; r < 31; r++) {
        for (s32 c = 0; c < 21; c++) {
            if (!(r==0 || r==30 || c==0 || c==20)) continue;
            f32 x = -5.0f + c * 0.5f;
            f32 z = -8.0f + r * 0.5f;
            f32 y        = random_rangef(&game->rng, 0.0f, TAU32);
            Quaternion q = quaternion_from_axis_angle(V3U, y);
            u32 id       = register_new_entity(&game->entity_manager, claw_mesh, EntityType_CLAW, S8ZERO, v3(x,0,z), q, v3(0.25f));
            Entity *e    = find_entity(&game->entity_manager, id);
            u32 rand     = random_range(&game->rng, 0, ARRAY_COUNT(claw_anims));
            Sampled_Animation *anim = find(&game->animation_catalog, claw_anims[rand]);
            play_animation(e, anim);
        }
    }
    
    // Init camera.
    game->camera = {};
#if DEVELOPER
    game->camera.orientation  = quaternion_identity();
#endif
    game->camera.start_offset = {0.0f, 1.0f, 3.0f};
    control_camera(&game->camera);
    
    set_view_to_proj();
    
#if DEVELOPER
    set_game_mode(GameMode_DEBUG);
#else
    //set_game_mode(GameMode_MENU);
    set_game_mode(GameMode_GAME);
#endif
    
    // Set initial default dir light.
    game->num_dir_lights = 1;
    game->dir_lights[0]  = default_dir_light();
}

FUNCTION void game_update()
{
#if DEVELOPER
    if (key_pressed(&os->tick_input, Key_F8)){
        set_game_mode(GameMode_DEBUG);
    }
#endif;
    
    control_camera(&game->camera);
    
    game->mouse_world = unproject(game->camera.position, 
                                  1.0f, 
                                  os->mouse_ndc, 
                                  world_to_view_matrix, 
                                  view_to_proj_matrix);
    
    Entity_Manager *manager = &game->entity_manager;
    
    //
    // Update all entities.
    //
    for (s32 i = 0; i < manager->all_entities.count; i++) {
        Entity *e = find_entity(manager, manager->all_entities[i]);
        update_entity(e);
    }
    
#if DEVELOPER
    if (game->mode == GameMode_DEBUG) {
        Ray camera_ray = {game->camera.position, normalize_or_zero(game->mouse_world - game->camera.position)};
        
        // @Note: We have to check if we are interacting with gizmo before doing mouse picking because 
        // we don't want mouse clicks to overlap. Maybe there's a better way to get around this.
        //
        // Process Gizmos
        //
        if (manager->selected_entity) {
            if (key_pressed(&os->tick_input, Key_F1)) gizmo_mode = GizmoMode_TRANSLATION;
            if (key_pressed(&os->tick_input, Key_F2)) gizmo_mode = GizmoMode_ROTATION;
            if (key_pressed(&os->tick_input, Key_F3)) gizmo_mode = GizmoMode_SCALE;
            
            V3 delta_pos;
            Quaternion delta_rot;
            V3 delta_scale;
            gizmo_execute(camera_ray, manager->selected_entity->position, &delta_pos, &delta_rot, &delta_scale);
            
            // Transform all selected entities.
            for (s32 i = 0; i < manager->selected_entities.count; i++) {
                Entity *e = find_entity(manager, manager->selected_entities[i]);
                e->position   += delta_pos;
                e->orientation = delta_rot * e->orientation;
                e->scale      += delta_scale;
            }
            
            // Duplicate selected entities.
            if (gizmo_is_active) {
                if (key_held(&os->tick_input, Key_ALT) && key_pressed(&os->tick_input, Key_MLEFT)) {
                    s32 end_index = (s32)manager->selected_entities.count - 1;
                    for (s32 i = 0; i < (end_index + 1); i++) {
                        Entity *e = find_entity(manager, manager->selected_entities[i]);
                        u32 new_entity_id = register_new_entity(&game->entity_manager, e->mesh, e->type, e->name, e->position, e->orientation);
                        add_entity_selection(manager, new_entity_id);
                    }
                    
                    array_remove_range(&manager->selected_entities, 0, end_index);
                }
            }
            
            // Move camera with gizmo
            if (key_held(&os->tick_input, Key_SHIFT))
                game->camera.position += delta_pos;
        }
        
        //
        // Mouse picking
        //
        if (key_pressed(&os->tick_input, Key_MLEFT) && !gizmo_is_active) {
            f32 sort_index      = F32_MAX;
            u32 best_entity_id  = U32_MAX;
            b32 multi_select    = key_held(&os->tick_input, Key_CONTROL);
            
            // Pick closest entity to camera (iff we intersect with one).
            for (s32 i = 0; i < manager->all_entities.count; i++) {
                Entity *e = find_entity(manager, manager->all_entities[i]);
                Triangle_Mesh *mesh = e->mesh;
                
                /*
                 @Note: Transforming camera ray to entity object space (from https://gamedev.stackexchange.com/questions/72440/the-correct-way-to-transform-a-ray-with-a-matrix):
                        Transforming the ray position and direction by the inverse model transformation is correct. However, many ray-intersection routines assume that the ray direction is a unit vector. If the model transformation involves scaling, the ray direction won't be a unit vector afterward, and should likely be renormalized.
                        
                        However, the distance along the ray returned by the intersection routines will then be measured in model space, and won't represent the distance in world space. If it's a uniform scale, you can simply multiply the returned distance by the scale factor to convert it back to world-space distance. For non-uniform scaling it's trickier; probably the best way is to transform the intersection point back to world space and then re-measure the distance from the ray origin there.
                 
        */
                
                V3 o = transform_point(e->object_to_world.inverse, camera_ray.origin);
                V3 d = transform_vector(e->object_to_world.inverse, camera_ray.direction);
                
                // camera ray in object space of entity mesh.
                Ray ray_object = {o, normalize(d)};
                
                // Early out.
                if (ray_box_intersect(&ray_object, mesh->bounding_box) == FALSE)
                    continue;
                
                V3 *vertices = mesh->vertices.data;
                if (mesh->flags & MeshFlags_ANIMATED) {
                    skin_mesh(e->animation_player);
                    vertices = mesh->skinned_vertices.data;
                }
                
                b32 is_hit = ray_mesh_intersect(&ray_object, mesh->vertices.count, vertices, mesh->indices.count, mesh->indices.data);
                if (is_hit && (ray_object.t < sort_index)) {
                    sort_index  = ray_object.t;
                    best_entity_id = manager->all_entities[i];
                }
            }
            
            if (best_entity_id != U32_MAX) {
                if (multi_select)
                    add_entity_selection(manager, best_entity_id);
                else
                    select_single_entity(manager, best_entity_id);
            } else {
                clear_entity_selection(manager);
                gizmo_clear();
            }
        }
    }
#endif
}

FUNCTION void game_render()
{
    Entity_Manager *manager = &game->entity_manager;
    Entity *player = get_player(manager);
    
    // Render all entities.
    for (s32 i = 0; i < manager->all_entities.count; i++) {
        Entity *e = find_entity(manager, manager->all_entities[i]);
        draw_entity(e);
    }
    
#if DEVELOPER
    draw_editor_ui();
    
    // Draw visual debugging stuff for selected entities.
    if (manager->selected_entity) {
        for (s32 i = 0; i < manager->selected_entities.count; i++) {
            Entity *e = find_entity(manager, manager->selected_entities[i]);
            draw_entity_wireframe(e);
            draw_skeleton(e);
        }
        
        // Render gizmos.
        gizmo_render();
    }
#endif
}