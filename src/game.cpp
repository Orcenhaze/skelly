
#include "mesh.cpp"
#include "animation.cpp"
#include "entity.cpp"
#include "draw.cpp"
#include "gizmo.cpp"

#if DEVELOPER
#include "editor.cpp"
#endif

FUNCTION void load_textures(Catalog<String8, Texture> *catalog)
{
    Arena_Temp scratch = get_scratch(0, 0);
    defer(free_scratch(scratch));
    
    String8 path_wild = sprint(scratch.arena, "%Stextures/*.*", os->data_folder);
    
    File_Group file_group = os->get_all_files_in_path(scratch.arena, path_wild);
    File_Info *info       = file_group.first_file_info;
    
    while (info) {
        // Use permanent arena for strings to persist.
        String8 name = str8_copy(os->permanent_arena, info->base_name);
        String8 path = str8_copy(os->permanent_arena, info->full_path);
        
        Texture tex  = {};
        d3d11_load_texture(&tex, path);
        
        add(catalog, name, tex);
        
        info = info->next;
    }
}

FUNCTION void load_meshes(Catalog<String8, Triangle_Mesh> *catalog)
{
    Arena_Temp scratch = get_scratch(0, 0);
    defer(free_scratch(scratch));
    
    String8 path_wild = sprint(scratch.arena, "%Smeshes/*.mesh", os->data_folder);
    
    File_Group file_group = os->get_all_files_in_path(scratch.arena, path_wild);
    File_Info *info       = file_group.first_file_info;
    
    while (info) {
        // Use permanent arena for strings to persist.
        String8 name = str8_copy(os->permanent_arena, info->base_name);
        String8 path = str8_copy(os->permanent_arena, info->full_path);
        
        Triangle_Mesh mesh = {};
        mesh.name = name;
        load_triangle_mesh(&mesh, path);
        
        add(catalog, name, mesh);
        
        info = info->next;
    }
}

FUNCTION void load_animations(Catalog<String8, Sampled_Animation> *catalog)
{
    Arena_Temp scratch = get_scratch(0, 0);
    defer(free_scratch(scratch));
    
    String8 path_wild = sprint(scratch.arena, "%Sanimations/*.sampled_animation", os->data_folder);
    
    File_Group file_group = os->get_all_files_in_path(scratch.arena, path_wild);
    File_Info *info       = file_group.first_file_info;
    
    while (info) {
        // Use permanent arena for strings to persist.
        String8 name = str8_copy(os->permanent_arena, info->base_name);
        String8 path = str8_copy(os->permanent_arena, info->full_path);
        
        Sampled_Animation anim = {};
        anim.name = name;
        load_sampled_animation(&anim, path);
        
        add(catalog, name, anim);
        
        info = info->next;
    }
}

FUNCTION void control_camera(Camera *cam)
{
    V3 delta_mouse = game->delta_mouse;
    f32 dt = os->dt;
    
    V3 cam_p = cam->position;
    V3 cam_f = cam->forward;
    V3 cam_r = normalize_or_zero(cross(cam_f, V3U));
    
    f32 speed = 2.0f;
    if (key_held(Key_SHIFT))
        speed *= 3.0f;
    
    if (key_held(Key_W)) 
        cam_p +=  cam_f * speed * dt;
    if (key_held(Key_S))
        cam_p += -cam_f * speed * dt;
    if (key_held(Key_D))
        cam_p +=  cam_r * speed * dt;
    if (key_held(Key_A))
        cam_p += -cam_r * speed * dt;
    if (key_held(Key_E))
        cam_p +=  V3U * speed * dt;
    if (key_held(Key_Q))
        cam_p += -V3U * speed * dt;
    
    if (key_held(Key_MMIDDLE)) {
        Quaternion yaw = quaternion_from_axis_angle(V3U, -speed*delta_mouse.x);
        cam_f = yaw * cam_f;
        
        Quaternion pitch = quaternion_from_axis_angle(cam_r, speed*delta_mouse.y);
        if(ABS(dot(V3U, pitch*cam_f)) < 0.98f)
            cam_f = pitch*cam_f;
    }
    
    cam->position = cam_p;
    cam->forward  = cam_f;
    cam->matrix   = look_at(cam->position, 
                            cam->position + cam->forward, 
                            V3U);
}

FUNCTION void game_init()
{
    game = PUSH_STRUCT_ZERO(os->permanent_arena, Game_State);
    
    catalog_init(&game->texture_catalog);
    catalog_init(&game->mesh_catalog);
    catalog_init(&game->animation_catalog);
    
    entity_manager_init(&game->entity_manager);
    
    set_view_to_proj();
    
    // Init camera.
    game->camera = {{-2.0f, 3.0f, 5.0f}, V3F};
    control_camera(&game->camera);
    set_world_to_view(game->camera.matrix);
    
    // Load assets. Textures first because meshes reference them.
    load_textures(&game->texture_catalog);
    load_meshes(&game->mesh_catalog);
    load_animations(&game->animation_catalog);
    
    // Set initial default dir light.
    game->num_dir_lights = 1;
    game->dir_lights[0]  = default_dir_light();
    
    
    // @Temporary:
    // @Temporary:
    // @Temporary:
    Entity e = create_default_entity();
    e.name = S8LIT("wolf");
    e.mesh   = find(&game->mesh_catalog, S8LIT("wolf"));
    create_animation_player_for_entity(&e);
    
    Entity *wolf = register_new_entity(&game->entity_manager, e);
    play_animation(wolf, find(&game->animation_catalog, S8LIT("wolf_idle")));
}

FUNCTION void game_update()
{
    LOCAL_PERSIST V3 old_ndc = {};
    game->delta_mouse = os->mouse_ndc - old_ndc;
    old_ndc           = os->mouse_ndc;
    
    game->mouse_world = unproject(game->camera.position, 
                                  1.0f, 
                                  os->mouse_ndc, 
                                  world_to_view_matrix, 
                                  view_to_proj_matrix);
    
    control_camera(&game->camera);
    set_world_to_view(game->camera.matrix);
    
    //
    // Update all entities.
    //
    Entity_Manager *manager = &game->entity_manager;
    for (s32 i = 0; i < manager->entities.count; i++) {
        Entity *e = manager->entities[i];
        update_entity_transform(e);
        
        advance_time(e->animation_player, os->dt);
        eval(e->animation_player);
    }
    
    // @Temporary:
    // @Temporary:
    // @Temporary:
    Entity *wolf = find_entity(&game->entity_manager, S8LIT("wolf"));
    Sampled_Animation *anim = find(&game->animation_catalog, S8LIT("wolf_idle"));
    if (key_held(Key_UP)) {
        if (key_held(Key_SHIFT))
            anim = find(&game->animation_catalog, S8LIT("wolf_run"));
        else
            anim = find(&game->animation_catalog, S8LIT("wolf_walk"));
    }
    play_animation(wolf, anim, TRUE, 2.0);
    
    
    
#if DEVELOPER
    Ray camera_ray = {game->camera.position, normalize_or_zero(game->mouse_world - game->camera.position)};
    
    // @Note: We have to check if we are interacting with gizmo before doing mouse picking because 
    // we don't want mouse clicks to overlap. Maybe there's a better way to get around this.
    //
    // Process Gizmos
    //
    if (manager->selected_entity) {
        if (key_pressed(Key_F1)) gizmo_mode = GizmoMode_TRANSLATION;
        if (key_pressed(Key_F2)) gizmo_mode = GizmoMode_ROTATION;
        if (key_pressed(Key_F3)) gizmo_mode = GizmoMode_SCALE;
        V3 delta_pos;
        Quaternion delta_rot;
        V3 delta_scale;
        gizmo_execute(camera_ray, manager->selected_entity->position, &delta_pos, &delta_rot, &delta_scale);
        for (s32 i = 0; i < manager->selected_entities.count; i++) {
            Entity *e = manager->selected_entities[i];
            e->position   += delta_pos;
            e->orientation = delta_rot * e->orientation;
            e->scale      += delta_scale;
        }
    }
    
    //
    // Mouse picking
    //
    if (key_pressed(Key_MLEFT) && !gizmo_is_active) {
        f32 sort_index      = F32_MAX;
        Entity *best_entity = 0;
        b32 multi_select    = key_held(Key_SHIFT);
        
        // Pick closest entity to camera (iff we intersect with one).
        for (s32 i = 0; i < manager->entities.count; i++) {
            Entity *e = manager->entities[i];
            Triangle_Mesh *mesh = e->mesh;
            
            /*
             @Note: Transforming camera ray to entity object space (from https://gamedev.stackexchange.com/questions/72440/the-correct-way-to-transform-a-ray-with-a-matrix):
                    Transforming the ray position and direction by the inverse model transformation is correct. However, many ray-intersection routines assume that the ray direction is a unit vector. If the model transformation involves scaling, the ray direction won't be a unit vector afterward, and should likely be renormalized.
                    
                    However, the distance along the ray returned by the intersection routines will then be measured in model space, and won't represent the distance in world space. If it's a uniform scale, you can simply multiply the returned distance by the scale factor to convert it back to world-space distance. For non-uniform scaling it's trickier; probably the best way is to transform the intersection point back to world space and then re-measure the distance from the ray origin there.
             
    */
            
            V3 o = (e->object_to_world_matrix.inverse * camera_ray.origin);
            V3 d = (e->object_to_world_matrix.inverse * v4(camera_ray.direction, 0.0f)).xyz;
            
            // camera ray in object space of entity mesh.
            Ray ray_object = {o, normalize(d)};
            
            V3 *vertices = mesh->vertices.data;
            if (mesh->flags & MeshFlags_ANIMATED)
                vertices = mesh->skinned_vertices.data;
            
            b32 is_hit = ray_mesh_intersect(&ray_object, mesh->vertices.count, vertices, mesh->indices.count, mesh->indices.data);
            if (is_hit && (ray_object.t < sort_index)) {
                sort_index  = ray_object.t;
                best_entity = e;
            }
        }
        
        if (best_entity) {
            if (multi_select) {
                s32 find_index = array_find_index(&manager->selected_entities, best_entity);
                if (find_index == -1)
                    array_add(&manager->selected_entities, best_entity);
                else
                    array_unordered_remove_by_index(&manager->selected_entities, find_index);
            } else {
                array_reset(&manager->selected_entities);
                array_add(&manager->selected_entities, best_entity);
            }
        } else {
            if (!multi_select)
                array_reset(&manager->selected_entities);
        }
        
        if (manager->selected_entities.count) {
            manager->selected_entity = manager->selected_entities[manager->selected_entities.count-1];
        } else {
            manager->selected_entity = 0;
        }
    }
#endif
}

FUNCTION void game_render()
{
#if DEVELOPER
    // Draw simple wireframe quad instead of viewport grid for now.
    immediate_begin(TRUE);
    set_texture(0);
    immediate_rect_3d({}, V3U, 200.0f, {.8f, .8f, .8f, 0.1f});
    immediate_end();
#endif
    
    // Render all entities.
    Entity_Manager *manager = &game->entity_manager;
    for (s32 i = 0; i < manager->entities.count; i++) {
        Entity *e = manager->entities[i];
        draw_entity(e);
    }
    
    
#if DEVELOPER
    draw_editor_ui();
    
    if (manager->selected_entity) {
        // Draw wireframe for selected entities.
        for (s32 i = 0; i < manager->selected_entities.count; i++) {
            Entity *e = manager->selected_entities[i];
            draw_entity_wireframe(e);
        }
        
        // Render gizmos.
        gizmo_render();
    }
#endif
}