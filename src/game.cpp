
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
    f32 dt         = os->tick_dt;
    
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
    
    set_view_from_camera(game->camera.object_to_world);
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
    
#if 0
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
#endif
    
    // Init camera.
    game->camera = {};
#if DEVELOPER
    game->camera.orientation  = quaternion_identity();
#endif
    game->camera.start_offset = {0.0f, 1.0f, 3.0f};
    control_camera(&game->camera);
    
    set_projection(90.0f);
    
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

FUNCTION void game_tick_update()
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
        V3 camera_position = game->camera.position;
        V3 camera_end      = camera_position + 300*normalize(game->mouse_world - camera_position);
        
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
            gizmo_execute(camera_position, manager->selected_entity->position, &delta_pos, &delta_rot, &delta_scale);
            
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
                
                V3 a = transform_point(e->object_to_world.inverse, camera_position);
                V3 b = transform_point(e->object_to_world.inverse, camera_end);
                
                // Early out.
                if (segment_aabb_intersect(a, b, mesh->bounding_box.min, mesh->bounding_box.max) == FALSE)
                    continue;
                
                V3 *vertices = mesh->vertices.data;
                if (mesh->flags & MeshFlags_ANIMATED) {
                    skin_mesh(e->animation_player);
                    vertices = mesh->skinned_vertices.data;
                }
                
                Hit_Result hit;
                segment_mesh_intersect(a, b, vertices, mesh->vertices.count, mesh->indices.data, mesh->indices.count, &hit);
                if (hit.result && (hit.percent < sort_index)) {
                    sort_index     = hit.percent;
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

FUNCTION void game_frame_update()
{
    
}

FUNCTION void game_render()
{
    Entity_Manager *manager = &game->entity_manager;
    Entity *player = get_player(manager);
    
#if DEVELOPER
    // Draw viewport grid.
    immediate_begin();
    set_texture(0);
    u32 w  = 32, h = 32;
    f32 hw = (f32)(w*0.5f), hh = (f32)(h*0.5f);
    immediate_grid_2point5d(v3(-hw, 0.0f, hh), V3U, w, h, 1.0f, v4(1,1,1,0.2f), 0.01f);
    immediate_line_2point5d(-V3F*hh, V3F*hh, V3U, v4(0,0,1,0.5f), 0.01f);
    immediate_line_2point5d(-V3R*hw, V3R*hw, V3U, v4(1,0,0,0.5f), 0.01f);
    immediate_end();
    
    //
    // @Temporary:
    // @Temporary: Testing collision!
    // @Temporary:
    //
    LOCAL_PERSIST V3 a1 = {}, b1 = v3(0.0001f), a2 = {}, b2 = {};
    LOCAL_PERSIST s32 scroll;
    scroll += os->frame_input.mouse_wheel_raw;
    V3 p1 = {}, p2 = {};
    f32 dist2 = 0.0f;
#if 0
    //~ lines
    if (key_pressed(&os->frame_input, Key_1)) {
        a1 = game->camera.position;
        b1 = game->mouse_world;
    }
    if (key_pressed(&os->frame_input, Key_2)) {
        a2 = game->camera.position;
        b2 = game->mouse_world;
    }
    
    // Draw lines.
    immediate_begin();
    set_texture(0);
    immediate_line(a1 - 50.0f*(b1-a1), b1 + 50.0f*(b1-a1), v4(0.8,0.8,0.8,1.0), 0.01f);
    immediate_line(a2 - 50.0f*(b2-a2), b2 + 50.0f*(b2-a2), v4(0.4,0.4,0.4,1.0), 0.01f);
    immediate_end();
#endif
#if 0
    //~ line segments
    if (key_pressed(&os->frame_input, Key_1)) {
        a1 = game->camera.position;
        b1 = game->mouse_world;
        b1 = a1 + 3.0f*normalize(b1-a1);
    }
    if (key_pressed(&os->frame_input, Key_2)) {
        a2 = game->camera.position;
        b2 = game->mouse_world;
    }
    
    // Draw line segments.
    immediate_begin();
    set_texture(0);
    immediate_line(a1, b1, v4(0.8,0.8,0.8,1.0), 0.01f);
    immediate_line(a2, b2, v4(0.4,0.4,0.4,1.0), 0.01f);
    immediate_end();
#endif
#if 0
    //~ point - line and point - segment
    V3 point = unproject(game->camera.position, 
                         1.0f + scroll, 
                         os->mouse_ndc, 
                         world_to_view_matrix, 
                         view_to_proj_matrix);
    
    //dist2 = closest_point_line_point(point, a1, b1, NULL, &p1);
    dist2 = closest_point_segment_point(point, a1, b1, NULL, &p1);
    
    immediate_begin();
    set_texture(0);
    immediate_sphere(point, 0.1f, v4(1), 16);
    immediate_line(point, p1, v4(1,0,0,1), 0.01f);
    immediate_end();
    
    // Draw distance as text.
    immediate_begin();
    d3d11_clear_depth();
    set_texture(&dfont.atlas);
    is_using_pixel_coords = TRUE;
    V3 p_pixel = ndc_to_pixel(world_to_ndc(lerp(point, 0.5f, p1)));
    immediate_text(dfont, p_pixel, 3, v4(1), "%f", _sqrt(dist2));
    immediate_end();
#endif
#if 0
    //~ line - line and segment segment
    //dist2 = closest_point_line_line(a1, b1, a2, b2, NULL, &p1, NULL, &p2);
    dist2 = closest_point_segment_segment(a1, b1, a2, b2, NULL, &p1, NULL, &p2);
    
    // Draw closest vector.
    immediate_begin();
    set_texture(0);
    immediate_line(p1, p2, v4(1,0,0,1), 0.01f);
    immediate_end();
    
    // Draw distance as text.
    immediate_begin();
    d3d11_clear_depth();
    set_texture(&dfont.atlas);
    is_using_pixel_coords = TRUE;
    V3 p_pixel = ndc_to_pixel(world_to_ndc(lerp(p1, 0.5f, p2)));
    immediate_text(dfont, p_pixel, 3, v4(1), "%f", _sqrt(dist2));
    immediate_end();
#endif
#if 0
    //~ segment - triangle
    V3 center = {0, 2, 0};
    V3 tri1 = center - V3R, tri2 = center + V3R, tri3 = center + 2*V3U;
    
    LOCAL_PERSIST Quaternion q = quaternion_identity();
    //q = quaternion_from_axis_angle(V3U, 45.0f * DEGS_TO_RADS * os->frame_dt) * q;
    tri1 = rotate_point_around_pivot(tri1, center, q);
    tri2 = rotate_point_around_pivot(tri2, center, q);
    tri3 = rotate_point_around_pivot(tri3, center, q);
    
    Hit_Result hit = {};
    V3 bary;
    b32 intersected = segment_triangle_intersect(a1, b1, tri1, tri2, tri3, &hit, &bary);
    
    debug_print("len= %f, percent= %f\n", length(b1-a1), hit.percent);
    
    // Draw closest vector.
    immediate_begin(TRUE);
    set_texture(0);
    immediate_triangle(tri1, tri2, tri3, v4(1));
    immediate_end();
    
    // Draw hit result stuff.
    V4 color = intersected? v4(0,1,0,1) : v4(1,0,0,1);
    immediate_begin();
    set_texture(0);
    immediate_cube(a1, 0.05f, color);
    immediate_cube(b1, 0.05f, color);
    immediate_cube(hit.impact_point, 0.035f, color);
    immediate_arrow(hit.impact_point, hit.impact_normal, 1.0f, v4(0,0,1,1), 0.01f);
    immediate_end();
    
    // Draw distance as text.
    if (hit.result) {
        immediate_begin();
        d3d11_clear_depth();
        set_texture(&dfont.atlas);
        is_using_pixel_coords = TRUE;
        V3 p_pixel = ndc_to_pixel(world_to_ndc(lerp(a1, 0.5f, hit.impact_point)));
        immediate_text(dfont, p_pixel, 3, color, "%f", length(b1-a1)*hit.percent);
        immediate_end();
    }
#endif
#if 0
    //~ segment - box
    
    LOCAL_PERSIST Quaternion q = quaternion_identity();
    q = quaternion_from_axis_angle(normalize(v3(1, 1, 0)), 45.0f * DEGS_TO_RADS * os->frame_dt) * q;
    
    //Collision_Shape box = make_aabb({-2.0f, 2.0f, -1.0f}, {0.25f, 0.5f, 0.15f});
    Collision_Shape box = make_obb({2.0f, 2.0f, 1.0f}, {0.25f, 0.5f, 0.15f}, q);
    
    Hit_Result hit = {};
    segment_box_intersect(a1, b1, box, &hit);
    
    debug_print("len= %f, percent= %f\n", length(b1-a1), hit.percent);
    
    // Draw box;
    immediate_begin(TRUE);
    set_texture(0);
    immediate_box(box.center, box.half_extents, box.rotation, v4(1));
    immediate_end();
    
    // Draw hit result stuff.
    V4 color = hit.result? v4(0,1,0,1) : v4(1,0,0,1);
    immediate_begin();
    set_texture(0);
    immediate_cube(a1, 0.05f, color);
    immediate_cube(b1, 0.05f, color);
    immediate_cube(hit.impact_point, 0.01f, color);
    immediate_arrow(hit.impact_point, hit.impact_normal, 1.0f, v4(0,0,1,1), 0.01f);
    immediate_end();
    
    // Draw distance as text.
    if (hit.result) {
        immediate_begin();
        d3d11_clear_depth();
        set_texture(&dfont.atlas);
        is_using_pixel_coords = TRUE;
        V3 p_pixel = ndc_to_pixel(world_to_ndc(lerp(a1, 0.5f, hit.impact_point)));
        immediate_text(dfont, p_pixel, 3, color, "%f", length(b1-a1)*hit.percent);
        immediate_end();
    }
#endif
#if 0
    //~ segment - sphere
    Collision_Shape sphere = make_sphere({-3.0f, 2.0f, -2.0f}, 0.5f);
    
    Hit_Result hit = {};
    segment_sphere_intersect(a1, b1, sphere, &hit);
    
    debug_print("len= %f, percent= %f\n", length(b1-a1), hit.percent);
    
    // Draw sphere.
    immediate_begin();
    set_texture(0);
    immediate_sphere(sphere.center, sphere.radius, v4(1), 16);
    immediate_end();
    
    // Draw hit result stuff.
    V4 color = hit.result? v4(0,1,0,1) : v4(1,0,0,1);
    immediate_begin();
    set_texture(0);
    immediate_cube(a1, 0.05f, color);
    immediate_cube(b1, 0.05f, color);
    immediate_cube(hit.impact_point, 0.01f, color);
    immediate_arrow(hit.impact_point, hit.impact_normal, 1.0f, v4(0,0,1,1), 0.01f);
    immediate_arrow(hit.impact_point, reflect(normalize(b1-a1), hit.impact_normal), 1.0f, v4(1,0,1,1), 0.01f);
    immediate_end();
    
    // Draw distance as text.
    if (hit.result) {
        immediate_begin();
        d3d11_clear_depth();
        set_texture(&dfont.atlas);
        is_using_pixel_coords = TRUE;
        V3 p_pixel = ndc_to_pixel(world_to_ndc(lerp(a1, 0.5f, hit.impact_point)));
        immediate_text(dfont, p_pixel, 3, color, "%f", length(b1-a1)*hit.percent);
        immediate_end();
    }
#endif
#if 0
    //~ segment - capsule
    
    LOCAL_PERSIST Quaternion q = quaternion_identity();
    q = quaternion_from_axis_angle(normalize(v3(1, 1, 0)), 45.0f * DEGS_TO_RADS * os->frame_dt) * q;
    
    Collision_Shape capsule = make_capsule(v3(-3.0f, 2.0f, -2.0f), 0.5f, 1.0f, quaternion_identity());
    //Collision_Shape capsule = make_capsule(v3(2.0f, 2.0f, -1.0f), 0.25f, 1.0f, q);
    
    Hit_Result hit = {};
    segment_capsule_intersect(a1, b1, capsule, &hit);
    
    debug_print("len= %f, percent= %f\n", length(b1-a1), hit.percent);
    
    // Draw capsule.
    immediate_begin();
    set_texture(0);
    immediate_debug_capsule(capsule.center, capsule.radius, capsule.half_height, capsule.rotation, v4(1), 16);
    immediate_end();
    
    // Draw hit result stuff.
    V4 color = hit.result? v4(0,1,0,1) : v4(1,0,0,1);
    immediate_begin();
    set_texture(0);
    immediate_cube(a1, 0.05f, color);
    immediate_cube(b1, 0.05f, color);
    immediate_cube(hit.impact_point, 0.01f, color);
    immediate_arrow(hit.impact_point, hit.impact_normal, 1.0f, v4(0,0,1,1), 0.01f);
    immediate_end();
    
    // Draw distance as text.
    if (hit.result) {
        immediate_begin();
        d3d11_clear_depth();
        set_texture(&dfont.atlas);
        is_using_pixel_coords = TRUE;
        V3 p_pixel = ndc_to_pixel(world_to_ndc(lerp(a1, 0.5f, hit.impact_point)));
        immediate_text(dfont, p_pixel, 3, color, "%f", length(b1-a1)*hit.percent);
        immediate_end();
    }
#endif
#if 0
    //~ point - box
    V3 point = unproject(game->camera.position, 
                         1.0f + scroll*0.005f, 
                         os->mouse_ndc, 
                         world_to_view_matrix, 
                         view_to_proj_matrix);
    
    
    LOCAL_PERSIST Quaternion q = quaternion_identity();
    q = quaternion_from_axis_angle(normalize(v3(1, 1, 0)), 45.0f * DEGS_TO_RADS * os->frame_dt) * q;
    
    //Collision_Shape box = make_aabb({-2.0f, 2.0f, -1.0f}, {0.25f, 0.5f, 0.15f});
    Collision_Shape box = make_obb({2.0f, 2.0f, 1.0f}, {0.25f, 0.5f, 0.15f}, q);
    
    dist2 = closest_point_box_point(point, box, &p1);
    
    immediate_begin();
    set_texture(0);
    immediate_box(box.center, box.half_extents, box.rotation, v4(1));
    immediate_sphere(point, 0.05f, quaternion_identity(), v4(1), 16);
    immediate_cube(p1, 0.05f, v4(1,0,0,1));
    immediate_line(point, p1, v4(1), 0.0005f);
    immediate_end();
    
    // Draw distance as text.
    immediate_begin();
    d3d11_clear_depth();
    set_texture(&dfont.atlas);
    is_using_pixel_coords = TRUE;
    V3 p_pixel = ndc_to_pixel(world_to_ndc(lerp(point, 0.5f, p1)));
    immediate_text(dfont, p_pixel, 3, v4(1), "%f", _sqrt(dist2));
    immediate_end();
#endif
#if 0
    //~ box - box
    
    LOCAL_PERSIST Quaternion q = quaternion_identity();
    q = quaternion_from_axis_angle(V3_Z_AXIS, 45.0f * DEGS_TO_RADS);
    //q = quaternion_from_axis_angle(normalize(v3(1, 1, 0)), 45.0f * DEGS_TO_RADS * os->frame_dt) * q;
    
    LOCAL_PERSIST V3 center = {0,0,1};
    if (key_held(&os->frame_input, Key_UP)) {
        center.z -= 0.005f;
    }
    if (key_held(&os->frame_input, Key_DOWN)) {
        center.z += 0.005f;
    }
    if (key_held(&os->frame_input, Key_RIGHT)) {
        center.x += 0.005f;
    }
    if (key_held(&os->frame_input, Key_LEFT)) {
        center.x -= 0.005f;
    }
    if (key_held(&os->frame_input, Key_X)) {
        center.y -= 0.005f;
    }
    if (key_held(&os->frame_input, Key_C)) {
        center.y += 0.005f;
    }
    
    V3 sz  = v3(0.25f);
    V3 sz2 = {0.25f, 0.5f, 0.15f};
    //Collision_Shape box1 = make_aabb({0,0,1}, sz);//v3(0.25f, 1.0f, 1.5f));
    //Collision_Shape box1 = make_aabb(center, sz);//v3(0.25f, 1.0f, 1.5f));
    //Collision_Shape box2 = make_aabb(center, sz);//v3(0.2f, 0.3f, 0.4f));
    Collision_Shape box1 = make_obb(center, sz + V3{0.2f, 0.1f, 0.0f}, q);//v3(0.25f, 1.0f, 1.5f));
    Collision_Shape box2 = make_obb({0,0.25f,0}, sz, q);
    
    Hit_Result hit = {};
    box_box_intersect(box1, box2, &hit);
    if (hit.result)
        center = hit.location;
    
    debug_print("Colliding: %b\n", hit.result);
    
    // Draw boxes.
    immediate_begin(TRUE);
    set_texture(0);
    immediate_box(box1.center, box1.half_extents, box1.rotation, v4(0.8f,0.8f,0.8f,0.5f));
    immediate_box(box2.center, box2.half_extents, box2.rotation, v4(0.3f,0.3f,0.3f,0.5f));
    immediate_end();
    
    // Draw hit result stuff.
    V4 color = hit.result? v4(0,1,0,1) : v4(1,0,0,1);
    immediate_begin();
    set_texture(0);
    immediate_cube(box1.center, 0.01f, v4(0,0,0,1));
    immediate_cube(hit.impact_point, 0.01f, color);
    immediate_cube(hit.location, 0.01f, v4(1,0,1,1));
    immediate_arrow(hit.impact_point, hit.impact_normal, 0.5f, v4(0,0,1,1), 0.002f);
    immediate_end();
#endif
#if 0
    //~ sphere - box
    
    LOCAL_PERSIST Quaternion q = quaternion_identity();
    //q = quaternion_from_axis_angle(V3_Z_AXIS, 45.0f * DEGS_TO_RADS);
    //q = quaternion_from_axis_angle(normalize(v3(1, 1, 0)), 45.0f * DEGS_TO_RADS * os->frame_dt) * q;
    
    LOCAL_PERSIST V3 center = {0,0,1};
    if (key_held(&os->frame_input, Key_UP)) {
        center.z -= 0.005f;
    }
    if (key_held(&os->frame_input, Key_DOWN)) {
        center.z += 0.005f;
    }
    if (key_held(&os->frame_input, Key_RIGHT)) {
        center.x += 0.005f;
    }
    if (key_held(&os->frame_input, Key_LEFT)) {
        center.x -= 0.005f;
    }
    if (key_held(&os->frame_input, Key_X)) {
        center.y -= 0.005f;
    }
    if (key_held(&os->frame_input, Key_C)) {
        center.y += 0.005f;
    }
    
    V3 sz  = v3(0.25f);
    Collision_Shape box    = make_obb({0,0.25f,0}, sz, q);
    Collision_Shape sphere = make_sphere(center, 0.25f);
    
    Hit_Result hit = {};
    sphere_box_intersect(sphere, box, &hit);
    if (hit.result)
        center = hit.location;
    
    debug_print("Colliding: %b\n", hit.result);
    
    // Draw boxes.
    immediate_begin(TRUE);
    set_texture(0);
    immediate_box(box.center, box.half_extents, box.rotation, v4(0.8f,0.8f,0.8f,0.5f));
    immediate_sphere(sphere.center, sphere.radius, quaternion_identity(), v4(0.3f,0.3f,0.3f,0.5f));
    immediate_end();
    
    // Draw hit result stuff.
    V4 color = hit.result? v4(0,1,0,1) : v4(1,0,0,1);
    immediate_begin();
    set_texture(0);
    immediate_cube(sphere.center, 0.01f, v4(0,0,0,1));
    immediate_cube(hit.impact_point, 0.01f, color);
    immediate_cube(hit.location, 0.01f, v4(1,0,1,1));
    immediate_arrow(hit.impact_point, hit.impact_normal, 0.5f, v4(0,0,1,1), 0.01f);
    immediate_arrow(hit.impact_point, hit.normal, 0.5f, v4(0,1,1,1), 0.01f);
    immediate_end();
#endif
#if 0
    //~ capsule - box
    
    LOCAL_PERSIST Quaternion q = quaternion_identity();
    q = quaternion_from_axis_angle(V3_Z_AXIS, 45.0f * DEGS_TO_RADS);
    //q = quaternion_from_axis_angle(normalize(v3(1, 1, 0)), 45.0f * DEGS_TO_RADS * os->frame_dt) * q;
    
    LOCAL_PERSIST V3 center = {0,0,1};
    if (key_held(&os->frame_input, Key_UP)) {
        center.z -= 0.005f;
    }
    if (key_held(&os->frame_input, Key_DOWN)) {
        center.z += 0.005f;
    }
    if (key_held(&os->frame_input, Key_RIGHT)) {
        center.x += 0.005f;
    }
    if (key_held(&os->frame_input, Key_LEFT)) {
        center.x -= 0.005f;
    }
    if (key_held(&os->frame_input, Key_X)) {
        center.y -= 0.005f;
    }
    if (key_held(&os->frame_input, Key_C)) {
        center.y += 0.005f;
    }
    
    V3 sz  = v3(0.25f);
    Collision_Shape box     = make_obb({0,0.25f,0}, sz, q);
    Collision_Shape capsule = make_capsule(center, 0.25f, 1.0f, q);
    
    Hit_Result hit = {};
    capsule_box_intersect(capsule, box, &hit);
    if (hit.result)
        center = hit.location;
    
    debug_print("Colliding: %b\n", hit.result);
    
    // Draw boxes.
    immediate_begin(TRUE);
    set_texture(0);
    immediate_box(box.center, box.half_extents, box.rotation, v4(0.8f,0.8f,0.8f,0.5f));
    immediate_debug_capsule(capsule.center, capsule.radius, capsule.half_height, q, v4(0.3f,0.3f,0.3f,0.5f));
    immediate_end();
    
    // Draw hit result stuff.
    V4 color = hit.result? v4(0,1,0,1) : v4(1,0,0,1);
    immediate_begin();
    set_texture(0);
    immediate_cube(capsule.center, 0.01f, v4(0,0,0,1));
    immediate_cube(hit.impact_point, 0.01f, color);
    immediate_cube(hit.location, 0.01f, v4(1,0,1,1));
    immediate_arrow(hit.impact_point, hit.impact_normal, 0.5f, v4(0,0,1,1), 0.01f);
    immediate_arrow(hit.impact_point, hit.normal, 0.5f, v4(0,1,1,1), 0.01f);
    immediate_end();
#endif
#if 0
    //~ sphere - sphere
    
    LOCAL_PERSIST V3 center = {0,0,1};
    if (key_held(&os->frame_input, Key_UP)) {
        center.z -= 0.005f;
    }
    if (key_held(&os->frame_input, Key_DOWN)) {
        center.z += 0.005f;
    }
    if (key_held(&os->frame_input, Key_RIGHT)) {
        center.x += 0.005f;
    }
    if (key_held(&os->frame_input, Key_LEFT)) {
        center.x -= 0.005f;
    }
    if (key_held(&os->frame_input, Key_X)) {
        center.y -= 0.005f;
    }
    if (key_held(&os->frame_input, Key_C)) {
        center.y += 0.005f;
    }
    
    Collision_Shape a = make_sphere(center, 0.25f);
    Collision_Shape b = make_sphere({0,0.25f,0}, 0.75f);
    
    Hit_Result hit = {};
    sphere_sphere_intersect(a, b, &hit);
    if (hit.result)
        center = hit.location;
    
    debug_print("Colliding: %b\n", hit.result);
    
    // Draw boxes.
    immediate_begin(TRUE);
    set_texture(0);
    immediate_sphere(a.center, a.radius, v4(0.8f,0.8f,0.8f,0.5f));
    immediate_sphere(b.center, b.radius, v4(0.3f,0.3f,0.3f,0.5f));
    immediate_end();
    
    // Draw hit result stuff.
    V4 color = hit.result? v4(0,1,0,1) : v4(1,0,0,1);
    immediate_begin();
    set_texture(0);
    immediate_cube(a.center, 0.01f, v4(0,0,0,1));
    immediate_cube(hit.impact_point, 0.01f, color);
    immediate_cube(hit.location, 0.01f, v4(1,0,1,1));
    immediate_arrow(hit.impact_point, hit.impact_normal, 0.5f, v4(0,0,1,1), 0.01f);
    immediate_arrow(hit.impact_point, hit.normal, 0.5f, v4(0,1,1,1), 0.01f);
    immediate_end();
#endif
#if 0
    //~ sphere - capsule
    
    LOCAL_PERSIST Quaternion q = quaternion_identity();
    //q = quaternion_from_axis_angle(V3_Z_AXIS, 45.0f * DEGS_TO_RADS);
    //q = quaternion_from_axis_angle(normalize(v3(1, 1, 0)), 45.0f * DEGS_TO_RADS * os->frame_dt) * q;
    
    LOCAL_PERSIST V3 center = {0,0,1};
    if (key_held(&os->frame_input, Key_UP)) {
        center.z -= 0.005f;
    }
    if (key_held(&os->frame_input, Key_DOWN)) {
        center.z += 0.005f;
    }
    if (key_held(&os->frame_input, Key_RIGHT)) {
        center.x += 0.005f;
    }
    if (key_held(&os->frame_input, Key_LEFT)) {
        center.x -= 0.005f;
    }
    if (key_held(&os->frame_input, Key_X)) {
        center.y -= 0.005f;
    }
    if (key_held(&os->frame_input, Key_C)) {
        center.y += 0.005f;
    }
    
    Collision_Shape a = make_sphere(center, 0.25f);
    Collision_Shape capsule = make_capsule({0,0.25f,0}, 0.25f, 1.0f, q);
    
    Hit_Result hit = {};
    sphere_capsule_intersect(a, capsule, &hit);
    if (hit.result)
        center = hit.location;
    
    debug_print("Colliding: %b\n", hit.result);
    
    // Draw boxes.
    immediate_begin(TRUE);
    set_texture(0);
    immediate_sphere(a.center, a.radius, v4(0.8f,0.8f,0.8f,0.5f));
    immediate_debug_capsule(capsule.center, capsule.radius, capsule.half_height, q, v4(0.3f,0.3f,0.3f,0.5f));
    immediate_end();
    
    // Draw hit result stuff.
    V4 color = hit.result? v4(0,1,0,1) : v4(1,0,0,1);
    immediate_begin();
    set_texture(0);
    immediate_cube(a.center, 0.01f, v4(0,0,0,1));
    immediate_cube(hit.impact_point, 0.01f, color);
    immediate_cube(hit.location, 0.01f, v4(1,0,1,1));
    immediate_arrow(hit.impact_point, hit.impact_normal, 0.5f, v4(0,0,1,1), 0.01f);
    immediate_arrow(hit.impact_point, hit.normal, 0.5f, v4(0,1,1,1), 0.01f);
    immediate_end();
#endif
#if 0
    //~ capsule - capsule
    
    LOCAL_PERSIST Quaternion q = quaternion_identity();
    q = quaternion_from_axis_angle(V3_Z_AXIS, 45.0f * DEGS_TO_RADS);
    //q = quaternion_from_axis_angle(normalize(v3(1, 1, 0)), 45.0f * DEGS_TO_RADS * os->frame_dt) * q;
    
    LOCAL_PERSIST V3 center = {0,0,1};
    if (key_held(&os->frame_input, Key_UP)) {
        center.z -= 0.005f;
    }
    if (key_held(&os->frame_input, Key_DOWN)) {
        center.z += 0.005f;
    }
    if (key_held(&os->frame_input, Key_RIGHT)) {
        center.x += 0.005f;
    }
    if (key_held(&os->frame_input, Key_LEFT)) {
        center.x -= 0.005f;
    }
    if (key_held(&os->frame_input, Key_X)) {
        center.y -= 0.005f;
    }
    if (key_held(&os->frame_input, Key_C)) {
        center.y += 0.005f;
    }
    
    Collision_Shape a = make_capsule(center, 0.1f, 0.87f, quaternion_identity());
    Collision_Shape b = make_capsule({0,0.25f,0}, 0.25f, 1.0f, q);
    
    Hit_Result hit = {};
    capsule_capsule_intersect(a, b, &hit);
    if (hit.result)
        center = hit.location;
    
    debug_print("Colliding: %b\n", hit.result);
    
    // Draw boxes.
    immediate_begin(TRUE);
    set_texture(0);
    immediate_debug_capsule(a.center, a.radius, a.half_height, a.rotation, v4(0.8f,0.8f,0.8f,0.5f));
    immediate_debug_capsule(b.center, b.radius, b.half_height, b.rotation, v4(0.3f,0.3f,0.3f,0.5f));
    immediate_end();
    
    // Draw hit result stuff.
    V4 color = hit.result? v4(0,1,0,1) : v4(1,0,0,1);
    immediate_begin();
    set_texture(0);
    immediate_cube(a.center, 0.01f, v4(0,0,0,1));
    immediate_cube(hit.impact_point, 0.01f, color);
    immediate_cube(hit.location, 0.01f, v4(1,0,1,1));
    immediate_arrow(hit.impact_point, hit.impact_normal, 0.5f, v4(0,0,1,1), 0.01f);
    immediate_arrow(hit.impact_point, hit.normal, 0.5f, v4(0,1,1,1), 0.01f);
    immediate_end();
#endif
    
#endif
    
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