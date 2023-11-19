
#include "mesh.cpp"
#include "entity.cpp"
#include "draw.cpp"

FUNCTION void load_textures(Table<String8, Texture> *table)
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
        
        table_add(table, name, tex);
        
        info = info->next;
    }
}

FUNCTION void load_meshes(Table<String8, Triangle_Mesh> *table)
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
        load_triangle_mesh(&mesh, path);
        
        table_add(table, name, mesh);
        
        info = info->next;
    }
}

FUNCTION void control_camera(Camera *cam, V3 delta_mouse)
{
    f32 dt = os->dt;
    
    V3 cam_p = cam->position;
    V3 cam_f = cam->forward;
    V3 cam_r = normalize0(cross(cam_f, V3U));
    
    f32 speed = 2.5f;
    if (key_held(Key_SHIFT))
        speed *= 2.0f;
    
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
    
    set_view_to_proj();
    
    //
    // Load assets. Textures first because meshes reference them.
    //
    load_textures(&game->texture_catalog);
    load_meshes(&game->mesh_catalog);
    
    // Init camera.
    game->camera = {{}, V3F};
}

FUNCTION void game_update()
{
    LOCAL_PERSIST V3 old_ndc = {};
    V3 delta_mouse = os->mouse_ndc - old_ndc;
    old_ndc        = os->mouse_ndc;
    
    control_camera(&game->camera, delta_mouse);
    set_world_to_view(game->camera.matrix);
}

FUNCTION void game_render()
{
    
}