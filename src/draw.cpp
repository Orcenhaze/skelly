
FUNCTION void draw_entity(Entity *e)
{
    Triangle_Mesh *mesh = e->mesh;
    
    if (!mesh) {
        debug_print("Can't draw entity %S; it has no mesh!\n", e->name);
        return;
    }
    
    // @Temporary: Should skin on the GPU
    // @Temporary:
    // @Temporary:
    if (e->animation_player) {
        if ((mesh->flags & MeshFlags_ANIMATED) && mesh->skeleton) {
            if (e->animation_player->num_changed_channels_last_eval)
                skin_mesh(e->animation_player);
        }
    }
    
    // Bind Input Assembler.
    device_context->IASetInputLayout(pbr_input_layout);
    device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    UINT stride = sizeof(Vertex_XTBNUC);
    UINT offset = 0;
    device_context->IASetVertexBuffers(0, 1, &mesh->vbo, &stride, &offset);
    device_context->IASetIndexBuffer(mesh->ibo, DXGI_FORMAT_R32_UINT, 0);
    
    // Vertex Shader.
    // Upload vs constants.
    PBR_VS_Constants vs_constants = {};
    vs_constants.object_to_proj_matrix  = view_to_proj_matrix.forward * world_to_view_matrix.forward * e->object_to_world_matrix.forward;
    vs_constants.object_to_world_matrix = e->object_to_world_matrix.forward;
    
    D3D11_MAPPED_SUBRESOURCE mapped;
    device_context->Map(pbr_vs_cbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    *(PBR_VS_Constants*)mapped.pData = vs_constants;
    device_context->Unmap(pbr_vs_cbuffer, 0);
    
    device_context->VSSetConstantBuffers(0, 1, &pbr_vs_cbuffer);
    device_context->VSSetShader(pbr_vs, 0, 0);
    
    // Rasterizer Stage.
    device_context->RSSetViewports(1, &viewport);
    device_context->RSSetState(rasterizer_state_solid);
    
    // Pixel Shader.
    device_context->PSSetSamplers(0, 1, &sampler_linear);
    device_context->PSSetShader(pbr_ps, 0, 0);
    
    // Output Merger.
    device_context->OMSetBlendState(default_blend_state, 0, 0XFFFFFFFFU);
    device_context->OMSetDepthStencilState(default_depth_state, 0);
    device_context->OMSetRenderTargets(1, &render_target_view, depth_stencil_view);
    
    // Upload ps constants.
    PBR_PS_Constants ps_constants = {};
    
    // Set point lights from scene.
    ps_constants.num_point_lights = game->num_point_lights;
    for (s32 i = 0; i < game->num_point_lights; i++) {
        ps_constants.point_lights[i] = game->point_lights[i];
    }
    
    // Set directional lights from scene.
    ps_constants.num_dir_lights = game->num_dir_lights;
    for (s32 i = 0; i < game->num_dir_lights; i++) {
        ps_constants.dir_lights[i] = game->dir_lights[i];
    }
    
    ps_constants.camera_position = game->camera.position;
    
    // Draw triangle lists.
    for (s32 list_index = 0; list_index < mesh->triangle_list_info.count; list_index++) {
        Triangle_List_Info *list = &mesh->triangle_list_info[list_index];
        Material_Info *m         = &mesh->material_info[list->material_index];
        
        b32 use_normal_map = FALSE;
        V4 base_color      = m->base_color;
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
        
        D3D11_MAPPED_SUBRESOURCE mapped;
        device_context->Map(pbr_ps_cbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        *(PBR_PS_Constants*)mapped.pData = ps_constants;
        device_context->Unmap(pbr_ps_cbuffer, 0);
        
        device_context->PSSetConstantBuffers(1, 1, &pbr_ps_cbuffer);
        
        // Upload textures.
        for (s32 map_index = 0; map_index < MaterialTextureMapType_COUNT; map_index++)
            device_context->PSSetShaderResources(map_index, 1, &list->texture_maps[map_index]->view);
        
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
    
    // Bind Input Assembler.
    device_context->IASetInputLayout(pbr_input_layout);
    device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    UINT stride = sizeof(Vertex_XTBNUC);
    UINT offset = 0;
    device_context->IASetVertexBuffers(0, 1, &mesh->vbo, &stride, &offset);
    device_context->IASetIndexBuffer(mesh->ibo, DXGI_FORMAT_R32_UINT, 0);
    
    // Vertex Shader.
    // Upload vs constants.
    PBR_VS_Constants vs_constants = {};
    vs_constants.object_to_proj_matrix  = view_to_proj_matrix.forward * world_to_view_matrix.forward * e->object_to_world_matrix.forward;
    vs_constants.object_to_world_matrix = e->object_to_world_matrix.forward;
    
    D3D11_MAPPED_SUBRESOURCE mapped;
    device_context->Map(pbr_vs_cbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    *(PBR_VS_Constants*)mapped.pData = vs_constants;
    device_context->Unmap(pbr_vs_cbuffer, 0);
    
    device_context->VSSetConstantBuffers(0, 1, &pbr_vs_cbuffer);
    device_context->VSSetShader(pbr_vs, 0, 0);
    
    // Rasterizer Stage.
    device_context->RSSetViewports(1, &viewport);
    device_context->RSSetState(rasterizer_state_wireframe);
    
    // Pixel Shader.
    device_context->PSSetSamplers(0, 1, &sampler_linear);
    device_context->PSSetShader(pbr_ps, 0, 0);
    
    // Output Merger.
    device_context->OMSetBlendState(default_blend_state, 0, 0XFFFFFFFFU);
    device_context->OMSetDepthStencilState(default_depth_state, 0);
    device_context->OMSetRenderTargets(1, &render_target_view, depth_stencil_view);
    
    // Upload ps constants, default params except for base_color.
    V3  c0            = { 0.89,  0.71, 0.882};
    V3  c1            = {0.929, 0.047, 0.898};
    f32 ct            = ping_pong((f32)os->time, 1.0f);
    V3  outline_color = lerp(c0, ct, c1);
    
    PBR_PS_Constants ps_constants = {};
    ps_constants.use_normal_map    = FALSE;
    ps_constants.base_color        = outline_color;
    ps_constants.metallic          = 0.0f;
    ps_constants.roughness         = 0.5f;
    ps_constants.ambient_occlusion = 1.0f;
    
    device_context->Map(pbr_ps_cbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    *(PBR_PS_Constants*)mapped.pData = ps_constants;
    device_context->Unmap(pbr_ps_cbuffer, 0);
    
    device_context->PSSetConstantBuffers(1, 1, &pbr_ps_cbuffer);
    
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
                immediate_line_3d(p0, p1, v4(1.0f, 0.8f, 0.4f, 1.0f), 0.011f);
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
            
            V3 p_ndc   = world_to_ndc(e->object_to_world_matrix.forward * p);
            V3 p_pixel = ndc_to_pixel(p_ndc);
            String8 joint_name = skeleton->joint_info[i].name;
            
            immediate_begin();
            d3d11_clear_depth();
            set_texture(&dfont.atlas);
            is_using_pixel_coords = TRUE;
            immediate_text(&dfont, p_pixel, 2, v4(0.8f, 0.8f, 0.8f, 1.0f), "%S", joint_name);
            immediate_end();
        }
    }
}
#endif