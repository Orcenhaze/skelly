
FUNCTION b32 load_sampled_animation(Sampled_Animation *anim, String8 full_path)
{
    String8 file = os->read_entire_file(full_path);
    if (!file.data) {
        debug_print("SAMPLED ANIMATION LOAD ERROR: Couldn't load animation at %S\n", full_path);
        return false;
    }
    String8 orig_file = file;
    
    //-
    
    //
    // @Todo: Probably shouldn't use permanent_arena here.
    //
    
    String8 name = extract_base_name(full_path);
    anim->name   = str8_copy(os->permanent_arena, name);
    
    s32 version = 0;
    get(&file, &version);
    ASSERT(version <= SAMPLED_ANIMATION_FILE_VERSION);
    
    // Samples per second
    get(&file, &anim->frame_rate);
    if (anim->frame_rate <= 0) {
        anim->frame_rate = 1;
        debug_print("Animation file %S had frame rate of zero, setting to %d\n", full_path, anim->frame_rate);
    }
    
    // Number of samples
    get(&file, &anim->num_samples);
    
    // Number of joints
    s32 num_joints;
    get(&file, &num_joints);
    array_init_and_resize(&anim->joints, num_joints);
    
    // Calculate duration of animation from number of samples and samples per second
    anim->duration = 1.0f;
    if (anim->joints.data) {
        anim->duration = anim->num_samples / (f64)anim->frame_rate;
    }
    
    // Read Joint name and parent id.
    for (s32 joint_index = 0; joint_index < anim->joints.count; joint_index++) {
        Pose_Joint_Info *joint = &anim->joints[joint_index];
        
        // Read name
        s32 joint_name_len = 0;
        get(&file, &joint_name_len);
        String8 joint_name = str8(file.data, joint_name_len);
        advance(&file, joint_name_len);
        joint->name = str8_copy(os->permanent_arena, joint_name);
        
        // Read parent id
        get(&file, &joint->parent_id);
        
        // Allocate memory for matrices
        array_init_and_resize(&joint->matrices, anim->num_samples);
    }
    
    
    for (s32 sample_index = 0; sample_index < anim->num_samples; sample_index++) {
        for (s32 joint_index = 0; joint_index < anim->joints.count; joint_index++) {
            Pose_Joint_Info *joint = &anim->joints[joint_index];
            
            get(&file, &joint->matrices[sample_index]);
        }
    }
    
    
    //-
    
    ASSERT(file.count == 0);
    
    os->free_file_memory(orig_file.data);
    return true;
}