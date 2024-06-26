#ifndef ANIMATION_H
#define ANIMATION_H

// @Note: Based on Jonathan Blow's animation system from "Game Engine Programming: Animation Playback" on YouTube

//~ Sampled Animation
//
GLOBAL s32 const SAMPLED_ANIMATION_FILE_VERSION = 1;

struct SQT
{
    Quaternion rotation;    // Q: quaternion
    V3         translation; // T: translation
    f32        scale;       // S: scale
};

inline SQT lerp(SQT a, f32 t, SQT b)
{
    // Should note that if we're using this to blend between animations, it might still give unwanted results. Check out Casey's video about quaternion double-cover and the rest pose neighborhood.
    SQT r = {};
    r.rotation    = nlerp(a.rotation,    t, b.rotation);
    r.translation =  lerp(a.translation, t, b.translation);
    r.scale       =  lerp(a.scale,       t, b.scale);
    return r;
}

inline M4x4 m4x4_from_sqt(SQT xform)
{
    M4x4 r = m4x4_from_translation_rotation_scale(xform.translation, xform.rotation, v3(xform.scale));
    return r;
}

struct Pose_Joint_Info
{
    // Joint-space matrices (relative to parent) for all keyframes (i.e. array.count == num_samples)
    Array<SQT> matrices;
    String8    name;
    s32        parent_id;
};

struct Sampled_Animation
{
    Array<Pose_Joint_Info> joints; // array.count == num_joints
    String8 name;
    
    f64 duration;    // In seconds.
    s32 num_samples; // Number of frames exported from 3D software for this animation.
    s32 frame_rate;  // Frames/samples per second
};

//~ Animation Channel
//

struct Animation_Channel
{
    // In SQT form because the animation player could cross-fade/blend with other animation channels.
    Array<SQT> lerped_joints_relative;
    
    Sampled_Animation *animation;
    
    f64 animation_duration; // Copied from sampled animation.
    f64 current_time;       // Local clock - will we run into synchronization issues?
    f64 old_time;
    
    // For blending with other channels. We set these from the outside (from some higher level thing) when playing animations.
    f64 last_blend_factor;
    f64 blend_duration;
    f64 blend_t;
    b32 blending_in;
    b32 blending_out;
    
    f32 time_multiplier;
    b32 is_looping;
    b32 is_completed;
    //b32 is_active;
};

#define DISABLE_LERPS FALSE

// @Todo: Animation Player
// @Todo: In set mesh function, decide the blend_mode for the anim player according to Casey's video?

struct Animation_Player
{
    Array<M4x4>               skinning_matrices;
    Array<SQT>                blended_joints_relative;
    Array<Animation_Channel*> channels;
    
    Triangle_Mesh *mesh;
    
    f64 current_time;
    f64 current_dt;
    
    s32 num_changed_channels_last_eval;
    
    // @Todo: Different blend modes (neighborhood with rest pose / invert / direct)
    // @Todo: remove locomotion
};

#endif //ANIMATION_H
