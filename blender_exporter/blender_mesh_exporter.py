mesh_name        = ""
smooth_shaded    = 0
make_convex_hull = 0

directory = "C:\\work\\skelly\\data\\"
extension = ".mesh"

# File format version. Increment when modifying file.
version = 1

"""
    About this mesh exporter:
    - It only writes blender's Principled BSDF, and only the basic data in it.
    - It exports one object only, so make sure to join sub-meshes together.
    - It only exports one index of Vertex Colors.
    - The exported object must have _at least_ one material slot.
    - It generates the convex hull if `make_convex_hull` is set to 1.
    
    INCOMPLETE:
    1) Doesn't export skeletal hierarchy nor animation.
    
"""

""" IMPORTS
"""
from   collections import OrderedDict
import os
import struct
import array
import bpy
import bmesh
import pdb

class Vertex_XTBNUC:
    def __init__(self, X, TBN, UV, C):

        self.position  = X
        self.tbn       = TBN
        self.uv        = UV
        self.color     = C

    def __hash__(self):
        return hash((self.position, self.tbn, self.uv, self.color))

""" GLOBAL VARIABLES
"""
# Poor man's enum of Material_Texture_Map_Type:
MapType_ALBEDO    = 0
MapType_NORMAL    = 1
MapType_METALLIC  = 2
MapType_ROUGHNESS = 3
MapType_AO        = 4
MapType_COUNT     = 5

vertex_to_index_map = {}    # {Vertex_XTBNUC hash : int}
xtbnuc_vertices     = []

# Data to write:
triangle_mesh_header  = []
vertices 			  = []
tbns                  = []
uvs 				  = []
colors 			      = []
indices 		      = []
triangle_list_info    = []
material_info         = []

# Reading material_info:
# for i in range header.num_materials:
#   read material attributes.
#   for s in range MapType_COUNT:
#     read length of texture map file_name
#     read the texture map file_name

""" FUNCTIONS
"""

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
            file_name         = texture_map_names[s]
            file_name_len     = len(file_name.encode('utf-8'))
            file_name_unicode = [ord(c) for c in file_name]
            material_info.append(file_name_len)
            material_info.append(file_name_unicode) # Append as a list of unicode ints.

def add_unique_vertex_and_append_index(vertex):
    h = hash(vertex)
    if (h not in vertex_to_index_map):
        vertex_to_index_map[h] = len(xtbnuc_vertices)
        xtbnuc_vertices.append(vertex)
    indices.append(vertex_to_index_map[h])
    
def main():
    """ VARIABLES
    """
    obj                = None
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
    obj = bpy.context.view_layer.objects.active
    if obj.type != "MESH":
        raise Exception("Object must be of type MESH, not %s." % obj.type)
    if not obj.material_slots.values():
        raise Exception("The object must have at least one Material Slot.")
    
    mesh_data      = obj.data
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
        ch_obj       = obj.copy()
        ch_obj.name  = "%sConvexHull" % obj.name
        ch_mesh_data = ch_obj.data = bpy.data.meshes.new("%sConvexHull" % orig_mesh_data.name)
        
        ch_b_mesh = bmesh.new()
        ch_b_mesh.from_mesh(orig_mesh_data)
        
        ch = bmesh.ops.convex_hull(ch_b_mesh, input=ch_b_mesh.verts, use_existing_faces=True)
        bmesh.ops.delete(ch_b_mesh, geom=ch["geom_unused"] + ch["geom_interior"], context='VERTS')
        #bmesh.ops.triangulate(ch_b_mesh, faces=ch_b_mesh.faces[:])
        
        ch_b_mesh.to_mesh(ch_mesh_data)
        ch_b_mesh.free()
        bpy.context.scene.collection.objects.link(ch_obj)
    
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
        
            TBN = face_tangent[:] + face_bitangent[:] + face_normal[:]
            if smooth_shaded:
                TBN = face_tangent[:] + face_bitangent[:] + mesh_data.vertices[loop.vertex_index].normal[:]

            # Construct and add vertex if it's unique.
            X     = mesh_data.vertices[loop.vertex_index].co[:]
            UV    = mesh_data.uv_layers.active.data[loop_index].uv[:] if mesh_data.uv_layers.active is not None else [0.0, 1.0]
            UV    = (UV[0], 1.0 - UV[1])    # Flip vertically, to make origin top-left.
            C     = mesh_data.vertex_colors[0].data[loop_index].color[:]
            add_unique_vertex_and_append_index(Vertex_XTBNUC(X, TBN, UV, C))
    
        # Append material used by this face.
        used_materials.append(obj.material_slots[face.material_index].material)
        
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

    """ TRIANGLE_MESH_HEADER
    """
    triangle_mesh_header.append(len(vertices)//3)		# num_vertices
    triangle_mesh_header.append(len(tbns)//9)	        # num_tbns
    triangle_mesh_header.append(len(uvs)//2)			# num_uvs
    triangle_mesh_header.append(len(colors)//4)		    # num_colors
    triangle_mesh_header.append(len(indices))           # num_indices
    triangle_mesh_header.append(len(triangle_lists))    # num_triangle_lists
    triangle_mesh_header.append(len(materials))		    # num_materials

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

    # material_info has different types inside it.
    for e in material_info:
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