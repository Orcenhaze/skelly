#ifndef MESH_H
#define MESH_H

GLOBAL s32 const MESH_FILE_VERSION = 2;

//~ Skeletal Animation
//

/*
@Note: Some terms and explainations:
- The vertices of a triangle mesh are in rest pose and in object space.
 
- Joints in Blender or Maya represent a transform. Basically a matrix that holds translation,
 rotation and scale information. These transforms can either be in "local space" (relative to 
the joint's parent) or in "global space" (relative to the object/model).
To represent a joint transform that's in local-space to global-space, we multiply up the hierarchy, i.e.,
we keep multiplying the matrix of the current joint with the parent joint's transform until we reach the
 root joint. The root joint has no parent, so its local-space and global-space transforms are the same. 

- When animating some skeleton with keyframes and such, the joint matrices would represent the current
 pose in local-space (each joint relative to its parent). But our mesh vertices are in objects space, so
we need to convert local-space joint matrices to object space (global-space), i.e., multiply up the 
hierarchy. The result is the current pose matrices in object space (for each joint).
We can't use the current pose matrices for skinning our mesh vertices straight away, and here's why:
 multiplying matrices combines the effects; the current pose matrices contain the following:
the transforms to apply to achieve current pose in _local-space_, AND THEN converting to global-space.
So, to use these matrices properly, we first need to convert the mesh vertices from object-space to 
joint-local space. 
To convert a vertex from object space to space of joint j, we multiply it by the inverse rest pose 
matrix of that joint. ONLY THEN do we multiply with the current pose matrix that includes the conversion
 back to object-space.

@Todo:
[] Create one struct called Skeleton_Info that contains both Joint_Weight_Info AND Skeleton?
[] Joint_Weight_Info is per-vertex, but it should be per-canonical-vertex, not mesh-vertex.
look at vertex_to_skeleton_info_map[] AnimPlayback pt.2 @2:13:36
[] Expand mesh exporter to export per-vertex joint_weights and also rest pose skeleton?
[] Apply correction matrix that rotates everything -90deg around x-axis (blender space to our space).

*/

#if 0
struct SQT
{
    Quaternion rotation;    // Q: quaternion
    V3         translation; // T: translation
    f32        scale;       // S: scale
};

struct Skeleton_Pose
{
    Skeleton   *skeleton;
    Array<SQT>  local_poses;  // Relative to joint parent.
    Array<M4x4> global_poses; // In model space.
};


struct Animation_Sample
{
    Array<SQT> joint_poses; // Joint poses at some keyframe in ??? space.
};

struct Animation_Clip
{
    Array<Animation_Sample> samples; // A.k.a keyframes.
    Skeleton *skeleton;
    f32 frames_per_second;
    u32 frame_count;
    b32 is_looping; // if False -> sample_count is frame_count+1; if True -> sample_count is frame_count
};
#endif


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
    M4x4    inverse_rest_pose; // Transforms vertices from object space to joint space.
    String8 name;
    s32     parent_id;
};

struct Skeleton_Info
{
    Array<Skeleton_Joint_Info> joint_info;        // num_skeleton_joints of these.
    Array<Vertex_Blend_Info>   vertex_blend_info; // Per canonical vertex;
};


//~ Triangle Mesh 
//
enum Material_Texture_Map_Type
{
	MaterialTextureMapType_ALBEDO,
	MaterialTextureMapType_NORMAL,
	MaterialTextureMapType_METALLIC,
	MaterialTextureMapType_ROUGHNESS,
	MaterialTextureMapType_AO,
	
	MaterialTextureMapType_COUNT
};

// @Todo: Remove num_tbns, num_uvs and num_colors; these must match num_vertices anyway...
struct Triangle_Mesh_Header {
    s32 num_vertices;            // Mul by sizeof(Vector3) to get bytes
    s32 num_tbns;                // Mul by sizeof(TBN)     to get bytes
    s32 num_uvs;                 // Mul by sizeof(Vector2) to get bytes
    s32 num_colors;              // Mul by sizeof(Vector4) to get bytes
    s32 num_indices;             // Mul by sizeof(u32)     to get bytes
    
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

struct Triangle_Mesh
{
    // @Note: The mesh data is read from `blender_mesh_exporter.py` output.
    // Bounding_Volume data is read from `blender_meshbv_exporter.py`
    
    String8 name;
    String8 full_path;
    
    Array<V3>  vertices;
    Array<TBN> tbns;
    Array<V2>  uvs;
    Array<V4>  colors;
    
    Array<u32> indices;
    
    // Ideally, num_triangle_lists == num_materials. 
    Array<Triangle_List_Info> triangle_list_info;
	Array<Material_Info>      material_info;
    
    // @Note: Maps mesh vertices to indices of unique/canonical vertices as if they were in a separate array.
    Array<s32>    canonical_vertex_map;
    Skeleton_Info skeleton_info; // @Todo: Should this be a pointer?
    
    // Bounds of the mesh, computed at mesh load time, in local space.
    Rect3 bounding_box;
    
    //Bounding_Volume bounding_volume; // Loaded from a .MESHBV file.
    
    // Vertex and index buffers.
    ID3D11Buffer *vbo;
    ID3D11Buffer *ibo;
};

#endif //MESH_H
