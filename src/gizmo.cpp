/*

// @Note: This file is NOT independant. It's using the engine's types, collision routines and immediate-mode renderer.
    
// @Incomplete: Add scale gizmo + snapping!

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

struct Gizmo_Axis
{
    V3 origin;
    V3 direction; 
};

struct Gizmo_Plane
{
    V3 center;
    V3 normal;
};

struct Gizmo_Circle
{
    V3  center;
    V3  normal;
    f32 radius;
};

struct Gizmo_Geometry
{
    Gizmo_Axis   axes   [3];
    Gizmo_Plane  planes [3];
    Gizmo_Circle circles[4];
};

GLOBAL b32            gizmo_is_active; // Whether we are interacting with the gizmo. True when we find the best gizmo element candidate and press the left mouse button.
GLOBAL Gizmo_Element  gizmo_element;   // The current best candidate (could be none) based on distance and intersection with corresponding geometry of the element. 
GLOBAL Gizmo_Mode     gizmo_mode;
GLOBAL const f32      GIZMO_MAX_PICK_DISTANCE = 300.0f; // We don't pick things farther than this value from camera position.

GLOBAL Gizmo_Geometry         geometry;
GLOBAL Gizmo_Rendering_Params params;

// Snap increments.
GLOBAL const f32              TRANSLATION_SNAP = 0.1f;
GLOBAL const f32              ROTATION_SNAP    = 15.0f * DEGS_TO_RADS;
GLOBAL const f32              SCALE_SNAP       = 0.25f;

GLOBAL V3                     current_point;  // Intersection point of gizmo_element's geometry in current frame.
GLOBAL V3                     previous_point; // Intersection point of gizmo_element's geometry in previous frame.
GLOBAL V3                     click_origin;   // The origin of the gizmo in first frame where gizmo became active.
GLOBAL V3                     click_point;    // Intersection point of gizmo_element's geometry in first frame where gizmo became active.
GLOBAL V3                     click_offset;   // The vector from gizmo_origin to click_point in first frame where gizmo became active.

GLOBAL V4 gizmo_active_color = {1.0f, 1.0f, 1.0f, 1.0f};

FUNCTION void gizmo_clear()
{
    // Call this when you "un-pick" something.
    params   = {};
    geometry = {};
}

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
    
    geometry.axes[0]    = {gizmo_origin, V3_X_AXIS};
    geometry.axes[1]    = {gizmo_origin, V3_Y_AXIS};
    geometry.axes[2]    = {gizmo_origin, V3_Z_AXIS};
    
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
                Gizmo_Axis axis   = geometry.axes[i];
                Gizmo_Plane plane = geometry.planes[i];
                
                V4 axis_c   = (gizmo_element == GizmoElement_TRANSLATE_X + i)? gizmo_active_color : colors[GizmoElement_TRANSLATE_X + i];
                V4 plane_c  = (gizmo_element == GizmoElement_TRANSLATE_YZ + i)? gizmo_active_color : colors[GizmoElement_TRANSLATE_YZ + i];
                
                immediate_arrow(axis.origin, axis.direction, s, axis_c, th);
                immediate_rect_2point5d(plane.center, plane.normal, 0.5f*ps, plane_c);
            }
        } else {
            // Draw selected/active translation element.
            if (gizmo_element <= GizmoElement_TRANSLATE_Z) {
                s32 index       = gizmo_element - GizmoElement_TRANSLATE_X;
                Gizmo_Axis axis = geometry.axes[index];
                V3 p0           = click_origin - axis.direction * 2000.0f;
                V3 p1           = click_origin + axis.direction * 2000.0f;
                immediate_line(p0, p1, colors[gizmo_element], 0.01f);
            } else if (gizmo_element <= GizmoElement_TRANSLATE_XY) {
                // When interacting with a plane, draw the other two axes.
                s32 index       = gizmo_element - GizmoElement_TRANSLATE_YZ;
                Gizmo_Axis axis = geometry.axes[(index + 1) % 3];
                V3 p0           = click_origin - axis.direction * 2000.0f;
                V3 p1           = click_origin + axis.direction * 2000.0f;
                immediate_line(p0, p1, colors[(index + 1) % 3 + 1], 0.01f);
                axis = geometry.axes[(index + 2) % 3];
                p0   = click_origin - axis.direction * 2000.0f;
                p1   = click_origin + axis.direction * 2000.0f;
                immediate_line(p0, p1, colors[(index + 2) % 3 + 1], 0.01f);
            }
        }
    } else if (gizmo_mode == GizmoMode_ROTATION) {
        if (!gizmo_is_active) {
            // Draw all circles/rings.
            for (s32 i = 0; i < 4; i++) {
                Gizmo_Circle circle = geometry.circles[i];
                V4 circle_c         = (gizmo_element == GizmoElement_ROTATE_X + i)? gizmo_active_color : colors[GizmoElement_ROTATE_X + i];
                immediate_torus(circle.center, circle.radius, circle.normal, circle_c, th);
            }
        } else {
            // Draw selected/active circle.
            s32 index           = gizmo_element - GizmoElement_ROTATE_X;
            Gizmo_Circle circle = geometry.circles[index];
            immediate_line(circle.center, click_point, gizmo_active_color, params.thickness*0.20f);
            immediate_line(circle.center, current_point, gizmo_active_color, params.thickness*0.50f);
            immediate_torus(circle.center, circle.radius, circle.normal, colors[gizmo_element], params.thickness);
        }
    }
    
    immediate_end();
}

FUNCTION void gizmo_execute(V3 const   &camera_position, V3 const &gizmo_origin, 
                            V3         *delta_position_out, 
                            Quaternion *delta_rotation_out, 
                            V3         *delta_scale_out)
{
    *delta_position_out = {0.0f, 0.0f, 0.0f};
    *delta_rotation_out = quaternion_identity();
    *delta_scale_out    = {0.0f, 0.0f, 0.0f};
    
    gizmo_calculate_rendering_params(camera_position, gizmo_origin);
    gizmo_calculate_geometry(camera_position, gizmo_origin);
    
    V3 camera_end  = unproject(camera_position, 
                               GIZMO_MAX_PICK_DISTANCE, 
                               os->mouse_ndc, 
                               world_to_view_matrix, 
                               view_to_proj_matrix);
    f32 best_dist2 = F32_MAX;
    f32 best_t     = 1.0f; // Percent from camera_position to camera_end
    b32 is_close   = FALSE;
    
    //
    // If we're not interacting with the gizmo, find the "best" gizmo element candidate based on distance and intersection.
    //
    if (!gizmo_is_active) {
        if (gizmo_mode == GizmoMode_TRANSLATION) {
            // Axes.
            for (s32 i = 0; i < 3; i++) {
                Gizmo_Axis axis = geometry.axes[i];
                f32 t1, t2;
                f32 dist2 = closest_point_segment_segment(camera_position, camera_end, axis.origin, axis.origin + params.scale*axis.direction, &t1, NULL, &t2, NULL);
                if ((dist2 < best_dist2) && (dist2 < SQUARE(params.threshold))) {
                    best_dist2    = dist2;
                    is_close      = TRUE;
                    gizmo_element = (Gizmo_Element)(GizmoElement_TRANSLATE_X + i);
                }
            }
            
            // Planes.
            for (s32 i = 0; i < 3; i++) {
                Gizmo_Plane plane = geometry.planes[i];
                Hit_Result hit;
                if (segment_plane_intersect(camera_position, camera_end, plane.center, plane.normal, &hit)) {
                    V3 offset   = hit.impact_point - plane.center; 
                    b32 is_hit  = ((ABS(offset.x) <= 0.5f*params.plane_scale) &&
                                   (ABS(offset.y) <= 0.5f*params.plane_scale) &&
                                   (ABS(offset.z) <= 0.5f*params.plane_scale));
                    if (is_hit && (hit.percent < best_t)) {
                        best_t        = hit.percent;
                        is_close      = TRUE;
                        gizmo_element = (Gizmo_Element)(GizmoElement_TRANSLATE_YZ + i);
                    }
                }
            }
        } else if (gizmo_mode == GizmoMode_ROTATION) {
            for (s32 i = 0; i < 4; i++) {
                V3  center = geometry.circles[i].center;
                f32 radius = geometry.circles[i].radius;
                V3  normal = geometry.circles[i].normal;
                Hit_Result hit;
                if (segment_plane_intersect(camera_position, camera_end, center, normal, &hit)) {
                    V3 on_circle = center + radius*normalize(hit.impact_point - center);
                    f32 dist2    = length2(hit.impact_point - on_circle);
                    if ((dist2 < best_dist2) && (dist2 < SQUARE(params.threshold)) && (hit.percent < best_t)) {
                        best_dist2    = dist2;
                        best_t        = hit.percent;
                        is_close      = TRUE;
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
            s32 index       = gizmo_element - GizmoElement_TRANSLATE_X;
            Gizmo_Axis axis = geometry.axes[index];
            V3 plane_tangent = cross(axis.direction, click_origin - camera_position );
            V3 plane_normal  = cross(axis.direction, plane_tangent);
            Hit_Result hit;
            segment_plane_intersect(camera_position, camera_end, gizmo_origin, plane_normal, &hit);
            if (hit.result) {
                current_point = gizmo_origin + axis.direction * dot(hit.impact_point - gizmo_origin, axis.direction);
                current_point = round(current_point / TRANSLATION_SNAP) * TRANSLATION_SNAP;
            }
        } else if (gizmo_element <= GizmoElement_TRANSLATE_XY) {
            // The element is a plane.
            s32 index   = gizmo_element - GizmoElement_TRANSLATE_YZ;
            Gizmo_Plane plane = geometry.planes[index];
            Hit_Result hit;
            segment_plane_intersect(camera_position, camera_end, plane.center, plane.normal, &hit);
            if (hit.result) {
                current_point = hit.impact_point;
                current_point = round(current_point / TRANSLATION_SNAP) * TRANSLATION_SNAP;
            }
        } else if (gizmo_element <= GizmoElement_ROTATE_C) {
            // The element is a torus/ring.
            s32 index  = gizmo_element - GizmoElement_ROTATE_X;
            V3  center = geometry.circles[index].center;
            f32 radius = geometry.circles[index].radius;
            V3  normal = geometry.circles[index].normal;
            Gizmo_Plane circle_plane = {center, normal};
            Hit_Result hit;
            segment_plane_intersect(camera_position, camera_end, center, normal, &hit);
            if (hit.result) {
                V3 on_circle  = center + radius*normalize(hit.impact_point - center);
                current_point = on_circle;
                
                // Snap rotation as we move.
                if (gizmo_is_active) {
                    V3 to_click   = normalize(click_point - center);
                    V3 to_circle  = normalize(on_circle - center);
                    f32 cos       = dot(to_click, to_circle);
                    f32 sin       = dot(cross(to_click, to_circle), normal);
                    f32 theta     = _arctan2(sin, cos);
                    theta         = _round(theta / ROTATION_SNAP) * ROTATION_SNAP;
                    Quaternion q  = quaternion_from_axis_angle(normal, theta);
                    current_point = rotate_point_around_pivot(click_point, center, q);
                }
            }
        }
    }
    
    //
    // Decide if gizmo_is_active or not.
    //
    if (is_close && key_pressed(&os->tick_input, Key_MLEFT)) {
        gizmo_is_active = TRUE;
        
        click_origin = gizmo_origin;
        
        click_point  = current_point;
        click_offset = click_point - gizmo_origin;
        
        // Initially the same to avoid errors.
        previous_point = current_point;
    }
    if (!key_held(&os->tick_input, Key_MLEFT)) {
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
