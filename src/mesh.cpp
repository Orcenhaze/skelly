
FUNCTION void load_mesh_data(Arena *arena, Triangle_Mesh *mesh, String8 file)
{
    s32 version = 0;
    get(&file, &version);
    ASSERT(version <= MESH_FILE_VERSION);
    
    Triangle_Mesh_Header header = {};
    get(&file, &header);
    ASSERT(header.num_vertices != 0);
    
    // Vertex data (exported in XTBNUC form)
    array_init_and_resize(&mesh->vertices, header.num_vertices);
    array_init_and_resize(&mesh->tbns,     header.num_vertices);
    array_init_and_resize(&mesh->uvs,      header.num_vertices);
    array_init_and_resize(&mesh->colors,   header.num_vertices);
    for (s32 i = 0; i < header.num_vertices; i++) {
        get(&file, &mesh->vertices[i]);
        get(&file, &mesh->tbns[i]);
        get(&file, &mesh->uvs[i]);
        get(&file, &mesh->colors[i]);
    }
    
    // Canonical vertex map.
    array_init_and_resize(&mesh->canonical_vertex_map, header.num_vertices);
    for (s32 i = 0; i < header.num_vertices; i++) {
        get(&file, &mesh->canonical_vertex_map[i]);
    }
    
    // Indices
    array_init_and_resize(&mesh->indices,  header.num_indices);
    for (s32 i = 0; i < header.num_indices; i++)
        get(&file, &mesh->indices[i]);
    
    // Triangle list info
    array_init_and_resize(&mesh->triangle_list_info, header.num_triangle_lists);
    for (s32 i = 0; i < mesh->triangle_list_info.count; i++) {
        get(&file, &mesh->triangle_list_info[i].material_index);
        get(&file, &mesh->triangle_list_info[i].num_indices);
        get(&file, &mesh->triangle_list_info[i].first_index);
    }
    
    // Material info
    array_init_and_resize(&mesh->material_info, header.num_materials);
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
    
    // Skeleton
    if (header.num_skeleton_joints) {
        mesh->flags   |= MeshFlags_ANIMATED;
        mesh->skeleton = PUSH_STRUCT_ZERO(arena, Skeleton);
        
        // Joint info
        Skeleton *skeleton = mesh->skeleton;
        array_init_and_resize(&skeleton->joint_info, header.num_skeleton_joints);
        for (s32 i = 0; i < header.num_skeleton_joints; i++) {
            Skeleton_Joint_Info *joint = &skeleton->joint_info[i];
            
            // Read 4x4 matrix.
            get(&file, &joint->object_to_joint_matrix);
            
            // Read the joint's rest pose rotation relative to its parent.
            get(&file, &joint->rest_pose_rotation_relative);
            
            // Read name
            s32 joint_name_len = 0;
            get(&file, &joint_name_len);
            String8 joint_name = str8(file.data, joint_name_len);
            advance(&file, joint_name_len);
            joint->name = str8_copy(arena, joint_name);
            
            // Read parent id.
            get(&file, &joint->parent_id);
        }
        
        // Blend info
        s32 num_canonical_vertices;
        get(&file, &num_canonical_vertices);
        ASSERT(num_canonical_vertices > 0);
        array_init_and_resize(&skeleton->vertex_blend_info, num_canonical_vertices);
        for (s32 i = 0; i < num_canonical_vertices; i++) {
            Vertex_Blend_Info *blend = &skeleton->vertex_blend_info[i];
            
            get(&file, &blend->num_pieces);
            
            for (s32 piece_index = 0; piece_index < blend->num_pieces; piece_index++) {
                get(&file, &blend->pieces[piece_index].joint_id);
                get(&file, &blend->pieces[piece_index].weight);
            }
        }
        
#if DEVELOPER
        // @Note: We keep those for mouse-picking.
        array_init_and_resize(&mesh->skinned_vertices, header.num_vertices);
        array_copy(&mesh->skinned_vertices, mesh->vertices);
#endif
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
                map = find(&game->texture_catalog, map_name);
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
    Vertex_XTBNUCJW *vertex_buffer = PUSH_ARRAY_ZERO(scratch.arena, Vertex_XTBNUCJW, num_vertices);
    Vertex_XTBNUCJW *vertex = vertex_buffer;
    
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
        
        vertex->joint_ids = v4s(-1);
        vertex->weights   = {};
        if (mesh->skeleton) {
            s32 canonical_vertex_index    = mesh->canonical_vertex_map[vindex];
            Vertex_Blend_Info *blend_info = &mesh->skeleton->vertex_blend_info[canonical_vertex_index];
            for (s32 piece_index = 0; piece_index < blend_info->num_pieces; piece_index++) {
                vertex->joint_ids.I[piece_index] = blend_info->pieces[piece_index].joint_id;
                vertex->weights.I  [piece_index] = blend_info->pieces[piece_index].weight;
            }
        }
    }
    
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (UINT)(sizeof(Vertex_XTBNUCJW) * num_vertices);
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.Usage     = D3D11_USAGE_IMMUTABLE;
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

FUNCTION void load_triangle_mesh(Arena *arena, Triangle_Mesh *mesh, String8 full_path)
{
    mesh->full_path = full_path;
    
    String8 file = os->read_entire_file(full_path);
    if (!file.data) {
        debug_print("MESH LOAD ERROR: Couldn't load mesh at %S\n", full_path);
        return;
    }
    
    String8 orig_file  = file;
    
    load_mesh_data(arena, mesh, file);
    load_mesh_textures(mesh);
    generate_buffers_for_mesh(mesh);
    generate_bounding_box_for_mesh(mesh);
    
    os->free_file_memory(orig_file.data);
}