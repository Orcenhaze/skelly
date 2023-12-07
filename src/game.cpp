
#include "mesh.cpp"
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
    
    String8 path_wild = sprint(scratch.arena, "%Stextures/*.png", os->data_folder);
    
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

FUNCTION void control_camera(Camera *cam)
{
    V3 delta_mouse = game->delta_mouse;
    f32 dt = os->dt;
    
    V3 cam_p = cam->position;
    V3 cam_f = cam->forward;
    V3 cam_r = normalize0(cross(cam_f, V3U));
    
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
    
    entity_manager_init(&game->entity_manager);
    
    set_view_to_proj();
    
    // Init camera.
    game->camera = {{0.0f, 0.0f, 2.0f}, V3F};
    
    // Load assets. Textures first because meshes reference them.
    load_textures(&game->texture_catalog);
    load_meshes(&game->mesh_catalog);
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
        Entity *e = &manager->entities[i];
        update_entity_transform(e);
    }
    
#if DEVELOPER
    //
    // Mouse picking
    //
    Ray camera_ray = {game->camera.position, normalize0(game->mouse_world - game->camera.position)};
    f32 sort_index = F32_MAX;
    
    manager->hovered_entity = 0;
    
    for (s32 i = 0; i < manager->entities.count; i++) {
        Entity *e = &manager->entities[i];
        
        /*
@Todo: Use ray-mesh intersection for more accurate picking!

         @Note: Transforming camera ray to entity object space (from https://gamedev.stackexchange.com/questions/72440/the-correct-way-to-transform-a-ray-with-a-matrix):
                Transforming the ray position and direction by the inverse model transformation is correct. However, many ray-intersection routines assume that the ray direction is a unit vector. If the model transformation involves scaling, the ray direction won't be a unit vector afterward, and should likely be renormalized.
                
                However, the distance along the ray returned by the intersection routines will then be measured in model space, and won't represent the distance in world space. If it's a uniform scale, you can simply multiply the returned distance by the scale factor to convert it back to world-space distance. For non-uniform scaling it's trickier; probably the best way is to transform the intersection point back to world space and then re-measure the distance from the ray origin there.
         
*/
        
        V3 o = (e->object_to_world_matrix.inverse * camera_ray.origin);
        V3 d = (e->object_to_world_matrix.inverse * v4(camera_ray.direction, 0.0f)).xyz;
        
        Ray ray = {o, normalize(d)};
        
        if (ray_box_intersect(&ray, e->mesh->bounding_box)) {
            if (camera_ray.t < sort_index) {
                sort_index              = camera_ray.t;
                manager->hovered_entity = e;
            }
        }
    }
    
    // @Todo: If ctrl- was pressed, we should add it to a group?
    //
    if (manager->hovered_entity && key_pressed(Key_MLEFT)) {
        manager->selected_entity = manager->hovered_entity;
    }
    
    //
    // Process Gizmos
    //
    if (key_pressed(Key_F1)) gizmo_mode = GizmoMode_TRANSLATION;
    if (key_pressed(Key_F2)) gizmo_mode = GizmoMode_ROTATION;
    if (key_pressed(Key_F3)) gizmo_mode = GizmoMode_SCALE;
    if (manager->selected_entity) {
        gizmo_execute(camera_ray, manager->selected_entity);
    }
#endif
}

FUNCTION void game_render()
{
    // Render all entities.
    Entity_Manager *manager = &game->entity_manager;
    for (s32 i = 0; i < manager->entities.count; i++) {
        Entity *e = &manager->entities[i];
        
        draw_entity(e);
    }
    
    
#if DEVELOPER
    draw_editor();
    
#if 0
    if (manager->hovered_entity) {
        // Draw bounding_box of mesh.
        immediate_begin(TRUE);
        set_texture(0);
        set_object_to_world(manager->hovered_entity->position, manager->hovered_entity->orientation, manager->hovered_entity->scale);
        
        immediate_cuboid(get_center(manager->hovered_entity->mesh->bounding_box), 
                         0.5f*get_size(manager->hovered_entity->mesh->bounding_box), 
                         {0,1,1,1});
        immediate_end();
    }
#endif
    
    //
    // Render Gizmos.
    //
    if (manager->selected_entity) {
        gizmo_render();
    }
#endif
}