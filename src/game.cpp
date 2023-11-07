
#include "mesh.h"

#include "mesh.cpp"

GLOBAL V3 V3F = {0,  0, -1};
GLOBAL V3 V3U = {0,  1,  0};
GLOBAL V3 V3R = {1,  0,  0};

struct Camera
{
    V3 position;
    V3 forward;
    M4x4_Inverse matrix; // look_at matrix.
};
GLOBAL Camera camera;

FUNCTION void game_init()
{
    camera.position = v3(0, 0, 1);
    camera.forward  = v3(0, 0, -1);
    
    set_view_to_proj();
}

FUNCTION void control_camera(Camera *cam, V3 delta_mouse)
{
    f32 dt = os->dt;
    
    V3 cam_p = cam->position;
    V3 cam_f = cam->forward;
    V3 cam_r = normalize0(cross(cam_f, V3U));
    
    f32 speed = 1.5f;
    if (key_held(Key_W)) 
        cam_p +=  cam_f * speed * dt;
    if (key_held(Key_S))
        cam_p += -cam_f * speed * dt;
    if (key_held(Key_D))
        cam_p +=  cam_r * speed * dt;
    if (key_held(Key_A))
        cam_p += -cam_r * speed * dt;
    if (key_held(Key_E))
        cam_p +=  V3U * speed * dt;
    if (key_held(Key_Q))
        cam_p += -V3U * speed * dt;
    
    if (key_held(Key_MMIDDLE)) {
        Quaternion yaw = quaternion_from_axis_angle(V3U, -speed*delta_mouse.x);
        cam_f = yaw * cam_f;
        
        Quaternion pitch = quaternion_from_axis_angle(cam_r, speed*delta_mouse.y);
        if(ABS(dot(V3U, pitch*cam_f)) < 0.98f)
            cam_f = pitch*cam_f;
    }
    
    cam->position = cam_p;
    cam->forward  = cam_f;
    cam->matrix   = look_at(cam->position, 
                            cam->position + cam->forward, 
                            V3U);
}

FUNCTION void game_update()
{
    LOCAL_PERSIST V3 old_ndc = {};
    V3 delta_mouse = os->mouse_ndc - old_ndc;
    old_ndc        = os->mouse_ndc;
    
    control_camera(&camera, delta_mouse);
    set_world_to_view(camera.matrix);
}

FUNCTION void game_render()
{
    immediate_begin();
    set_texture(0);
    immediate_rect(v2(0), v2(1), v4(1));
    immediate_end();
}