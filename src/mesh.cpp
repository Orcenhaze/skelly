
FUNCTION void load_mesh_data(Arena *arena, Triangle_Mesh *mesh, String8 file)
{
    s32 version = 0;
    get(&file, &version);
    ASSERT(version == MESH_FILE_VERSION);
    
    Triangle_Mesh_Header header = {};
    get(&file, &header);
    ASSERT(header.num_vertices != 0);
    
    // @Important:
    // @Memory @Todo: Each array will have its own arena. This will end up wasting ALOT of virtual memory.
    // Maybe find a way to force array arena to only reserve specific amount?
    //
    array_init_and_resize(&mesh->vertices, header.num_vertices);
    array_init_and_resize(&mesh->tbns,     header.num_tbns);
    array_init_and_resize(&mesh->uvs,      header.num_uvs);
    array_init_and_resize(&mesh->colors,   header.num_colors);
    array_init_and_resize(&mesh->indices,  header.num_indices);
    
    for (s32 i = 0; i < mesh->vertices.count; i++)
        get(&file, &mesh->vertices[i]);
    for (s32 i = 0; i < mesh->tbns.count; i++)
        get(&file, &mesh->tbns[i]);
    for (s32 i = 0; i < mesh->uvs.count; i++)
        get(&file, &mesh->uvs[i]);
    for (s32 i = 0; i < mesh->colors.count; i++)
        get(&file, &mesh->colors[i]);
    for (s32 i = 0; i < mesh->indices.count; i++)
        get(&file, &mesh->indices[i]);
    
    array_init_and_resize(&mesh->triangle_list_info, header.num_triangle_lists);
    array_init_and_resize(&mesh->material_info,      header.num_materials);
    
    for (s32 i = 0; i < mesh->triangle_list_info.count; i++) {
        get(&file, &mesh->triangle_list_info[i].material_index);
        get(&file, &mesh->triangle_list_info[i].num_indices);
        get(&file, &mesh->triangle_list_info[i].first_index);
    }
    
    for (s32 i = 0; i < mesh->material_info.count; i++) {
        // Get default values of material attributes.
        get(&file, &mesh->material_info[i].base_color);
        get(&file, &mesh->material_info[i].metallic);
        get(&file, &mesh->material_info[i].roughness);
        
        // @Note: Blender doesn't seem to have ambient occlusion attribute in Principled BSDF, so we'll set its default value manually.
        mesh->material_info[i].ambient_occlusion = 1.0f;
        
        for (s32 map_index = 0; map_index < MaterialTextureMapType_COUNT; map_index++) {
            s32 map_name_len = 0;
            get(&file, &map_name_len);
            
            // Exporter includes extension in the file name. We only want the base name.
            String8 map_name = extract_base_name(str8(file.data, map_name_len));
            advance(&file, map_name_len);
            
            mesh->material_info[i].texture_map_names[map_index] = str8_copy(arena, map_name);
        }
    }
    
    ASSERT(file.count == 0);
}

FUNCTION void load_mesh_textures(Triangle_Mesh *mesh)
{
    for (s32 list_index = 0; list_index < mesh->triangle_list_info.count; list_index++) {
        Triangle_List_Info *list = &mesh->triangle_list_info[list_index];
        Material_Info *m         = &mesh->material_info[list->material_index];
        
        for (s32 map_index = 0; map_index < MaterialTextureMapType_COUNT; map_index++) {
            String8 map_name = m->texture_map_names[map_index];
            
            Texture *map = &white_texture;
            if (!str8_empty(map_name)) {
                map = table_find_pointer(&game->texture_catalog, map_name);
            }
            
            if (!map) {
                debug_print("MESH TEXTURE LOAD ERROR: Couldn't find texture %S\n", map_name);
                list->texture_maps[map_index] = &white_texture;
                continue;
            }
            
            list->texture_maps[map_index] = map;
        }
    }
}

FUNCTION void generate_buffers_for_mesh(Triangle_Mesh *mesh)
{
    s64 num_vertices = mesh->vertices.count;
    
    Arena_Temp scratch = get_scratch(0, 0);
    Vertex_XTBNUC *vertex_buffer = PUSH_ARRAY_ZERO(scratch.arena, Vertex_XTBNUC, num_vertices);
    Vertex_XTBNUC *vertex = vertex_buffer;
    
    for (s32 vindex = 0; vindex < num_vertices; vindex++, vertex++) {
        if (mesh->vertices.count) vertex->position = mesh->vertices[vindex];
        else                      vertex->position = {};
        
        if (mesh->tbns.count)     vertex->tangent = mesh->tbns[vindex].tangent;
        else                      vertex->tangent = {1.0f, 0.0f, 0.0f};
        
        if (mesh->tbns.count)     vertex->bitangent = mesh->tbns[vindex].bitangent;
        else                      vertex->bitangent = {0.0f, 1.0f, 0.0f};
        
        if (mesh->tbns.count)     vertex->normal = mesh->tbns[vindex].normal;
        else                      vertex->normal = {0.0f, 0.0f, 1.0f};
        
        if (mesh->uvs.count)      vertex->uv = mesh->uvs[vindex];
        else                      vertex->uv = {};
        
        if (mesh->colors.count)   vertex->color = mesh->colors[vindex];
        else                      vertex->color = {1.0f, 1.0f, 1.0f, 1.0f};
    }
    
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (UINT)(sizeof(Vertex_XTBNUC) * num_vertices);
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.Usage     = D3D11_USAGE_IMMUTABLE;
    D3D11_SUBRESOURCE_DATA vertex_data = { vertex_buffer };
    device->CreateBuffer(&desc, &vertex_data, &mesh->vbo);
    
    D3D11_BUFFER_DESC desc2 = desc;
    desc2.ByteWidth = (UINT)(sizeof(u32) * mesh->indices.count);
    desc2.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA index_data = { mesh->indices.data };
    device->CreateBuffer(&desc2, &index_data, &mesh->ibo);
    
    free_scratch(scratch);
}

FUNCTION void load_triangle_mesh(Triangle_Mesh *mesh, String8 full_path)
{
    mesh->full_path = full_path;
    
    String8 file = os->read_entire_file(full_path);
    if (!file.data) {
        debug_print("MESH LOAD ERROR: Couldn't load mesh at %S\n", full_path);
        return;
    }
    
    String8 orig_file  = file;
    Arena_Temp scratch = get_scratch(0, 0);
    
    load_mesh_data(scratch.arena, mesh, file);
    load_mesh_textures(mesh);
    generate_buffers_for_mesh(mesh);
    
    free_scratch(scratch);
    os->free_file_memory(orig_file.data);
}