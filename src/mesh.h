#ifndef MESH_H
#define MESH_H

GLOBAL s32 const MESH_FILE_VERSION = 2;

/*
@Note: Some terms and explainations:
- The vertices of a triangle mesh are in rest pose and in object space.
 
- Joints in Blender or Maya represent a transform. Basically a matrix that holds translation,
 rotation and scale information. These transforms can either be in "local space" (relative to 
the joint's parent) or in "global space" (relative to the object/model).
To represent a joint transform that's in local-space to global-space, we multiply down the hierarchy, i.e.,
we keep multiplying the matrix of the current joint with the parent joint's global transform until we are done with every joint.
   The root joint has no parent, so its local-space and global-space transforms are the same. 

- When animating some skeleton with keyframes and such, the joint matrices would represent the current
 pose in local-space (each joint relative to its parent). But our mesh vertices are in objects space, so
we need to convert local-space joint matrices to object space (global-space), i.e., multiply down the 
hierarchy. The result is the current pose matrices in object space (for each joint).
We can't use the current pose matrices for skinning our mesh vertices straight away, and here's why:
 multiplying matrices combines the effects; the current pose matrices contain the following:
the transforms to apply to achieve current pose in _local-space_, AND THEN converting to global-space.
So, to use these matrices properly, we first need to convert the mesh vertices from object-space to 
joint-local space. 
To convert a vertex from object space to space of joint j, we multiply it by the inverse rest pose 
matrix of that joint. ONLY THEN do we multiply with the current pose matrix that includes the conversion
 back to object-space.

*/

#define MAX_JOINTS_PER_VERTEX 4
struct Vertex_Blend_Piece
{
    s32 joint_id;
    f32 weight;
};

struct Vertex_Blend_Info
{
    s32 num_pieces;
    Vertex_Blend_Piece pieces[MAX_JOINTS_PER_VERTEX]; // Weights should add to 1?
};

struct Skeleton_Joint_Info
{
    // People call this "inverse bind pose matrix" or "offset matrix"; it transforms vertices from
    // object/mesh space to joint space (relative to parent).
    M4x4       object_to_joint_matrix; 
    
    // Joint's rest pose rotation relative to its parent (joint space).
    // We use this to figure out if we are in the neighborhood of the rest pose when blending animations.
    // Check Casey Muratori's video "Quaternion Double-cover and the Rest Pose Neighborhood".
    Quaternion rest_pose_rotation_relative;
    
    String8    name;
    s32        parent_id;
};

struct Skeleton
{
    Array<Skeleton_Joint_Info> joint_info;        // num_skeleton_joints of these.
    Array<Vertex_Blend_Info>   vertex_blend_info; // Per canonical vertex; Use canonical_vertex_map
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

struct Triangle_Mesh_Header {
    s32 num_vertices;
    s32 num_indices;             // Mul by sizeof(u32) to get bytes
    s32 num_triangle_lists;      // Mul by sizeof(Triangle_List_Info) to get bytes
    s32 num_materials;      
    s32 num_skeleton_joints;     // Mul by sizeof(Skeleton_Joint_Info) to get bytes
};

struct Triangle_List_Info {
    s32 material_index;
    s32 num_indices;
    s32 first_index;
    
    Texture *texture_maps[MaterialTextureMapType_COUNT];
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

enum Mesh_Flags
{
    MeshFlags_NONE,
    
    MeshFlags_ANIMATED
};

struct Triangle_Mesh
{
    // @Note: The mesh data is read from `blender_mesh_exporter.py` output.
    
    Array<V3>  vertices;
    Array<TBN> tbns;
    Array<V2>  uvs;
    Array<V4>  colors;
    
    // Maps mesh vertices to unique/canonical vertices (needed for indexing blend info).
    Array<s32> canonical_vertex_map;
    
    Array<u32> indices;
    
    Array<Triangle_List_Info> triangle_list_info;
	Array<Material_Info>      material_info;
    
    // @Temporary: Should skin on the GPU
    // @Temporary:
    // @Temporary:
    Array<V3> skinned_vertices;
    Array<V3> skinned_normals;
    
    // Bounds of the mesh, computed at mesh load time, in local space.
    Rect3 bounding_box;
    
    String8 name;
    String8 full_path;
    
    Skeleton *skeleton;
    
    // Vertex and index buffers.
    ID3D11Buffer *vbo;
    ID3D11Buffer *ibo;
    
    u32 flags;
};

#endif //MESH_H
