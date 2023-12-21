
FUNCTION void print_sqts(Array<SQT> transforms)
{
    debug_print("{ ");
    for (s32 i = 0; i < transforms.count; i++) {
        debug_print("{%f, %v4, %v3}, ", transforms[i].scale, transforms[i].rotation, transforms[i].translation);
    }
    debug_print("} \n\n");
}

//~ Sampled Animation stuff
//
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

//~ Animation Channel stuff
//

FUNCTION void set_animation(Animation_Channel *channel, Sampled_Animation *anim, f64 t0)
{
    if (anim) {
        if (anim->num_samples == 0 || anim->joints.count == 0) {
            anim = 0;
            debug_print("Animation %S has no samples or joints, can't set it for channel.\n", anim->name);
        }
    }
    
    channel->animation_duration = 0.0;
    
    if (!anim) return;
    
    channel->animation          = anim;
    channel->animation_duration = anim->duration;
    
    channel->current_time    = t0;
    channel->time_multiplier = 1.0f;
    channel->should_loop     = TRUE;
    channel->completed       = FALSE;
    channel->is_active       = TRUE;
    
    array_init_and_resize(&channel->lerped_joints_relative, anim->joints.count);
}

FUNCTION void advance_time(Animation_Channel *channel, f32 dt)
{
    // Advance current_time by dt.
    channel->old_time      = channel->current_time;
    channel->current_time += dt * channel->time_multiplier;
    
    if ((channel->old_time != channel->current_time) && (channel->current_time >= channel->animation_duration)) {
        if (!channel->should_loop) {
            channel->completed    = TRUE;
            channel->current_time = channel->animation_duration;
            
            return;
        }
        
        channel->current_time -= channel->animation_duration;
    }
    
}

FUNCTION void get_lerped_joints(Sampled_Animation *anim, f64 time, Array<SQT> *joints_out)
{
    // @Todo: max_index parameter? What is it for? To ignore some joint children?
    
    s32 num_joints = (s32)anim->joints.count;
    if (!num_joints) return;
    
    s32 num_samples = anim->num_samples;
    if (!num_samples) return;
    
    // Figure out which samples we lie between.
    f64 phase      = time / anim->duration;
    s32 base_index = (s32)(phase * (f64)(num_samples - 1)); // Index of base sample.
    base_index     = CLAMP(0, base_index, num_samples - 1);
    
    // Calculate the fraction/t-value between the two samples.
    f64 fraction = 0.0;
    if (num_samples > 1){
        // Make sure we at least have two samples.
        
        f64 time_per_sample = anim->duration / (f64)(num_samples - 1);
        f64 base_time       = time_per_sample * base_index;
        
        fraction = (time - base_time) / time_per_sample;
    }
    fraction = CLAMP(0.0, fraction, 1.0);
    
    // Calculate the lerped joint transforms.
    for (s32 i = 0; i < num_joints; i++) {
        Pose_Joint_Info *joint = &anim->joints[i];
        if (DISABLE_LERPS) {
            joints_out->data[i] = joint->matrices[base_index];
        } else {
            SQT a = joint->matrices[base_index + 0];
            SQT b = joint->matrices[base_index + 1];
            
            joints_out->data[i] = lerp(a, (f32)fraction, b);
        }
    }
    
}

FUNCTION b32 eval(Animation_Channel *channel)
{
    // Figure out between which two samples of the animation we lie (based on current_time) and 
    // lerp the joint transforms.
    
    if (!channel->animation)
        return false;
    
    if (channel->completed)
        return false;
    
    ASSERT(channel->lerped_joints_relative.count == channel->animation->joints.count);
    get_lerped_joints(channel->animation, channel->current_time, &channel->lerped_joints_relative);
    
    return true;
}