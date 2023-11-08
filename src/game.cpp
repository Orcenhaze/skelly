
// @Cleanup: 
// @Cleanup: Move to appropriate files!!
// @Cleanup:

#include "mesh.h"

struct Entity
{
    String8 name;
    
    V3           position;
    Quaternion   orientation;
    M4x4_Inverse object_to_world_matrix;
    
    Triangle_Mesh *mesh;
};

FUNCTION void update_entity_transform(Entity *entity)
{
    V3 pos         = entity->position;
    Quaternion ori = entity->orientation;
    
    M4x4 m_non = m4x4_identity();
    m_non._14 = pos.x;
    m_non._24 = pos.y;
    m_non._34 = pos.z;
    //m_non._11 = scale.x;
    //m_non._22 = scale.y;
    //m_non._33 = scale.z;
    
    M4x4 m_inv = m4x4_identity();
    m_inv._14 = -pos.x;
    m_inv._24 = -pos.y;
    m_inv._34 = -pos.z;
    //m_inv._11 = 1.0f/scale.x;
    //m_inv._22 = 1.0f/scale.y;
    //m_inv._33 = 1.0f/scale.z;
    
    M4x4 r = m4x4_from_quaternion(ori);
    
    entity->object_to_world_matrix.forward = m_non * r;
    entity->object_to_world_matrix.inverse = transpose(r) * m_inv;
}


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

#include "mesh.cpp"
#include "draw.cpp"

GLOBAL Triangle_Mesh guy_mesh;
GLOBAL Entity guy;

FUNCTION void game_init()
{
    camera.position = v3(0, 0, 1);
    camera.forward  = v3(0, 0, -1);
    
    set_view_to_proj();
    
    // Mesh init.
    guy_mesh.name = S8LIT("guy");
    load_triangle_mesh(os->permanent_arena, &guy_mesh);
    
    // Entity init.
    guy.name        = S8LIT("guy");
    guy.position    = {};
    guy.orientation = quaternion_identity();
    guy.mesh        = &guy_mesh;
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
    draw_entity(&guy);
}