/*

// @Note: This file is NOT independant. It's using the engine's types, collision routines and immediate-mode renderer.
    
// @Incomplete: Add scale gizmo!

*/ 

enum Gizmo_Mode
{
    GizmoMode_TRANSLATION,
    GizmoMode_ROTATION,
    GizmoMode_SCALE,
};

enum Gizmo_Element
{
    GizmoElement_NONE,
    
    GizmoElement_TRANSLATE_X,
    GizmoElement_TRANSLATE_Y,
    GizmoElement_TRANSLATE_Z,
    
    GizmoElement_TRANSLATE_YZ,
    GizmoElement_TRANSLATE_ZX,
    GizmoElement_TRANSLATE_XY,
    
    GizmoElement_ROTATE_X,
    GizmoElement_ROTATE_Y,
    GizmoElement_ROTATE_Z,
    GizmoElement_ROTATE_C, // Around camera axis.
};

GLOBAL V4 colors[] =
{
    {},
    {1.0f, 0.0f, 0.0f, 0.8f}, // x-axis
    {0.0f, 1.0f, 0.0f, 0.8f}, // y-axis
    {0.0f, 0.0f, 1.0f, 0.8f}, // z-axis
    
    {1.0f, 1.0f, 0.0f, 0.5f}, // yz-plane
    {1.0f, 1.0f, 0.0f, 0.5f}, // zy-plane
    {1.0f, 1.0f, 0.0f, 0.5f}, // xy-plane
    
    {1.0f, 0.0f, 0.0f, 0.8f}, // x-ring
    {0.0f, 1.0f, 0.0f, 0.8f}, // y-ring
    {0.0f, 0.0f, 1.0f, 0.8f}, // z-ring
    {1.0f, 1.0f, 0.0f, 0.5f}, // c-ring that rotates around camera
};

struct Gizmo_Rendering_Params 
{
    f32 scale;       // Used for length of axes.
    f32 threshold;   // Used for picking element candidates.
    f32 thickness;   // Used for thickness of axis lines and rotation rings.
    f32 plane_scale; // Used for translation planes.
};

struct Gizmo_Geometry
{
    Ray    axes   [3];
    Plane  planes [3];
    Circle circles[4];
};

GLOBAL b32            gizmo_is_active; // Whether we are interacting with the gizmo. True when we find the best gizmo element candidate and press the left mouse button.
GLOBAL Gizmo_Element  gizmo_element;   // The current best candidate (could be none) based on distance and intersection with corresponding geometry of the element. 
GLOBAL Gizmo_Mode     gizmo_mode;
GLOBAL const f32      gizmo_max_t = 300.0f; // Max limit for camera_ray.t when intersecting.

GLOBAL Gizmo_Geometry         geometry;
GLOBAL Gizmo_Rendering_Params params;

GLOBAL V3                     current_point;  // Intersection point of gizmo_element's geometry in current frame.
GLOBAL V3                     previous_point; // Intersection point of gizmo_element's geometry in previous frame.
GLOBAL V3                     click_point;    // Intersection point of gizmo_element's geometry in first frame where gizmo became active.
GLOBAL V3                     click_offset;   // The vector from gizmo_origin to click_point in first frame where gizmo became active.

GLOBAL V4 gizmo_active_color = {1.0f, 1.0f, 1.0f, 1.0f};

FUNCTION void gizmo_calculate_rendering_params(V3 camera_position, V3 gizmo_origin)
{
    f32 distance_to_camera    = length(camera_position - gizmo_origin);
    
    params.scale              = distance_to_camera / 5.00f; // @Hardcode: scale.
    params.threshold          = params.scale       * 0.07f; // @Hardcode: threshold ratio.
    params.thickness          = params.scale       * 0.03f; // @Hardcode: thickness ratio.
    params.plane_scale        = params.scale       * 0.50f; // @Hardcode: plane_scale ratio.
}

FUNCTION void gizmo_calculate_geometry(V3 camera_position, V3 gizmo_origin)
{
    V3 origin_to_camera = normalize_or_zero(camera_position - gizmo_origin);
    
    geometry.axes[0]    = {gizmo_origin, {1.0f, 0.0f, 0.0f}, F32_MAX};
    geometry.axes[1]    = {gizmo_origin, {0.0f, 1.0f, 0.0f}, F32_MAX};
    geometry.axes[2]    = {gizmo_origin, {0.0f, 0.0f, 1.0f}, F32_MAX};
    
    f32 plane_scale     = params.plane_scale;
    V3 sx               = {0.0f, plane_scale, plane_scale};
    V3 sy               = {plane_scale, 0.0f, plane_scale};
    V3 sz               = {plane_scale, plane_scale, 0.0f};
    
    geometry.planes[0]  = {gizmo_origin + sx, {1.0f, 0.0f, 0.0f}};
    geometry.planes[1]  = {gizmo_origin + sy, {0.0f, 1.0f, 0.0f}};
    geometry.planes[2]  = {gizmo_origin + sz, {0.0f, 0.0f, 1.0f}};
    
    f32 scale           = params.scale;
    geometry.circles[0] = {gizmo_origin, {1.0f, 0.0f, 0.0f}, scale+(scale*0.10f)};
    geometry.circles[1] = {gizmo_origin, {0.0f, 1.0f, 0.0f}, scale+(scale*0.05f)};
    geometry.circles[2] = {gizmo_origin, {0.0f, 0.0f, 1.0f}, scale+(scale*0.00f)};
    geometry.circles[3] = {gizmo_origin,   origin_to_camera, scale+(scale*0.30f)};
}

FUNCTION void gizmo_render()
{
    f32 s  = params.scale;
    f32 th = params.thickness;
    f32 ps = params.plane_scale;
    
    d3d11_clear_depth();
    immediate_begin();
    set_texture(0);
    
    if (gizmo_mode == GizmoMode_TRANSLATION) {
        if (!gizmo_is_active) {
            // Draw all translation elements
            for (s32 i = 0; i < 3; i++) {
                Ray axis    = geometry.axes[i];
                Plane plane = geometry.planes[i];
                
                V4 axis_c   = (gizmo_element == GizmoElement_TRANSLATE_X + i)? gizmo_active_color : colors[GizmoElement_TRANSLATE_X + i];
                V4 plane_c  = (gizmo_element == GizmoElement_TRANSLATE_YZ + i)? gizmo_active_color : colors[GizmoElement_TRANSLATE_YZ + i];
                
                immediate_arrow(axis.o, axis.d, s, axis_c, th);
                immediate_rect_3d(plane.center, plane.normal, 0.5f*ps, plane_c);
            }
        } else {
            // Draw selected/active translation element.
            if (gizmo_element <= GizmoElement_TRANSLATE_Z) {
                s32 index = gizmo_element - GizmoElement_TRANSLATE_X;
                Ray axis  = geometry.axes[index];
                V3 p0     = click_point - axis.d * 2000.0f;
                V3 p1     = click_point + axis.d * 2000.0f;
                immediate_line_3d(p0, p1, colors[gizmo_element], 0.01f);
            } else if (gizmo_element <= GizmoElement_TRANSLATE_XY) {
                // When interacting with a plane, draw the other two axes.
                s32 index = gizmo_element - GizmoElement_TRANSLATE_YZ;
                Ray axis  = geometry.axes[(index + 1) % 3];
                V3 p0     = click_point - axis.d * 2000.0f;
                V3 p1     = click_point + axis.d * 2000.0f;
                immediate_line_3d(p0, p1, colors[(index + 1) % 3 + 1], 0.01f);
                axis      = geometry.axes[(index + 2) % 3];
                p0        = click_point - axis.d * 2000.0f;
                p1        = click_point + axis.d * 2000.0f;
                immediate_line_3d(p0, p1, colors[(index + 2) % 3 + 1], 0.01f);
            }
        }
    } else if (gizmo_mode == GizmoMode_ROTATION) {
        if (!gizmo_is_active) {
            // Draw all circles/rings.
            for (s32 i = 0; i < 4; i++) {
                Circle circle = geometry.circles[i];
                V4 circle_c   = (gizmo_element == GizmoElement_ROTATE_X + i)? gizmo_active_color : colors[GizmoElement_ROTATE_X + i];
                immediate_torus(circle.center, circle.radius, circle.normal, circle_c, th);
            }
        } else {
            // Draw selected/active circle.
            s32 index     = gizmo_element - GizmoElement_ROTATE_X;
            Circle circle = geometry.circles[index];
            immediate_line_3d(circle.center, click_point, gizmo_active_color, params.thickness*0.20f);
            immediate_line_3d(circle.center, current_point, gizmo_active_color, params.thickness*0.50f);
            immediate_torus(circle.center, circle.radius, circle.normal, colors[gizmo_element], params.thickness);
        }
    }
    
    immediate_end();
}

FUNCTION void gizmo_execute(Ray camera_ray, V3 gizmo_origin, 
                            V3         *delta_position_out, 
                            Quaternion *delta_rotation_out, 
                            V3         *delta_scale_out)
{
    *delta_position_out = {0.0f, 0.0f, 0.0f};
    *delta_rotation_out = quaternion_identity();
    *delta_scale_out    = {0.0f, 0.0f, 0.0f};
    
    f32 best_dist = F32_MAX;
    f32 best_t    = F32_MAX;
    b32 is_close  = false;
    
    gizmo_calculate_rendering_params(camera_ray.origin, gizmo_origin);
    gizmo_calculate_geometry(camera_ray.origin, gizmo_origin);
    
    //
    // If we're not interacting with the gizmo, find the "best" gizmo element candidate based on distance and intersection.
    //
    if (!gizmo_is_active) {
        if (gizmo_mode == GizmoMode_TRANSLATION) {
            // Axes.
            for (s32 i = 0; i < 3; i++) {
                Ray *axis   = &geometry.axes[i];
                f32 dist    = closest_distance_ray_ray(&camera_ray, axis);
                b32 t_valid = (axis->t > 0) && (axis->t < params.scale);
                if ((dist < best_dist) && (dist < params.threshold) && (t_valid)) {
                    best_dist     = dist;
                    is_close      = true;
                    gizmo_element = (Gizmo_Element)(GizmoElement_TRANSLATE_X + i);
                }
            }
            
            // Planes.
            for (s32 i = 0; i < 3; i++) {
                Plane plane = geometry.planes[i];
                if (ray_plane_intersect(&camera_ray, plane)) {
                    V3 on_plane = camera_ray.o + camera_ray.t*camera_ray.d;
                    V3 offset   = on_plane - plane.center; 
                    b32 is_hit  = ((ABS(offset.x) <= 0.5f*params.plane_scale) &&
                                   (ABS(offset.y) <= 0.5f*params.plane_scale) &&
                                   (ABS(offset.z) <= 0.5f*params.plane_scale));
                    if (is_hit && (camera_ray.t < best_t)) {
                        best_t        = camera_ray.t;
                        is_close      = true;
                        gizmo_element = (Gizmo_Element)(GizmoElement_TRANSLATE_YZ + i);
                    }
                }
            }
        } else if (gizmo_mode == GizmoMode_ROTATION) {
            for (s32 i = 0; i < 4; i++) {
                V3  center = geometry.circles[i].center;
                f32 radius = geometry.circles[i].radius;
                V3  normal = geometry.circles[i].normal;
                
                Plane circle_plane = {center, normal};
                
                if (ray_plane_intersect(&camera_ray, circle_plane)) {
                    V3 on_plane  = camera_ray.o + camera_ray.t*camera_ray.d;
                    V3 on_circle = center + radius*normalize(on_plane - center);
                    f32 dist     = length(on_plane - on_circle);
                    if ((dist < best_dist) && (dist < params.threshold) && (camera_ray.t < best_t)) {
                        best_dist     = dist;
                        best_t        = camera_ray.t;
                        is_close      = true;
                        gizmo_element = (Gizmo_Element)(GizmoElement_ROTATE_X + i);
                    }
                }
            }
        }
    }
    
    //
    // If we found the "best" element, update the intersection point with its geometry every frame.
    //
    if (gizmo_element != GizmoElement_NONE) {
        if (gizmo_element <= GizmoElement_TRANSLATE_Z) {
            // The element is an axis.
            s32 index = gizmo_element - GizmoElement_TRANSLATE_X;
            Ray axis  = geometry.axes[index];
            V3 ignored, plane_normal;
            calculate_tangents(axis.d, &plane_normal, &ignored);
            Plane plane = {gizmo_origin, plane_normal};
            b32 is_hit  = ray_plane_intersect(&camera_ray, plane);
            if (is_hit && (camera_ray.t < gizmo_max_t)) {
                V3 on_plane   = camera_ray.o + camera_ray.t*camera_ray.d;
                current_point = plane.center + axis.d * dot(on_plane - plane.center, axis.d);
            }
        } else if (gizmo_element <= GizmoElement_TRANSLATE_XY) {
            // The element is a plane.
            s32 index   = gizmo_element - GizmoElement_TRANSLATE_YZ;
            Plane plane = geometry.planes[index];
            b32 is_hit  = ray_plane_intersect(&camera_ray, plane);
            if (is_hit && (camera_ray.t < gizmo_max_t)) {
                V3 on_plane   = camera_ray.o + camera_ray.t*camera_ray.d;
                current_point = on_plane;
            }
        } else if (gizmo_element <= GizmoElement_ROTATE_C) {
            // The element is a torus/ring.
            s32 index  = gizmo_element - GizmoElement_ROTATE_X;
            V3  center = geometry.circles[index].center;
            f32 radius = geometry.circles[index].radius;
            V3  normal = geometry.circles[index].normal;
            
            Plane circle_plane = {center, normal};
            b32 is_hit         = ray_plane_intersect(&camera_ray, circle_plane);
            if (is_hit && (camera_ray.t < gizmo_max_t)) {
                V3 on_plane   = camera_ray.o + camera_ray.t*camera_ray.d;
                V3 on_circle  = center + radius*normalize(on_plane - center);
                current_point = on_circle;
            }
        }
    }
    
    //
    // Decide if gizmo_is_active or not.
    //
    if (is_close && key_pressed(Key_MLEFT)) {
        gizmo_is_active = TRUE;
        
        click_point  = current_point;
        click_offset = click_point - gizmo_origin;
        
        // Initially the same to avoid errors.
        previous_point = current_point;
    }
    if (!key_held(Key_MLEFT)) {
        gizmo_is_active = FALSE;
        gizmo_element   = is_close? gizmo_element : GizmoElement_NONE;
    }
    
    //
    // Calculate delta movement/rotation.
    //
    if (gizmo_is_active) {
        if (gizmo_mode == GizmoMode_TRANSLATION) {
            *delta_position_out = (current_point - gizmo_origin) - click_offset;
        } else if(gizmo_mode == GizmoMode_ROTATION) {
            s32 index = gizmo_element - GizmoElement_ROTATE_X;
            V3 c      = geometry.circles[index].center;
            V3 n      = geometry.circles[index].normal;
            
            V3 a = normalize(previous_point - c);
            V3 b = normalize(current_point - c);
            
            f32 cos   = dot(a, b);
            f32 sin   = dot(cross(a, b), n);
            f32 theta = _arctan2(sin, cos);
            
            *delta_rotation_out = quaternion_from_axis_angle(n, theta);
        }
        
        previous_point = current_point;
    }
}