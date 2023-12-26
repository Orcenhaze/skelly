mesh_name        = ""
smooth_shaded    = 0
make_convex_hull = 0

directory = "C:\\work\\skelly\\data\\meshes\\"
extension = ".mesh"

# File format version. Increment when modifying file.
version = 2

"""
    About this mesh exporter:
    - It only writes blender's Principled BSDF, and only the basic data in it.
    - It exports one object only, so make sure to join sub-meshes together.
    - It only exports one index of Vertex Colors.
    - The exported object must have _at least_ one material slot.
    - It generates the convex hull if `make_convex_hull` is set to 1.
    - It exports vertex weights and rest pose skeleton info.
        MESH origin and ARMATURE origin must match!
    
    TODO:
    - We need to merge all vertices by distance before exporting. Not doing so causes issues...
    - Sometimes mesh_data.vertices != len(processed_verts) during weight parsing which causes issues. 
      Probably due to some vertices not being used in any face/polygon or something.
    
    @Cleanup: This file is super messy!
    @Cleanup:
    @Cleanup:
    
    
    MISC:
    
    // @Note: 
    // "object space" is where mesh vertices exist (MESH object origin), a.k.a. "model space".
    // "armature space" is where joint matrices often exist (ARMATURE object origin).
    // This exporter requires that armature origin and mesh origin are the same (object space == armature space).

    // Gives the joint's current pose matrix(4x4) in armature space.
    bpy.data.objects['Armature'].pose.bones["Torso"].matrix
    
    // To get the joint's current pose matrix relative to the parent, we do the following
    m_child  = bpy.data.objects['Armature'].pose.bones["Torso"].matrix
    m_parent = bpy.data.objects['Armature'].pose.bones["Torso"].parent.matrix
    result   = m_parent.inverted() @ m_child
    
    // Gives the joint's rest pose matrix in armature space.
    // Both are the same.
    bpy.data.objects['Armature'].pose.bones["Torso"].bone.matrix_local
    bpy.data.objects['Armature'].data.bones["Torso"].matrix_local
    
    // Gives the joint's current pose matrix(4x4) relative to the same joint in rest pose.
    // Translation part of the matrix is zero (as we can't translate joints in pose mode?).
    bpy.data.objects['Armature'].pose.bones["Torso"].matrix_basis
    
    // Gives the joint's rest pose matrix(3x3) relative to parent (no translation)
    bpy.data.objects['Armature'].data.bones["Torso"].matrix
 
"""

""" IMPORTS
"""
from   collections import OrderedDict
from   numpy import allclose
import os
import struct
import array
import bpy
import bmesh
import pdb
import mathutils

class Vertex_XTBNUC:
    def __init__(self, X, TBN, UV, C):

        self.position  = X
        self.tbn       = TBN
        self.uv        = UV
        self.color     = C

    def __hash__(self):
        return hash((self.position, self.tbn, self.uv, self.color))

class Joint_Info:
    def __init__(self):
        self.object_to_joint_matrix      = [] # Matrix4x4 as array
        self.rest_pose_rotation_relative = [] # Quaternion as array
        self.name                        = ""
        self.parent_id                   = 0  # could be negative (root's parent)


class Vertex_Blend_Info:
    def __init__(self):
        self.num_pieces  = 0  # How many joints influence this vertex
        self.pieces      = [] # list of (joint_id, weight)

""" GLOBAL VARIABLES
"""
# Matrix to transform from Blender's space coordinates to "our" space coordinates.
# Rotate -90deg around x-axis.
BlenderToEngineQuat   = mathutils.Quaternion((0.707, -0.707, 0.0, 0.0))
BlenderToEngineMatrix = mathutils.Quaternion((0.707, -0.707, 0.0, 0.0)).to_matrix().to_4x4()

# Poor man's enum of Material_Texture_Map_Type:
MapType_ALBEDO    = 0
MapType_NORMAL    = 1
MapType_METALLIC  = 2
MapType_ROUGHNESS = 3
MapType_AO        = 4
MapType_COUNT     = 5

vertex_to_index_map = {}    # {Vertex_XTBNUC hash : int}
xtbnuc_vertices     = []

skeleton_joints     = []   # list of all (Joint_Info) count=num_skeleton_joints
num_skeleton_joints = 0

vertex_blends        = []  # list of all (Vertex_Blend_Info) count= len(mesh_data.vertices)

# Data to write:
triangle_mesh_header  = []
vertices 			  = []
tbns                  = []
uvs 				  = []
colors 			      = []
indices 		      = []
triangle_list_info    = []
material_info         = []
skeleton_info         = []

# Reading material_info:
# for i in range header.num_materials:
#   read material attributes.
#   for s in range MapType_COUNT:
#     read length of texture map file_name
#     read the texture map file_name

""" UTILS
"""
def append_string(the_list, the_string):
    length  = len(the_string.encode('utf-8'))
    unicode = [ord(c) for c in the_string]
    the_list.append(length)
    the_list.append(unicode) # Append as a list of unicode ints.

""" FUNCTIONS
"""


def add_joint(inv_rest_pose, rest_pose_rotation_rel, name, parent_id):
    q = rest_pose_rotation_rel

    joint = Joint_Info()
    joint.object_to_joint_matrix      = [elem for row in inv_rest_pose for elem in row]
    joint.rest_pose_rotation_relative = [q.x, q.y, q.z, q.w]
    joint.name                        = name
    joint.parent_id                   = parent_id
    
    skeleton_joints.append(joint)
    global num_skeleton_joints
    num_skeleton_joints += 1
    
    return num_skeleton_joints-1 # id of the joint that was just added.

def parse_armature(armature):
    #
    # This function will fill the skeleton_joints[] list with Joint_Info (num_skeleton_joints as many)
    #

    # Search for root candidates
    bones = armature.data.bones
    root_candidates = [b for b in bones if not b.parent]
    # root_candidates = [b for b in bones if not b.parent and b.name[:4].lower() == 'root']

    # Only one node can be eligible
    if len(root_candidates) > 1:
        print('Found multiple root joints, only one is allowed:')
        for r in root_candidates:
            print(r.name)
        raise Exception( 'A single root must exist in the armature.')
    elif len(root_candidates) == 0:
        raise Exception( 'No root found.')

    # Get the root
    root = root_candidates[0]
    del root_candidates  

    rest_pose_matrix       = BlenderToEngineMatrix @ root.matrix_local
    rest_pose_rotation_rel = BlenderToEngineQuat   @ root.matrix.to_quaternion()

    root_id = add_joint(rest_pose_matrix.inverted(), rest_pose_rotation_rel.normalized(), root.name, -1)
    
    # recursively compute others bones data
    for child in root.children:
        recursively_add_joint(child, root_id)

def recursively_add_joint(joint, parent_id):
    rest_pose_matrix       = BlenderToEngineMatrix @ joint.matrix_local
    rest_pose_rotation_rel = joint.matrix.to_quaternion().normalized()
    joint_id               = add_joint(rest_pose_matrix.inverted(), rest_pose_rotation_rel, joint.name, parent_id)

    for child in joint.children:
      recursively_add_joint(child, joint_id)

def parse_weights(mesh_obj, mesh_data):
    #
    # This function will fill the vertex_blends[] list with Vertex_Blend_Info (len(mesh_data.vertices) as many)
    # 
    
    #
    # We will iterate through vertices in same order we wrote vertex_xtbnuc, i.e. through faces and 
    # NOT directly through mesh_data.vertices.
    #
    
    # Since we are iterating in this order, we will go over same vertices multiple times. 
    # This list will help us write Vertex_Blend_Info only for canonical vertices.
    processed_verts = []

    for face in mesh_data.polygons:
        for loop_index in face.loop_indices:
            loop = mesh_data.loops[loop_index]
            
            if loop.vertex_index not in processed_verts:
                processed_verts.append(loop.vertex_index)
                vertex = mesh_data.vertices[loop.vertex_index]
                vertex_blend = Vertex_Blend_Info()
            
                # Create a list to store ids and weights for the current vertex
                pieces = []

                # Iterate through vertex groups
                for group in mesh_obj.vertex_groups:
                    # Check if the vertex is assigned to this group
                    for v_group in vertex.groups:
                        if v_group.group == group.index:
                            pieces.append({'bone_index': group.index, 'weight': v_group.weight})

                # Sort the weights by weight value (descending order)
                pieces.sort(key=lambda x: x['weight'], reverse=True)

                # Truncate the list to a maximum of 4 weights per vertex
                pieces = pieces[:4]

                # If the length of weights is less than 4, append zero-weight entries
                #while len(pieces) < 4:
                #    pieces.append({'bone_index': -1, 'weight': 0.0})

                vertex_blend.num_pieces = len(pieces)
                vertex_blend.pieces     = [value for d in pieces for value in d.values()]

                # Append the vertex weights dictionary to the list
                vertex_blends.append(vertex_blend)
    assert len(processed_verts) == len(mesh_data.vertices)

def append_attribute_default_value_to_material_info(input):
    if input.type == "RGBA":
        material_info.extend(input.default_value)
    elif input.type == "VALUE":
        material_info.append(input.default_value)
    else:
        raise Exception("Unsupported BSDF attribute type %s." % input.type)

def append_attribute_and_set_map_name(input, texture_map_names, map_type):
    # Always read the default values for BSDF attributes (except Normal, only accept normal map).
    # In case the attribute is connected to a TEX_IMAGE node, then also set the map name.
    
    # Make sure we pass initialized list of map names.
    assert len(texture_map_names) == MapType_COUNT
    
    if input.name != 'Normal':
        append_attribute_default_value_to_material_info(input)
    
    if len(input.links):
        linked_node = input.links[0].from_node
        if linked_node.type == 'TEX_IMAGE':
            assert linked_node.image != None
            texture_map_names[map_type] = linked_node.image.name
            
        elif linked_node.type == 'NORMAL_MAP':
            image_node = linked_node.inputs['Color'].links[0].from_node
            assert image_node.type  == 'TEX_IMAGE'
            assert image_node.image != None
            texture_map_names[map_type] = image_node.image.name

def set_materials(materials):
    for mat in  materials:    
        # Get the BSDF node.
        bsdf = None
        if mat.node_tree != None:                   # Should have node tree.
            for n in mat.node_tree.nodes.keys():    # Searching all nodes.
                node = mat.node_tree.nodes[n]
                if node.type == 'BSDF_PRINCIPLED':  # Node type should be BSDF.
                    bsdf = node                     # Setting the node.
                    break                           # Exit node tree.
        
        # BSDF not found, skipping.
        if bsdf == None:
            continue
        
        # Get attributes (Principled BSDF doesn't have ambient occlusion, apparently).
        base_color = bsdf.inputs.get("Base Color")
        metallic   = bsdf.inputs.get("Metallic") 
        roughness  = bsdf.inputs.get("Roughness")
        normal_map = bsdf.inputs.get("Normal")

        # Initialize map names with empty names.
        texture_map_names = [''] * MapType_COUNT
        
        # Set attributes.
        append_attribute_and_set_map_name(base_color, texture_map_names, MapType_ALBEDO)
        append_attribute_and_set_map_name(metallic,   texture_map_names, MapType_METALLIC)
        append_attribute_and_set_map_name(roughness,  texture_map_names, MapType_ROUGHNESS)
        append_attribute_and_set_map_name(normal_map, texture_map_names, MapType_NORMAL)

        # Append basenames to material_info.
        for s in range(MapType_COUNT):
            file_name = texture_map_names[s]
            append_string(material_info, file_name)

def add_unique_vertex_and_append_index(vertex):
    h = hash(vertex)
    if (h not in vertex_to_index_map):
        vertex_to_index_map[h] = len(xtbnuc_vertices)
        xtbnuc_vertices.append(vertex)
    indices.append(vertex_to_index_map[h])

def main():
    """ VARIABLES
    """
    mesh_obj           = None
    mesh_data          = None
    previous_face      = None
    triangle_lists     = []     # len(triangle_lists) is num_triangle_lists
    materials          = []     # len(materials)      is num_materials
    used_materials     = []
    current_list_index = -1
    
    """ SETUP
    """
    # Set object mode.
    bpy.ops.object.mode_set(mode="OBJECT")
    
    # Get the active/selected object. Make sure it's the correct type.
    mesh_obj = bpy.context.view_layer.objects.active
    if mesh_obj.type != "MESH":
        raise Exception("Active object must be of type MESH, not %s." % mesh_obj.type)
    if not mesh_obj.material_slots.values():
        raise Exception("The object must have at least one Material Slot.")
    
    # Get armature if available and set pose position to REST pose.
    armature = None
    if mesh_obj.parent and mesh_obj.parent.type == "ARMATURE":
        armature = mesh_obj.parent
        if not allclose(mesh_obj.matrix_world, armature.matrix_world):
            raise Exception("ARMATURE and MESH origins must match!");
        armature.data.pose_position = 'REST'
    
    mesh_data      = mesh_obj.data
    orig_mesh_data = mesh_data.copy()

    # Create bmesh.
    b_mesh = bmesh.new()
    b_mesh.from_mesh(mesh_data)
   
    # Split quads into triangles.
    bmesh.ops.triangulate(b_mesh, faces=b_mesh.faces[:])
    
    # Update mesh after triangulation.
    b_mesh.to_mesh(mesh_data)
    b_mesh.free()
    
    # Build the Convex Hull as a mesh.
    if make_convex_hull:
        ch_obj       = mesh_obj.copy()
        ch_obj.name  = "%sConvexHull" % mesh_obj.name
        ch_mesh_data = ch_obj.data = bpy.data.meshes.new("%sConvexHull" % orig_mesh_data.name)
        
        ch_b_mesh = bmesh.new()
        ch_b_mesh.from_mesh(orig_mesh_data)
        
        ch = bmesh.ops.convex_hull(ch_b_mesh, input=ch_b_mesh.verts, use_existing_faces=True)
        bmesh.ops.delete(ch_b_mesh, geom=ch["geom_unused"] + ch["geom_interior"], context='VERTS')
        #bmesh.ops.triangulate(ch_b_mesh, faces=ch_b_mesh.faces[:])
        
        ch_b_mesh.to_mesh(ch_mesh_data)
        ch_b_mesh.free()
        bpy.context.scene.collection.objects.link(ch_obj)
    
    # Make sure we have a uv map.
    if len(mesh_data.uv_layers) == 0:
        bpy.ops.mesh.uv_texture_add()
        
    # Make sure we have one vertex color index.
    if not mesh_data.vertex_colors.active:
        mesh_data.vertex_colors.new()
    
    # Calculate tangents for mesh.
    mesh_data.calc_tangents()
    
    for face in mesh_data.polygons:
        
        for loop_index in face.loop_indices:
            
            loop = mesh_data.loops[loop_index]
            
            face_tangent   = loop.tangent
            face_normal    = loop.normal
            face_bitangent = loop.bitangent_sign * face_normal.cross(face_tangent)
            
            face_tangent   = BlenderToEngineMatrix @ face_tangent
            face_normal    = BlenderToEngineMatrix @ face_normal
            face_bitangent = BlenderToEngineMatrix @ face_bitangent
        
            TBN = face_tangent[:] + face_bitangent[:] + face_normal[:]
            if smooth_shaded:
                TBN = face_tangent[:] + face_bitangent[:] + mesh_data.vertices[loop.vertex_index].normal[:]

            # Construct and add vertex if it's unique.
            X     = (BlenderToEngineMatrix @ mesh_data.vertices[loop.vertex_index].co)[:]
            UV    = mesh_data.uv_layers.active.data[loop_index].uv[:] if mesh_data.uv_layers.active is not None else [0.0, 1.0]
            UV    = (UV[0], 1.0 - UV[1])    # Flip vertically, to make origin top-left.
            C     = mesh_data.vertex_colors[0].data[loop_index].color[:]
            add_unique_vertex_and_append_index(Vertex_XTBNUC(X, TBN, UV, C))
    
        # Append material used by this face.
        used_materials.append(mesh_obj.material_slots[face.material_index].material)
        
        # Construct triangle list.
        if previous_face == None or face.material_index != previous_face.material_index:
            triangle_lists.append([face.material_index, face.loop_total, face.loop_start])
            previous_face = face
            current_list_index += 1
        else:
            triangle_lists[current_list_index][1] += face.loop_total
            previous_face = face
 
    # Set triangle_list_info to export.
    for t in triangle_lists:
        triangle_list_info.extend(t)
        
    # Intersection between all materials and used_materials.
    used_materials = list(OrderedDict.fromkeys(used_materials))
    materials      = [mat for mat in mesh_data.materials if mat in used_materials]
    
    # Set material_info to export.
    set_materials(materials)
    
    # Set vertex attributes to export.
    for v in xtbnuc_vertices:
        vertices.extend(v.position)
        tbns    .extend(v.tbn)
        uvs     .extend(v.uv)
        colors  .extend(v.color)

    # Prepare and set Skeleton Info
    if armature != None:
        parse_armature(armature)
        parse_weights(mesh_obj, mesh_data)
        if num_skeleton_joints:
            for joint in skeleton_joints:
                skeleton_info.extend(joint.object_to_joint_matrix)
                skeleton_info.extend(joint.rest_pose_rotation_relative)
                append_string(skeleton_info, joint.name)
                skeleton_info.append(joint.parent_id)
            
            skeleton_info.append(len(mesh_data.vertices)) # num_canonical_vertices
            for blend in vertex_blends:
                skeleton_info.append(blend.num_pieces)
                skeleton_info.extend(blend.pieces)


    """ TRIANGLE_MESH_HEADER
    """
    triangle_mesh_header.append(len(vertices)//3)		# num_vertices
    triangle_mesh_header.append(len(tbns)//9)	        # num_tbns
    triangle_mesh_header.append(len(uvs)//2)			# num_uvs
    triangle_mesh_header.append(len(colors)//4)		    # num_colors
    triangle_mesh_header.append(len(indices))           # num_indices
    triangle_mesh_header.append(len(triangle_lists))    # num_triangle_lists
    triangle_mesh_header.append(len(materials))		    # num_materials
    triangle_mesh_header.append(num_skeleton_joints)

    """ WRITE TO BINARY FILE
    """
    filepath = directory + mesh_name + extension
    
    # Overwrite the mesh file.
    if os.path.exists(filepath):
      os.remove(filepath)
    f = open(filepath, "wb")

    f.write(struct.pack("<i", version))
    array.array('i', triangle_mesh_header)  .tofile(f)
    array.array('f', vertices)              .tofile(f)
    array.array('f', tbns)                  .tofile(f)
    array.array('f', uvs)                   .tofile(f)
    array.array('f', colors)                .tofile(f)
    array.array('I', indices)               .tofile(f)
    array.array('i', triangle_list_info)    .tofile(f)

    # @Cleanup: Pull out code for writing list of multiple types to file..?
    
    # material_info has different types inside it.
    for e in material_info:
        if isinstance(e, float):
            f.write(struct.pack("<f", e))
        if isinstance(e, int):
            f.write(struct.pack("<i", e))
        if isinstance(e, list):
            array.array('b', e).tofile(f)

    # skeleton_info has different types inside it.
    for e in skeleton_info:
        if isinstance(e, float):
            f.write(struct.pack("<f", e))
        if isinstance(e, int):
            f.write(struct.pack("<i", e))
        if isinstance(e, list):
            array.array('b', e).tofile(f)
    
    f.close()

    # Clear console (for debugging)
    #cls = lambda: system('cls')
    #cls()

    """ DEBUG
    """

    print("\"{0}\" exported successfully!".format(mesh_name+extension))

if __name__ == "__main__":
    main()