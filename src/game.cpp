
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
    f32 turn_speed = 15.0f;
    
    Entity *player   = get_player(&game->entity_manager);
    V3 cam_r         = get_right(cam->object_to_world);
    V3 cam_f         = get_forward(cam->object_to_world);
    
    if (cam->mode == CameraMode_GAME) {
        V3 target = (player->position + 2.0f*V3U);
        
        Quaternion q_yaw = quaternion_from_axis_angle(V3U, -turn_speed*delta_mouse.x);
        cam->position  = rotate_point_around_pivot(cam->position, target, q_yaw);
        
        Quaternion q_pitch = quaternion_from_axis_angle(cam_r, -turn_speed*delta_mouse.y);
        
        // Limit the pitch angle!
        f32 pitch_min = -70 * DEGS_TO_RADS;
        f32 pitch_max = +30 * DEGS_TO_RADS;
        f32 pitch     = normalize_half_axis(get_euler(q_pitch * cam->orientation).x);
        if(pitch > pitch_min && pitch < pitch_max) {
            cam->position = rotate_point_around_pivot(cam->position, target, q_pitch);
        }
        
        // Make camera always maintain desired distance from target.
        f32 desired_distance      = 5.0f; // In units.
        V3 desired_camera_postion = target + desired_distance * normalize_or_z_axis(cam->position - target);
        cam->position = move_towards(cam->position, desired_camera_postion, dt, 8.0f);
        
        cam->orientation = quaternion_look_at(target - cam->position, V3U);
        cam->object_to_world = m4x4_from_translation_rotation_scale(cam->position, cam->orientation, v3(1));
    }
#if DEVELOPER
    else if (cam->mode == CameraMode_DEBUG) {
        f32 move_speed = 2.0f;
        if (key_held(&os->tick_input, Key_SHIFT))
            move_speed *= 3.0f;
        
        if (key_held(&os->tick_input, Key_MMIDDLE)) {
            Quaternion yaw   = quaternion_from_axis_angle(V3U,  -turn_speed*delta_mouse.x);
            Quaternion pitch = quaternion_from_axis_angle(cam_r,-turn_speed*delta_mouse.y);
            cam->orientation = yaw * cam->orientation;
            if(ABS(dot(V3U, pitch*cam_f)) < 0.98f)
                cam->orientation = pitch * cam->orientation;
        }
        cam->object_to_world = m4x4_from_translation_rotation_scale(cam->position, cam->orientation, v3(1));
        cam_r = get_right(cam->object_to_world);
        cam_f = get_forward(cam->object_to_world);
        
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
    }
#endif
    
    set_world_to_view_from_camera(game->camera.object_to_world);
}

FUNCTION void toggle_mode()
{
    s32 mode = game->camera.mode == CameraMode_GAME? CameraMode_DEBUG : CameraMode_GAME;
    if (mode == CameraMode_DEBUG)
        os->set_cursor_mode(CursorMode_NORMAL);
    else if (mode == CameraMode_GAME)
        os->set_cursor_mode(CursorMode_DISABLED);
    
    game->camera.mode = mode;
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
    
    // Set initial default dir light.
    game->num_dir_lights = 1;
    game->dir_lights[0]  = default_dir_light();
    
    Triangle_Mesh *player_mesh = find(&game->mesh_catalog, S8LIT("guy"));
    entity_manager_init(&game->entity_manager, player_mesh);
    
    // Init camera.
    game->camera;
    game->camera.position    = {0.0f, 4.0f, 4.0f};
    game->camera.orientation = quaternion_identity();
    game->camera.mode = CameraMode_DEBUG;
    control_camera(&game->camera);
    
    set_view_to_proj();
}

FUNCTION void game_update()
{
#if DEVELOPER
    if (key_pressed(&os->tick_input, Key_ESCAPE)){
        toggle_mode();
    }
#endif;
    
    control_camera(&game->camera);
    
    game->mouse_world = unproject(game->camera.position, 
                                  1.0f, 
                                  os->mouse_ndc, 
                                  world_to_view_matrix, 
                                  view_to_proj_matrix);
    
    Entity_Manager *manager = &game->entity_manager;
    Entity *player = get_player(manager);
    
    //
    // Update all entities.
    //
    for (s32 i = 0; i < manager->all_entities.count; i++) {
        Entity *e = find_entity(manager, manager->all_entities[i]);
        update_entity_transform(e);
        
        advance_time(e->animation_player, os->dt);
        eval(e->animation_player);
    }
    
#if DEVELOPER
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
                    u32 new_entity_id = register_new_entity(&game->entity_manager, e->mesh, e->name, e->position, e->orientation);
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
            
            V3 o = transform_point(e->object_to_world_matrix.inverse, camera_ray.origin);
            V3 d = transform_vector(e->object_to_world_matrix.inverse, camera_ray.direction);
            
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
#endif
}

FUNCTION void game_render()
{
    Entity_Manager *manager = &game->entity_manager;
    
#if DEVELOPER
    // Draw simple wireframe quad instead of viewport grid for now.
    immediate_begin(TRUE);
    set_texture(0);
    immediate_rect_3d({}, V3U, 200.0f, {0.8f, 0.8f, 0.8f, 1.0f});
    immediate_end();
    
    Entity *player = get_player(manager);
    immediate_begin();
    V3 o = player->position;
    V3 x = get_right(player->object_to_world_matrix.forward);
    V3 y = get_up(player->object_to_world_matrix.forward);
    V3 z = -get_forward(player->object_to_world_matrix.forward);
    immediate_line_3d(o, o + 2.0f * x, v4(1, 0, 0, 1));
    immediate_line_3d(o, o + 2.0f * y, v4(0, 1, 0, 1));
    immediate_line_3d(o, o + 2.0f * z, v4(0, 0, 1, 1));
    immediate_end();
#endif
    
    // Render all entities.
    for (s32 i = 0; i < manager->all_entities.count; i++) {
        Entity *e = find_entity(manager, manager->all_entities[i]);
        //draw_entity(e);
    }
    
    
#if DEVELOPER
    draw_editor_ui();
    
    // Draw visual debugging stuff for selected entities.
    if (manager->selected_entity) {
        for (s32 i = 0; i < manager->selected_entities.count; i++) {
            Entity *e = find_entity(manager, manager->selected_entities[i]);
            draw_entity_wireframe(e);
        }
        
        // Render gizmos.
        gizmo_render();
    }
    
    // Draw skeleton lines and joint names
    for (s32 i = 0; i < manager->all_entities.count; i++) {
        Entity *e = find_entity(manager, manager->all_entities[i]);
        draw_skeleton(e);
    }
#endif
}