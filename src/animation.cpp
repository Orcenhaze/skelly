
FUNCTION void print_sqts(Array<SQT> transforms)
{
    debug_print("{ ");
    for (s32 i = 0; i < transforms.count; i++) {
        debug_print("{%f, %v4, %v3}, ", transforms[i].scale, transforms[i].rotation, transforms[i].translation);
    }
    debug_print("} \n\n");
}

//~ Sampled Animation
//
FUNCTION b32 load_sampled_animation(Arena *arena, Sampled_Animation *anim, String8 full_path)
{
    String8 file = os->read_entire_file(full_path);
    if (!file.data) {
        debug_print("SAMPLED ANIMATION LOAD ERROR: Couldn't load animation at %S\n", full_path);
        return false;
    }
    String8 orig_file = file;
    
    //-
    
    String8 name = extract_base_name(full_path);
    anim->name   = str8_copy(arena, name);
    
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
        joint->name = str8_copy(arena, joint_name);
        
        // Read parent id
        get(&file, &joint->parent_id);
        
        // Allocate memory for matrices
        array_init_and_resize(&joint->matrices, anim->num_samples);
    }
    
    
    // Read joint matrices.
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

//~ Animation Channel
//
FUNCTION void init(Animation_Channel *channel)
{
    array_init(&channel->lerped_joints_relative);
}

FUNCTION void destroy(Animation_Channel **channel)
{
    if (!channel || !*channel)
        return;
    
    Animation_Channel *ch = *channel;
    array_free(&ch->lerped_joints_relative);
    
    free(ch);
    *channel = 0;
}

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
    channel->is_looping      = TRUE;
    channel->is_completed    = FALSE;
    //channel->is_active       = TRUE;
    
    array_resize(&channel->lerped_joints_relative, anim->joints.count);
}

FUNCTION void advance_time(Animation_Channel *channel, f64 dt)
{
    if (!channel) return;
    
    // Advance current_time by dt.
    channel->old_time      = channel->current_time;
    channel->current_time += dt * channel->time_multiplier;
    
    if ((channel->old_time != channel->current_time) && (channel->current_time >= channel->animation_duration)) {
        if (!channel->is_looping) {
            channel->is_completed = TRUE;
            channel->current_time = channel->animation_duration;
            
            return;
        }
        
        channel->current_time -= channel->animation_duration;
    }
    
}

FUNCTION void get_lerped_joints(Sampled_Animation *anim, f64 time, Array<SQT> joints_out)
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
            joints_out[i] = joint->matrices[base_index];
        } else {
            SQT a = joint->matrices[base_index + 0];
            SQT b = joint->matrices[base_index + 1];
            
            joints_out[i] = lerp(a, (f32)fraction, b);
        }
    }
    
}

FUNCTION b32 eval(Animation_Channel *channel)
{
    // Figure out between which two samples of the animation we lie (based on current_time) and 
    // lerp the joint transforms.
    
    if (!channel->animation)
        return false;
    
    if (channel->is_completed)
        return false;
    
    ASSERT(channel->lerped_joints_relative.count == channel->animation->joints.count);
    get_lerped_joints(channel->animation, channel->current_time, channel->lerped_joints_relative);
    
    return true;
}

//~ Animation Player
//
FUNCTION void init(Animation_Player *player)
{
    //table_init(&player->joint_name_to_index_map);
    array_init(&player->skinning_matrices);
    array_init(&player->blended_joints_relative);
    array_init(&player->channels);
}

FUNCTION void destroy(Animation_Player **player)
{
    if (!player || !*player)
        return;
    
    Animation_Player *pl = *player;
    
    //table_free(&pl->joint_name_to_index_map);
    array_free(&pl->skinning_matrices);
    array_free(&pl->blended_joints_relative);
    
    for (s32 i = 0; i < pl->channels.count; i++)
        destroy(&pl->channels[i]);
    
    array_free(&pl->channels);
    
    free(pl);
    *player = 0;
}

FUNCTION void set_mesh(Animation_Player *player, Triangle_Mesh *mesh)
{
    if (!mesh || ((mesh->flags & MeshFlags_ANIMATED) == 0)) return;
    
    //table_reset(&player->joint_name_to_index_map);
    
    Skeleton *skeleton = mesh->skeleton;
    if (!skeleton) {
        debug_print("Mesh %S doesn't have a skeleton, can't set it on animation player!\n", mesh->name);
        return;
    }
    
    // Resize and initialize final skinning matrices to identity.
    array_resize(&player->skinning_matrices, skeleton->joint_info.count);
    for (s32 i = 0; i < player->skinning_matrices.count; i++)
        player->skinning_matrices[i] = m4x4_identity();
    
    player->mesh = mesh;
}

FUNCTION Animation_Channel* add_animation_channel(Animation_Player *player)
{
    Animation_Channel *channel = (Animation_Channel *) malloc(sizeof(Animation_Channel));
    ASSERT(channel);
    MEMORY_ZERO(channel, sizeof(Animation_Channel));
    init(channel);
    array_add(&player->channels, channel);
    
    return channel;
}

FUNCTION void advance_time(Animation_Player *player, f64 dt)
{
    if (!player) return;
    
    for (s32 i = 0; i < player->channels.count; i++) {
        Animation_Channel *channel = player->channels[i];
        advance_time(channel, dt);
        
        if (channel->blending_in || channel->blending_out) {
            channel->blend_t += dt;
            
            if ((channel->blend_t > channel->blend_duration) &&
                (channel->blending_out)) {
                destroy(&player->channels[i]);
                array_ordered_remove_by_index(&player->channels, i);
            }
        }
    }
    
    player->current_time += dt;
    player->current_dt    = dt;
}

FUNCTION void eval(Animation_Player *player)
{
    if (!player || !player->channels.count) return;
    
    if (!player->mesh) return;
    
    ASSERT(player->mesh->skeleton);
    
    Skeleton *skeleton = player->mesh->skeleton;
    
    // @Sanity: @Todo: For now, assume that mesh skeleton joints match joints of every channel in
    // terms of names and order/indices. Our mesh and animation exporters are synched when writing
    // the joint data. If anything, we can make sure everything matched at load time.
    // @Note: Because of this assumption. The animation player and channel doesn't hold any info about
    // joint names, instead, we can rely on the mesh skeleton to get that info. But since the animation
    // player allows you to add _any_ channel, and if we want to support some kind of "re-targeting",
    // it would probably be best if we did store pose joint names somewhere and only apply transforms 
    // to matching joints from the mesh skeleton.
    for (s32 i = 0; i < player->channels.count; i++) {
        ASSERT(player->skinning_matrices.count == player->channels[i]->lerped_joints_relative.count);
    }
    
    //
    // Evaluate all channels.
    //
    b32 num_changed = 0;
    for (s32 i = 0; i < player->channels.count; i++) {
        b32 changed = eval(player->channels[i]);
        if (changed) num_changed++;
    }
    player->num_changed_channels_last_eval = num_changed;
    
    //
    // Calculate blending factor for cross-fading with other channels.
    //
    for (s32 i = 0; i < player->channels.count; i++) {
        Animation_Channel *channel = player->channels[i];
        
        f64 blend_factor = 1.0;
        if (channel->blending_out)
            blend_factor = 1.0 - (channel->blend_t / channel->blend_duration);
        else if (channel->blending_in)
            blend_factor = (channel->blend_t / channel->blend_duration);
        
        blend_factor = CLAMP01(blend_factor);
        
        if (blend_factor != channel->last_blend_factor) num_changed++;
        channel->last_blend_factor = blend_factor;
    }
    
    if (!num_changed)
        return;
    
    //
    // Blend with other animation channels (cross-fade).
    //
    array_resize(&player->blended_joints_relative, player->channels[0]->lerped_joints_relative.count);
    b32 first = TRUE;
    for (s32 i = 0; i < player->channels.count; i++) {
        Animation_Channel *channel = player->channels[i];
        ASSERT(channel->lerped_joints_relative.count == player->blended_joints_relative.count);
        
        for (s32 joint_index = 0; joint_index < channel->lerped_joints_relative.count; joint_index++) {
            SQT joint = channel->lerped_joints_relative[joint_index];
            
            if (first) {
                player->blended_joints_relative[joint_index] = joint;
            }
            else {
                // @Incomplete:
                // @Incomplete:
                // @Todo: We are not doing Casey's neighborhood thing, and we should!
                // We should figure out which blend mode to use:
                // neighborhood with rest pose vs. invert vs. direct.
                // For now we will just do the dumb thing and lerp the old way.
                //
                f64 t = channel->last_blend_factor;
                if (t != 1) {
                    SQT lerped = lerp(player->blended_joints_relative[joint_index], (f32)t, joint);
                    player->blended_joints_relative[joint_index] = lerped;
                } else {
                    player->blended_joints_relative[joint_index] = joint;
                }
            }
        }
        
        first = FALSE;
    }
    
    //
    // Calculate global matrices (object/mesh space) from local matrices (joint-space relative to parent).
    // Could store global matrices in separate array, but we'll store them in skinning matrices for now.
    //
    for (s32 i = 0; i < player->blended_joints_relative.count; i++) {
        s32 parent_index = skeleton->joint_info[i].parent_id;
        
        M4x4 m = m4x4_from_sqt(player->blended_joints_relative[i]);
        if (parent_index >= 0) {
            M4x4 parent_matrix = player->skinning_matrices[parent_index];
            m = parent_matrix * m;
        }
        
        player->skinning_matrices[i] = m;
    }
    
    //
    // Calculate skinning matrices by including the "inverse rest pose matrix" from skeleton.
    //
    for (s32 i = 0; i < player->skinning_matrices.count; i++) {
        M4x4 object_to_joint         = skeleton->joint_info[i].object_to_joint_matrix;
        player->skinning_matrices[i] = player->skinning_matrices[i] * object_to_joint;
    }
    
    // @Todo: Remove locomotion?
}

#if DEVELOPER
FUNCTION void skin_mesh(Animation_Player *player)
{
    // @Note: We skin on CPU only for things like mouse-picking in editor.
    
    if (!player || !player->mesh || !player->mesh->skeleton) return;
    
    Triangle_Mesh *mesh = player->mesh;
    Skeleton *sk = mesh->skeleton;
    for (s32 i = 0; i < mesh->vertices.count; i++) {
        s32 canonical_vertex_index    = mesh->canonical_vertex_map[i];
        Vertex_Blend_Info *blend_info = &sk->vertex_blend_info[canonical_vertex_index];
        
        V4 p = {};
        //V3 n = {};
        
        for (s32 piece_index = 0; piece_index < blend_info->num_pieces; piece_index++) {
            Vertex_Blend_Piece piece = blend_info->pieces[piece_index];
            M4x4 m = player->skinning_matrices[piece.joint_id];
            p     += transform(m, v4(mesh->vertices[i], 1.0f)) * piece.weight;
            //n += w * transform(m, v4(mesh->tbns[i].normal, 0.0f)).xyz;
        }
        
        if (p.w)
            p.xyz /= p.w;
        
        mesh->skinned_vertices[i] = p.xyz;
    }
}
#endif