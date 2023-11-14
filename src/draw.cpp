
FUNCTION void draw_entity(Entity *e)
{
    Triangle_Mesh *mesh = e->mesh;
    
    if (!mesh) {
        debug_print("Entity %S has no mesh!\n", e->name);
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
    device_context->RSSetState(rasterizer_state_solid);
    
    // Pixel Shader.
    device_context->PSSetSamplers(0, 1, &sampler_linear);
    device_context->PSSetShader(pbr_ps, 0, 0);
    
    // Output Merger.
    device_context->OMSetBlendState(default_blend_state, 0, 0XFFFFFFFFU);
    device_context->OMSetDepthStencilState(default_depth_state, 0);
    device_context->OMSetRenderTargets(1, &render_target_view, depth_stencil_view);
    
    // Draw triangle lists.
    for (s32 list_index = 0; list_index < mesh->triangle_list_info.count; list_index++) {
        Triangle_List_Info *list = &mesh->triangle_list_info[list_index];
        Material_Info *m         = &mesh->material_info[list->material_index];
        
        b32 use_normal_map = FALSE;
        V4 base_color      = m->base_color;
        f32 metallic       = m->metallic;
        f32 roughness      = m->roughness;
        f32 ao             = m->ambient_occlusion;
        
        auto *v = white_texture.view;
        if (list->texture_maps[MaterialTextureMapType_NORMAL].view!= v)    use_normal_map = TRUE;
        if (list->texture_maps[MaterialTextureMapType_ALBEDO].view!= v)    base_color.x   = -1.0f;
        if (list->texture_maps[MaterialTextureMapType_METALLIC].view!= v)  metallic       = -1.0f;
        if (list->texture_maps[MaterialTextureMapType_ROUGHNESS].view!= v) roughness      = -1.0f;
        if (list->texture_maps[MaterialTextureMapType_AO].view!= v)        ao             = -1.0f;
        
        // Upload ps constants.
        PBR_PS_Constants ps_constants = {};
        ps_constants.camera_position   = camera.position;
        ps_constants.light_position    = {1.5f, 0.3f, 2.5f};
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
            device_context->PSSetShaderResources(map_index, 1, &list->texture_maps[map_index].view);
        
        // Draw.
        device_context->DrawIndexed(list->num_indices, list->first_index, 0);
    }
}