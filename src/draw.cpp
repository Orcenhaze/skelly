
FUNCTION void draw_entity(Entity *e)
{
    Triangle_Mesh *mesh = e->mesh;
    
    if (!mesh) {
        debug_print("Can't draw entity %S; it has no mesh!\n", e->name);
        return;
    }
    
    skeletal_mesh_pbr_bind_shader();
    skeletal_mesh_pbr_bind_buffers(mesh->vbo, mesh->ibo);
    device_context->RSSetState(rasterizer_state_solid);
    
    // Upload vertex constant data.
    PBR_VS_Constants vs_constants = {};
    vs_constants.object_to_proj_matrix  = view_to_proj_matrix.forward * world_to_view_matrix.forward * e->object_to_world.forward;
    vs_constants.object_to_world_matrix = e->object_to_world.forward;
    if (e->animation_player && (e->mesh->flags & MeshFlags_ANIMATED)) {
        u64 matrix_count = e->animation_player->skinning_matrices.count;
        ASSERT(matrix_count <= MAX_JOINTS);
        
        vs_constants.flags |= VSConstantsFlags_SHOULD_SKIN;
        
        MEMORY_COPY(vs_constants.skinning_matrices, e->animation_player->skinning_matrices.data, matrix_count * sizeof(M4x4));
    }
    skeletal_mesh_pbr_upload_vertex_constants(vs_constants);
    
    // Upload pixel constant data.
    PBR_PS_Constants ps_constants = {};
    ps_constants.num_point_lights = game->num_point_lights;
    for (s32 i = 0; i < game->num_point_lights; i++)
        ps_constants.point_lights[i] = game->point_lights[i];
    
    ps_constants.num_dir_lights = game->num_dir_lights;
    for (s32 i = 0; i < game->num_dir_lights; i++)
        ps_constants.dir_lights[i] = game->dir_lights[i];
    
    ps_constants.camera_position = game->camera.position;
    
    // Draw triangle lists.
    for (s32 list_index = 0; list_index < mesh->triangle_list_info.count; list_index++) {
        Triangle_List_Info *list = &mesh->triangle_list_info[list_index];
        Material_Info *m         = &mesh->material_info[list->material_index];
        
        b32 use_normal_map = FALSE;
        V4 base_color      = !nearly_zero(e->color)? e->color : m->base_color;
        f32 metallic       = m->metallic;
        f32 roughness      = m->roughness;
        f32 ao             = m->ambient_occlusion;
        
        // Make shader use texture maps.
        Texture *w = &white_texture;
        if (list->texture_maps[MaterialTextureMapType_NORMAL]   != w) use_normal_map = TRUE;
        if (list->texture_maps[MaterialTextureMapType_ALBEDO]   != w) base_color.x   = -1.0f;
        if (list->texture_maps[MaterialTextureMapType_METALLIC] != w) metallic       = -1.0f;
        if (list->texture_maps[MaterialTextureMapType_ROUGHNESS]!= w) roughness      = -1.0f;
        if (list->texture_maps[MaterialTextureMapType_AO]       != w) ao             = -1.0f;
        
        ps_constants.use_normal_map    = use_normal_map;
        ps_constants.base_color        = base_color.rgb;
        ps_constants.metallic          = metallic;
        ps_constants.roughness         = roughness;
        ps_constants.ambient_occlusion = ao;
        
        // Upload textures.
        for (s32 map_index = 0; map_index < MaterialTextureMapType_COUNT; map_index++)
            skeletal_mesh_pbr_bind_texture(map_index, &list->texture_maps[map_index]->view);
        
        skeletal_mesh_pbr_upload_pixel_constants(ps_constants);
        
        // Draw.
        device_context->DrawIndexed(list->num_indices, list->first_index, 0);
    }
}

#if DEVELOPER
FUNCTION void draw_entity_wireframe(Entity *e)
{
    Triangle_Mesh *mesh = e->mesh;
    
    if (!mesh) {
        debug_print("Can't draw entity %S; it has no mesh!\n", e->name);
        return;
    }
    
    skeletal_mesh_pbr_bind_shader();
    skeletal_mesh_pbr_bind_buffers(mesh->vbo, mesh->ibo);
    device_context->RSSetState(rasterizer_state_wireframe);
    
    // Upload vertex constant data.
    PBR_VS_Constants vs_constants = {};
    vs_constants.object_to_proj_matrix  = view_to_proj_matrix.forward * world_to_view_matrix.forward * e->object_to_world.forward;
    vs_constants.object_to_world_matrix = e->object_to_world.forward;
    if (e->animation_player && (e->mesh->flags & MeshFlags_ANIMATED)) {
        u64 matrix_count = e->animation_player->skinning_matrices.count;
        ASSERT(matrix_count <= MAX_JOINTS);
        
        vs_constants.flags |= VSConstantsFlags_SHOULD_SKIN;
        
        MEMORY_COPY(vs_constants.skinning_matrices, e->animation_player->skinning_matrices.data, matrix_count * sizeof(M4x4));
    }
    skeletal_mesh_pbr_upload_vertex_constants(vs_constants);
    
    // Upload ps constants, default params except for base_color.
    V3  c0            = { 0.89,  0.71, 0.882};
    V3  c1            = {0.929, 0.047, 0.898};
    f32 ct            = ping_pong((f32)os->frame_time, 1.0f);
    V3  outline_color = lerp(c0, ct, c1);
    PBR_PS_Constants ps_constants  = {};
    ps_constants.use_normal_map    = FALSE;
    ps_constants.base_color        = outline_color;
    ps_constants.metallic          = 0.0f;
    ps_constants.roughness         = 0.5f;
    ps_constants.ambient_occlusion = 1.0f;
    skeletal_mesh_pbr_upload_pixel_constants(ps_constants);
    
    // Draw triangle lists.
    for (s32 list_index = 0; list_index < mesh->triangle_list_info.count; list_index++) {
        Triangle_List_Info *list = &mesh->triangle_list_info[list_index];
        device_context->DrawIndexed(list->num_indices, list->first_index, 0);
    }
}

GLOBAL b32 DRAW_JOINT_NAMES = FALSE;
GLOBAL b32 DRAW_JOINT_LINES = FALSE;

FUNCTION void draw_skeleton(Entity *e)
{
    if (!e->animation_player)
        return;
    if (!e->mesh->skeleton)
        return;
    
    Animation_Player *player = e->animation_player;
    Skeleton *skeleton       = e->mesh->skeleton;
    
    ASSERT(player->skinning_matrices.count == skeleton->joint_info.count);
    
    // Draw skeleton lines.
    if (DRAW_JOINT_LINES) {
        for (s32 i = 0; i < player->skinning_matrices.count; i++) {
            s32 parent_index = skeleton->joint_info[i].parent_id;
            
            // @Note: The skinning matrices contain the global joint matrices + the inverse bind pose.
            // We want to get rid of the inverse bind pose part, so we'll just multiply with its inverse
            // to get the identity matrix.
            // @Note: points are in object space.
            M4x4 inv;
            invert(skeleton->joint_info[i].object_to_joint_matrix, &inv);
            V3 p0 = get_translation(player->skinning_matrices[i] * inv);
            
            if (parent_index >= 0) {
                invert(skeleton->joint_info[parent_index].object_to_joint_matrix, &inv);
                V3 p1 = get_translation(player->skinning_matrices[parent_index] * inv);
                
                immediate_begin();
                d3d11_clear_depth();
                set_texture(0);
                set_object_to_world(e->position, e->orientation, e->scale);
                immediate_line(p0, p1, v4(1.0f, 0.8f, 0.4f, 1.0f), 0.011f);
                immediate_end();
            }
            
            immediate_begin();
            set_texture(0);
            set_object_to_world(e->position, e->orientation, e->scale);
            immediate_cube(p0, 0.012f, v4(1.0f, 0.4f, 0.4f, 1.0f));
            immediate_end();
        }
    }
    
    if (DRAW_JOINT_NAMES) {
        // Draw skeleton names.
        for (s32 i = 0; i < player->skinning_matrices.count; i++) {
            M4x4 inv;
            invert(skeleton->joint_info[i].object_to_joint_matrix, &inv);
            V3 p = get_translation(player->skinning_matrices[i] * inv);
            
            V3 p_pixel = world_to_pixel(transform_point(e->object_to_world.forward, p));
            String8 joint_name = skeleton->joint_info[i].name;
            
            immediate_begin();
            d3d11_clear_depth();
            set_texture(&dfont.atlas);
            is_using_pixel_coords = TRUE;
            immediate_text(dfont, p_pixel, 2, v4(0.8f, 0.8f, 0.8f, 1.0f), "%S", joint_name);
            immediate_end();
        }
    }
}
#endif