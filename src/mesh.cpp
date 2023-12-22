
FUNCTION void load_mesh_data(Arena *arena, Triangle_Mesh *mesh, String8 file)
{
    s32 version = 0;
    get(&file, &version);
    ASSERT(version <= MESH_FILE_VERSION);
    
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
    
    //
    // Read Skeleton
    //
    mesh->skeleton.exists = FALSE;
    if (header.num_skeleton_joints) {
        Skeleton *sk = &mesh->skeleton;
        sk->exists   = TRUE;
        array_init_and_resize(&sk->joint_info, header.num_skeleton_joints);
        
        for (s32 i = 0; i < header.num_skeleton_joints; i++) {
            Skeleton_Joint_Info *joint = &sk->joint_info[i];
            
            // Read 4x4 matrix.
            get(&file, &joint->inverse_rest_pose);
            
            /* 
                        // @Hack:
                        if (joint->inverse_rest_pose._44 == 0) {
                            joint->inverse_rest_pose = m4x4_identity();
                        }
             */
            
            // Read name
            // @Todo: Maybe lifetime of these names shouldn't be permanent...
            s32 joint_name_len = 0;
            get(&file, &joint_name_len);
            String8 joint_name = str8(file.data, joint_name_len);
            advance(&file, joint_name_len);
            joint->name = str8_copy(os->permanent_arena, joint_name);
            
            // Read parent id.
            get(&file, &joint->parent_id);
        }
        
        s32 num_canonical_vertices;
        get(&file, &num_canonical_vertices);
        ASSERT(num_canonical_vertices > 0);
        array_init_and_resize(&sk->vertex_blend_info, num_canonical_vertices);
        
        for (s32 i = 0; i < num_canonical_vertices; i++) {
            Vertex_Blend_Info *blend = &sk->vertex_blend_info[i];
            
            get(&file, &blend->num_pieces);
            
            for (s32 piece_index = 0; piece_index < blend->num_pieces; piece_index++) {
                get(&file, &blend->pieces[piece_index].joint_id);
                get(&file, &blend->pieces[piece_index].weight);
            }
        }
        
        
        // @Temporary:
        // @Temporary:
        // @Temporary:
        // @Temporary:
        array_init_and_resize(&mesh->skinned_vertices, header.num_vertices);
        array_init_and_resize(&mesh->skinned_normals, header.num_vertices);
    }
    
    
    ASSERT(file.count == 0);
    
    // @Todo: @Speed: Not sure if there's a faster way to construct this... maybe when reading vertex_blends?
    //
    // Construct canonical_vertex_map.
    {
        Array<V3> canonical_vertices;
        defer(array_free(&canonical_vertices));
        array_init_and_reserve(&canonical_vertices, mesh->vertices.count);
        for (s32 i = 0; i < mesh->vertices.count; i++)
            array_add_unique(&canonical_vertices, mesh->vertices[i]);
        
        array_init_and_resize(&mesh->canonical_vertex_map, mesh->vertices.count);
        for (s32 i = 0; i < mesh->vertices.count; i++) {
            s32 find_index = array_find_index(&canonical_vertices, mesh->vertices[i]);
            ASSERT(find_index >= 0);
            
            mesh->canonical_vertex_map[i] = find_index;
        }
    }
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
                map = table_find_pointer(&game->texture_catalog.table, map_name);
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
    
    // @Temporary:
    // @Temporary:
    // @Temporary:
    // @Temporary:
    b32 animated = mesh->skeleton.exists;
    
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (UINT)(sizeof(Vertex_XTBNUC) * num_vertices);
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.Usage     = D3D11_USAGE_IMMUTABLE;
    if (animated) {
        desc.Usage          = D3D11_USAGE_DYNAMIC;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }
    D3D11_SUBRESOURCE_DATA vertex_data = { vertex_buffer };
    device->CreateBuffer(&desc, &vertex_data, &mesh->vbo);
    
    D3D11_BUFFER_DESC desc2 = {};
    desc2.ByteWidth = (UINT)(sizeof(u32) * mesh->indices.count);
    desc2.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc2.Usage     = D3D11_USAGE_IMMUTABLE;
    D3D11_SUBRESOURCE_DATA index_data = { mesh->indices.data };
    device->CreateBuffer(&desc2, &index_data, &mesh->ibo);
    
    free_scratch(scratch);
}

FUNCTION void generate_bounding_box_for_mesh(Triangle_Mesh *mesh)
{
    V3 min = v3(F32_MAX);
    V3 max = v3(F32_MIN);
    
    for (s32 i = 0; i < mesh->vertices.count; i++) {
        V3 v = mesh->vertices[i];
        
        if(v.x < min.x) min.x = v.x;
        if(v.x > max.x) max.x = v.x;
        
        if(v.y < min.y) min.y = v.y;
        if(v.y > max.y) max.y = v.y;
        
        if(v.z < min.z) min.z = v.z;
        if(v.z > max.z) max.z = v.z;
    }
    
    mesh->bounding_box = {min, max};
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
    generate_bounding_box_for_mesh(mesh);
    
    free_scratch(scratch);
    os->free_file_memory(orig_file.data);
}