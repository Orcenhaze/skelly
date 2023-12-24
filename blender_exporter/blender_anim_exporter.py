anim_name = ""

directory = "C:\\work\\skelly\\data\\animations\\"
extension = ".sampled_animation"


version = 1

"""
    About this animation exporter:
    - MESH object must selected before running the script.
    - ARMATURE must be direct parent of the MESH object.
    - Exports joints' scale value as float NOT Vector3 (we don't support non-uniform scaling).
    
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
# Matrix to transform from Blender's space coordinates to "our" space coordinates.
# Rotate -90deg around x-axis.
BlenderToEngineQuat   = mathutils.Quaternion((0.707, -0.707, 0.0, 0.0))
BlenderToEngineMatrix = mathutils.Quaternion((0.707, -0.707, 0.0, 0.0)).to_matrix().to_4x4()

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
    
    # Search for root candidates
    bones = armature.pose.bones
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
    
    root_id = export_joint(data, root.name, -1)
    joint_names.append(root.name)
    
    # recursively export joint names and their parent ids
    for child in root.children:
        recursively_export_joints_meta_data(data, joint_names, child, root_id)
    
    return joint_names

def recursively_export_joints_meta_data(data, joint_names, joint, parent_id):
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
    
    # Get armature if available and set pose position to REST pose.
    armature = None
    if mesh_obj.parent and mesh_obj.parent.type == "ARMATURE":
        armature = mesh_obj.parent
        if not allclose(mesh_obj.matrix_world, armature.matrix_world):
            raise Exception("ARMATURE and MESH origins must match!");
        armature.data.pose_position = 'POSE'

    if armature == None:
        raise Exception("Couldn't extract ARMATURE from %." % mesh_obj.name);
    
    export_all_animations(armature)

if __name__ == "__main__":
    main()