#ifndef MESH_H
#define MESH_H

struct Triangle_Mesh_Header {
    s32 num_vertices;            // Mul by sizeof(Vector3) to get bytes
    s32 num_uvs;                 // Mul by sizeof(Vector2) to get bytes
    s32 num_tbns;                // Mul by sizeof(TBN)     to get bytes
    s32 num_colors;              // Mul by sizeof(Vector4) to get bytes
    s32 num_indices;             // Mul by sizeof(u32)     to get bytes
    
    s32 num_triangle_lists;      // Mul by sizeof(Triangle_List_Info) to get bytes
    s32 num_materials;      
};

struct Triangle_List_Info {
    s32 material_index;
    s32 num_indices;
    s32 first_index;
    
    // @Todo: Multiple textures per triangle.
	Texture *diffuse_map;
	Texture *normal_map;
};

enum Material_Texture_Map_Type
{
	MaterialTextureMapType_ALBEDO,
	MaterialTextureMapType_NORMAL,
	MaterialTextureMapType_METALLIC,
	MaterialTextureMapType_ROUGHNESS,
	MaterialTextureMapType_AO,
	
	MaterialTextureMapType_COUNT
};

struct Material_Info {
    V4   base_color;
	f32  metallic;
    f32  roughness;
	f32  ambient_occlusion;
    
    String8 texture_map_names[MaterialTextureMapType_COUNT];
};

struct TBN
{
    V3 tangent;
    V3 bitangent;
    V3 normal;
};

struct Triangle_Mesh
{
    // @Note: The mesh data is read from `blender_mesh_exporter.py` output.
    // Bounding_Volume data is read from `blender_meshbv_exporter.py`
    
    String8 name;
    String8 full_path;
    
    Array<V3>  vertices;
    Array<V2>  uvs;
    Array<TBN> tbns;
    Array<V4>  colors;
    
    Array<u32> indices;
    
    //Bounding_Box    bounding_box;    // Computed at mesh load time.
    //Bounding_Volume bounding_volume; // Loaded from a .MESHBV file.
    
    // Ideally, num_triangle_lists == num_materials. 
    Array<Triangle_List_Info> triangle_list_info;
	Array<Material_Info>      material_info;
    
    // Vertex and index buffers.
    //GLuint vbo_id;
    //GLuint ibo_id;
};

#endif //MESH_H
