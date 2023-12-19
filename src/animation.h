#ifndef ANIMATION_H
#define ANIMATION_H

GLOBAL s32 const SAMPLED_ANIMATION_FILE_VERSION = 1;

struct SQT
{
    Quaternion rotation;    // Q: quaternion
    V3         translation; // T: translation
    f32        scale;       // S: scale
};

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
    
    f64 duration;    // in seconds.
    s32 num_samples; // number of frames exported from 3D software for this animation.
    s32 frame_rate;  // frames/samples per second - won't use "sample_rate" to not confuse with audio.
    
    b32 is_looping;
};


#endif //ANIMATION_H
