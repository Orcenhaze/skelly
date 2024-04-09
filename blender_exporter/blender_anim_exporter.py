anim_name = ""

directory = "C:\\work\\skelly\\data\\animations\\"
extension = ".sampled_animation"


# File format version. Increment when modifying file format.
version = 1

"""
    blender_anim_exporter.py - v0.01
    
    IMPORTANT:
    !! Make sure to always use front-view (Numpad1) when modeling the front side of the object; we automatically apply a matrix transform to make the object comply with our engine coordinates. !!
    
    I'm using "joint" and "bone" synonymously.

    About this animation exporter:
    - MESH object must selected before running the script.
    - ARMATURE must be direct parent of the MESH object.
    - Only ONE deformation root bone must exist.
    - DOES NOT support non-uniform scaling (the scale value for joints are exported as float NOT Vector3).
    - DOES NOT support rigify rigs; metarigs hierarchy seems fine, but as soon as you generate the rig, the bone hierarchy gets messed up.
    
    NOTE: Exporting from Mixamo:
    - Import one Mixamo FBX with skin, the others should just be armatures with no skin.
    - Click Armature -> apply all transforms -> go to animation tab -> go to graph editor: 
        Filter by location -> cursor x = 1, y = 0 -> pivot point = 2D cursor -> make sure armature is in pose mode + pose postion -> press "a" in viewport to select all bones -> 
        click "a" in graph editor to select all location keys -> click "s + 0.01 + enter" to scale by 0.01 and apply.
    - Go back to layout -> select armature -> export armature to GLTF -> import GLTF:
    - Make sure shading tab is using materals as expected, make sure all anim actions of GLTF armature are used by someone, use exporters like usual.
    
    @Cleanup: The export_joints_meta_data maneuver might be unnecessary, not sure...
    @Cleanup:
    @Cleanup:
"""

from   numpy import allclose
import os
import struct
import array
import bpy
import mathutils

""" GLOBAL VARIABLES
"""
PI = 3.141592653589793

# Matrix to transform from Blender's space coordinates to "our" space coordinates... -90.deg around x, and 180.deg around y.
BlenderToEngineEuler  = mathutils.Euler((-PI * 0.5, PI, 0), 'XYZ')
BlenderToEngineQuat   = BlenderToEngineEuler.to_quaternion()
BlenderToEngineMatrix = BlenderToEngineEuler.to_matrix().to_4x4()

""" UTILS
"""
def append_string(the_list, the_string):
    length  = len(the_string.encode('utf-8'))
    unicode = [ord(c) for c in the_string]
    the_list.append(length)
    the_list.append(unicode) # Append as a list of unicode ints.

""" FUNCTIONS
"""

# We'll use this to return current joint id as we export the joint
num_joints = 0 

def export_joint(data, name, parent_id):
    append_string(data, name)
    data.append(parent_id)
    
    global num_joints
    num_joints += 1
    
    return num_joints-1

def export_joints_meta_data(data, armature):
    #
    # @Sanity: We are doing this because I don't know the order of joints when iterating through aramature.pose.bones
    #

    # We'll return this to remember our order when exporting matrices for each sample.
    joint_names = [] 
    
    # Search for root candidates (deform joints that have no parents)
    bones = armature.pose.bones
    root_candidates = [b for b in bones if not b.parent and b.bone.use_deform == True]
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
    
    root_id = export_joint(data, root.name, -1)
    joint_names.append(root.name)
    
    # recursively export joint names and their parent ids
    for child in root.children:
        recursively_export_joints_meta_data(data, joint_names, child, root_id)
    
    return joint_names

def recursively_export_joints_meta_data(data, joint_names, joint, parent_id):
    if joint.bone.use_deform == False:
        return

    joint_id = export_joint(data, joint.name, parent_id)
    joint_names.append(joint.name)

    for child in joint.children:
      recursively_export_joints_meta_data(data, joint_names, child, joint_id)

def export_all_animations(armature):
    if armature.animation_data == None:
        raise Exception("No animation set for %." % armature.name);
    
    scene = bpy.context.scene
    
    orig_action = armature.animation_data.action
    orig_frame  = scene.frame_current

    for action in (a for a in bpy.data.actions if a.users > 0):
        export_action(armature, action)
    
    armature.animation_data.action = orig_action
    scene.frame_set(orig_frame)

def export_action(armature, action):
    # 
    # Create inter keyframe samples and export them (each action is separate file)
    #
    data     = []
    filepath = directory + anim_name + "_" + action.name + extension
    
    scene = bpy.context.scene
    frame_start = int(action.frame_range.x)
    frame_end   = int(action.frame_range.y)
    frame_range = range(frame_start, frame_end + 1)
    
    frames_per_second = scene.render.fps
    num_samples       = len(frame_range)
    num_joints        = len(armature.pose.bones)
    
    # Set meta-data of animation
    data.append(frames_per_second)
    data.append(num_samples)
    data.append(num_joints)
    
    # Change current action
    armature.animation_data.action = action
    bpy.context.view_layer.update()
    
    joint_names = export_joints_meta_data(data, armature) # This will dictate the order of joints to write.

    # export joints matrices for each sample
    for frame in frame_range:
        scene.frame_set(frame)
        # Set current pose joint info of current frame (in joint-space relative to parent)
        for joint_name in joint_names:
            joint       = armature.pose.bones[joint_name]
            pose_matrix = joint.matrix
            if joint.parent != None:
                pose_matrix = joint.parent.matrix.inverted() @ pose_matrix

            rot   = pose_matrix.to_quaternion().normalized() 
            pos   = pose_matrix.to_translation().xyz          # head position?
            scale = sum(pose_matrix.to_scale()) / 3.0

            # Transform Root to our engine space
            if joint.parent == None:
                rot = BlenderToEngineQuat   @ rot
                pos = BlenderToEngineMatrix @ pos

            data.extend([rot.x, rot.y, rot.z, rot.w])
            data.extend(list(pos))
            data.append(scale)

    write_to_file(filepath, data)

def write_to_file(filepath, data):
    # Overwrite file
    if os.path.exists(filepath):
      os.remove(filepath)
    f = open(filepath, "wb")
    
    f.write(struct.pack("<i", version))
    
    for d in data:
        if isinstance(d, float):
            f.write(struct.pack("<f", d))
        if isinstance(d, int):
            f.write(struct.pack("<i", d))
        if isinstance(d, list):
            array.array('b', d).tofile(f)

    f.close()
    
    print("\"{0}\" exported successfully!".format(filepath))

def main():
    mesh_obj = None
    
    """ SETUP
    """
    # Set object mode.
    bpy.ops.object.mode_set(mode="OBJECT")
    
    # Get the active/selected object. Make sure it's the correct type.
    mesh_obj = bpy.context.view_layer.objects.active
    if mesh_obj.type != "MESH":
        raise Exception("Active object must be of type MESH, not %s." % mesh_obj.type)
    
    # Get armature if available.
    armature = None
    if mesh_obj.parent and mesh_obj.parent.type == "ARMATURE":
        armature = mesh_obj.parent
    
    if armature == None:
        raise Exception("Couldn't extract ARMATURE from %." % mesh_obj.name);
        
    if not allclose(mesh_obj.matrix_world, armature.matrix_world):
        raise Exception("ARMATURE and MESH origins must match!");
    
    armature.data.pose_position = 'POSE'
    export_all_animations(armature)

if __name__ == "__main__":
    main()