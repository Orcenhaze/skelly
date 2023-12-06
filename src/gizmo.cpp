/*

// @Note: This file is NOT independant. It's using the engine's types, collision routines and immediate-mode renderer.
    
// @Cleanup: This needs some pest control!
// @Cleanup: This needs some pest control!
// @Cleanup: This needs some pest control!

// @Todo: Replace gizmo_old and new points!!

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
    // Different elements used depending on gizmo mode.
    GizmoElement_NONE,
    GizmoElement_XAXIS,
    GizmoElement_YAXIS,
    GizmoElement_ZAXIS,
    GizmoElement_XPLANE,
    GizmoElement_YPLANE,
    GizmoElement_ZPLANE,
    GizmoElement_CPLANE, // camera_ray plane.
};

struct Gizmo_Rendering_Params 
{
    // @Todo: Docs, explain what these are used for...
    // @Todo: Docs, explain what these are used for...
    //
    f32 scale;
    f32 threshold;
    f32 thickness;
    f32 plane_scale;
    V3  position_to_camera; // position of entity to camera.
};

// @Todo: Create a state struct?
// @Todo: Create a state struct?
// @Todo: Create a state struct?

struct Gizmo_Geometry
{
    Ray    axes   [3];
    Plane  planes [3];
    Circle circles[4];
};

// @Cleanup
// @Cleanup
// @Cleanup

GLOBAL b32            gizmo_is_active; // True iff close to an element and MouseButton_Left is down.
GLOBAL Gizmo_Mode     gizmo_mode;
GLOBAL Gizmo_Element  gizmo_element;

// @Todo: Replace.
GLOBAL V3 gizmo_new_point;
GLOBAL V3 gizmo_old_point;
GLOBAL V3 gizmo_start_point; // For visualizing rotations.

GLOBAL V3         original_position;
GLOBAL Quaternion original_orientation;
GLOBAL V3         original_scale;

GLOBAL V3                     click_offset;
GLOBAL Gizmo_Rendering_Params click_rendering_params; // From when we first clicked.
GLOBAL Gizmo_Geometry         click_geometry; // From when we first clicked.


GLOBAL V4 gizmo_active_color = {1.0f, 1.0f, 1.0f, 1.0f};
GLOBAL V4 gizmo_plane_color  = {1.0f, 1.0f, 0.0f, 0.5f};
GLOBAL V4 gizmo_axes_colors[3] =
{
    {1.0f, 0.0f, 0.0f, 0.8f}, // x-axis
    {0.0f, 1.0f, 0.0f, 0.8f}, // y-axis
    {0.0f, 0.0f, 1.0f, 0.8f}, // z-axis
};

FUNCTION Gizmo_Rendering_Params gizmo_calculate_rendering_params(V3 camera_position, V3 entity_position)
{
    V3 position_to_camera  = camera_position - entity_position;
    f32 distance_to_camera = length(position_to_camera);
    position_to_camera     = normalize0(position_to_camera);
    
    Gizmo_Rendering_Params result = {};
    
    result.scale              = distance_to_camera / 5.00f; // @Hardcode: scale.
    result.threshold          = result.scale       * 0.07f; // @Hardcode: threshold ratio.
    result.thickness          = result.scale       * 0.05f; // @Hardcode: thickness ratio.
    result.plane_scale        = result.scale       * 0.50f; // @Hardcode: plane_scale ratio.
    result.position_to_camera = position_to_camera;
    
    return result;
}

FUNCTION Gizmo_Geometry gizmo_calculate_geometry(V3 entity_position, Gizmo_Rendering_Params params)
{
    Gizmo_Geometry result = {};
    
    result.axes[0] = {entity_position, {1.0f, 0.0f, 0.0f}, F32_MAX};
    result.axes[1] = {entity_position, {0.0f, 1.0f, 0.0f}, F32_MAX};
    result.axes[2] = {entity_position, {0.0f, 0.0f, 1.0f}, F32_MAX};
    
    f32 plane_scale = params.plane_scale;
    V3 sx = {0.0f, plane_scale, plane_scale};
    V3 sy = {plane_scale, 0.0f, plane_scale};
    V3 sz = {plane_scale, plane_scale, 0.0f};
    
    result.planes[0] = {entity_position + sx, {1.0f, 0.0f, 0.0f}};
    result.planes[1] = {entity_position + sy, {0.0f, 1.0f, 0.0f}};
    result.planes[2] = {entity_position + sz, {0.0f, 0.0f, 1.0f}};
    
    f32 scale = params.scale;
    result.circles[0] = {entity_position,        {1.0f, 0.0f, 0.0f}, scale+(scale*0.10f)};
    result.circles[1] = {entity_position,        {0.0f, 1.0f, 0.0f}, scale+(scale*0.05f)};
    result.circles[2] = {entity_position,        {0.0f, 0.0f, 1.0f}, scale+(scale*0.00f)};
    result.circles[3] = {entity_position, params.position_to_camera, scale+(scale*0.30f)};
    
    return result;
}

FUNCTION void gizmo_render_translation(Gizmo_Geometry geometry, Gizmo_Rendering_Params params)
{
    // Ignore z-buffer.
    d3d11_clear_depth();
    immediate_begin();
    set_texture(0);
    
    // Axes
    for (s32 i = 0; i < 3; i++) {
        Ray axis = geometry.axes[i];
        V4 color = gizmo_axes_colors[i];
        if ((gizmo_element - GizmoElement_XAXIS) == i)
            color = gizmo_active_color;
        
        immediate_arrow(axis.o, axis.d, params.scale, color, params.thickness);
    }
    
    // Planes
    for (s32 i = 0; i < 3; i++) {
        Plane plane = geometry.planes[i];
        V4 color = gizmo_plane_color;
        if ((gizmo_element - GizmoElement_XPLANE) == i)
            color = gizmo_active_color;
        
        immediate_rect_3d(plane.center, plane.normal, 0.5f*params.plane_scale, color);
    }
    
    immediate_end();
}

FUNCTION void gizmo_render_rotation(Gizmo_Geometry geometry, Gizmo_Rendering_Params params)
{
    // Ignore z-buffer.
    d3d11_clear_depth();
    immediate_begin();
    set_texture(0);
    
    if(gizmo_is_active)
    {
        // Assuming they all have the same center.
        immediate_line_3d(geometry.circles[0].center, gizmo_new_point, gizmo_active_color, params.thickness*0.50f);
        immediate_line_3d(geometry.circles[0].center, gizmo_start_point, gizmo_active_color, params.thickness*0.20f);
    }
    
    for (s32 i = 0; i < 4; i++) {
        Circle circle = geometry.circles[i];
        
        V4 color;
        if (i == 3) color = gizmo_plane_color;
        else       color  = gizmo_axes_colors[i];
        if ((gizmo_element - GizmoElement_XPLANE) == i)
            color = gizmo_active_color;
        
        immediate_torus(circle.center, circle.radius, circle.normal, color, params.thickness);
    }
    
    immediate_end();
}

FUNCTION void gizmo_render(Ray camera_ray, Entity *selected_entity)
{
    V3 pos_to_cam   = camera_ray.o - selected_entity->position;
    f32 cam_dist    = length(pos_to_cam);
    
    V3 pos = selected_entity->position;
    Gizmo_Rendering_Params params = gizmo_calculate_rendering_params(camera_ray.origin, pos);
    Gizmo_Geometry geometry       = gizmo_calculate_geometry(pos, params);
    
    // @Cleanup: Cleanup inactive stuff...
    // @Cleanup:
    // @Cleanup:
    
    if(gizmo_mode == GizmoMode_TRANSLATION) {
        if (!gizmo_is_active) {
            gizmo_render_translation(geometry, params);
        } else {
            V4 ghost_color = {0.2f, 0.2f, 0.2f, 0.4f};
            d3d11_clear_depth();
            immediate_begin();
            set_texture(0);
            immediate_cube(original_position, 0.05f, ghost_color);
            
            if (gizmo_element < GizmoElement_XPLANE) {
                s32 axis_index = gizmo_element - GizmoElement_XAXIS;
                Ray axis       = click_geometry.axes[axis_index];
                
                immediate_arrow(axis.o, axis.d, click_rendering_params.scale, ghost_color, click_rendering_params.thickness);
                
                immediate_line_3d(original_position + -50000*axis.d, 
                                  original_position +  50000*axis.d, 
                                  gizmo_axes_colors[axis_index], 0.01f);
            } else {
                s32 plane_index = gizmo_element - GizmoElement_XPLANE;
                Plane plane     = click_geometry.planes[plane_index];
                immediate_rect_3d(plane.center, plane.normal, 0.5f*click_rendering_params.plane_scale, ghost_color);
                
                s32 tangent_index   = (plane_index + 1) % 3;
                s32 bitangent_index = (plane_index + 2) % 3;
                
                Ray axis0 = click_geometry.axes[tangent_index];
                Ray axis1 = click_geometry.axes[bitangent_index];
                
                immediate_line_3d(original_position + -50000*axis0.d, 
                                  original_position +  50000*axis0.d, 
                                  gizmo_axes_colors[tangent_index], 0.01f);
                immediate_line_3d(original_position + -50000*axis1.d, 
                                  original_position +  50000*axis1.d, 
                                  gizmo_axes_colors[bitangent_index], 0.01f);
            }
            immediate_end();
        }
        
        
    } else if(gizmo_mode == GizmoMode_ROTATION) {
        gizmo_render_rotation(geometry, params);
    }
}

FUNCTION void gizmo_execute(Ray camera_ray, Entity *selected_entity)
{
    f32 best_dist = F32_MAX;
    f32 best_t    = F32_MAX;
    b32 is_close  = false;
    
    V3 pos = selected_entity->position;
    Gizmo_Rendering_Params params = gizmo_calculate_rendering_params(camera_ray.origin, pos);
    Gizmo_Geometry geometry       = gizmo_calculate_geometry(pos, params);
    
    //
    // Choose a gizmo element based on distance or intersection.
    //
    if (!gizmo_is_active) {
        if (gizmo_mode == GizmoMode_TRANSLATION) {
            // Axes.
            for (s32 i = 0; i < 3; i++) {
                Ray *axis   = &geometry.axes[i];
                f32 dist    = closest_distance_ray_ray(&camera_ray, axis);
                b32 t_valid = (axis->t > 0) && (axis->t < params.scale);
                
                if ((dist < best_dist) && (dist < params.threshold) && (t_valid)) {
                    best_dist = dist;
                    
                    is_close      = true;
                    gizmo_element = (Gizmo_Element)(GizmoElement_XAXIS + i);
                }
            }
            
            // Planes.
            for (s32 i = 0; i < 3; i++) {
                Plane plane = geometry.planes[i];
                if (ray_plane_intersect(&camera_ray, plane)) {
                    V3 on_plane        = camera_ray.o + camera_ray.t*camera_ray.d;
                    Rect3 plane_bounds = get_plane_bounds(plane.center, plane.normal, params.plane_scale);
                    
                    // Only compare the dimensions of the plane. So guarantee equality for 
                    // the other dimension to have a proper point_inside_box_test().
                    on_plane.I[i]      = plane_bounds.min.I[i];
                    
                    if (point_inside_box_test(on_plane, plane_bounds) && (camera_ray.t < best_t)) {
                        best_t = camera_ray.t;
                        
                        is_close      = true;
                        gizmo_element = (Gizmo_Element)(GizmoElement_XPLANE + i);
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
                        best_dist = dist;
                        best_t    = camera_ray.t;
                        
                        is_close      = true;
                        gizmo_element = (Gizmo_Element)(GizmoElement_XPLANE + i);
                    }
                }
            }
        }
    }
    
    
    
    //
    // Decide if gizmo_is_active or not.
    //
    if (is_close && key_held(Key_MLEFT)) {
        gizmo_is_active   = TRUE;
        
        original_position    = selected_entity->position;
        original_orientation = selected_entity->orientation;
        original_scale       = selected_entity->scale;
        
        click_rendering_params = params;
        click_geometry         = geometry;
        
        // @Cleanup:
        // @Cleanup:
        // @Cleanup:
        // Calculate click_offset
        if (gizmo_element < GizmoElement_XPLANE) {
            // Axes.
            s32 axis_index  = gizmo_element - GizmoElement_XAXIS;
            Ray *axis       = &geometry.axes[axis_index];
            
            // We'll manipulate using a plane. The active axis lies on that plane. We want its normal.
            V3 ignored      = {};
            V3 plane_normal = {};
            calculate_tangents(axis->d, &plane_normal, &ignored);
            Plane plane = {original_position, plane_normal};
            
            if (ray_plane_intersect(&camera_ray, plane)) {
                V3 on_plane = camera_ray.o + camera_ray.t*camera_ray.d;
                //on_plane.I[idx] = plane.center.I[idx];
                
                V3 v = on_plane - original_position;
                click_offset = dot(v, axis->d) * axis->d;
            }
        } else {
            // Planes.
            s32 plane_index = gizmo_element - GizmoElement_XPLANE;
            Plane plane     = geometry.planes[plane_index];
            
            if (ray_plane_intersect(&camera_ray, plane)) {
                V3 on_plane = camera_ray.o + camera_ray.t*camera_ray.d;
                
                V3 v = on_plane - original_position;
                click_offset = v;
            }
        }
    }
    if (!key_held(Key_MLEFT)) {
        gizmo_is_active = FALSE;
        gizmo_element   = is_close? gizmo_element : GizmoElement_NONE;
        
        gizmo_old_point = {};
        gizmo_new_point = {};
    }
    
    
    
    //
    // Process gizmo manipulation.
    //
    if (gizmo_is_active) {
        if (gizmo_mode == GizmoMode_TRANSLATION) {
            V3 delta = {};
            
            // @Todo: Debug values on edges. We are getting weird snapping!
            //
            if (gizmo_element < GizmoElement_XPLANE) {
                // Axes.
                s32 axis_index  = gizmo_element - GizmoElement_XAXIS;
                Ray *axis       = &geometry.axes[axis_index];
                
                // We'll manipulate using a plane. The active axis lies on that plane. We want its normal.
                V3 ignored      = {};
                V3 plane_normal = {};
                calculate_tangents(axis->d, &plane_normal, &ignored);
                Plane plane = {original_position, plane_normal};
                
                if (ray_plane_intersect(&camera_ray, plane)) {
                    V3 on_plane = camera_ray.o + camera_ray.t*camera_ray.d;
                    //on_plane.I[idx] = plane.center.I[idx];
                    
                    V3 v = on_plane - original_position;
                    selected_entity->position = original_position + dot(v, axis->d) * axis->d;
                    selected_entity->position -= click_offset;
                    
                    debug_print("t: %f\n", camera_ray.t);
                }
                
            } else {
                // Planes.
                s32 plane_index = gizmo_element - GizmoElement_XPLANE;
                Plane plane     = geometry.planes[plane_index];
                
                if (ray_plane_intersect(&camera_ray, plane)) {
                    V3 on_plane = camera_ray.o + camera_ray.t*camera_ray.d;
                    
                    V3 v = on_plane - original_position;
                    selected_entity->position = original_position + v;
                    selected_entity->position -= click_offset;
                }
            }
            
        } else if(gizmo_mode == GizmoMode_ROTATION) {
            s32 plane_index = gizmo_element - GizmoElement_XPLANE;
            V3  c           = geometry.circles[plane_index].center;
            f32 r           = geometry.circles[plane_index].radius;
            V3  n           = geometry.circles[plane_index].normal;
            
            Plane circle_plane = {c, n};
            
            if (ray_plane_intersect(&camera_ray, circle_plane)) {
                V3 on_plane  = camera_ray.o + camera_ray.d*camera_ray.t;
                gizmo_new_point   = c + r*normalize(on_plane - c);
            }
            
            // The first frame we start interacting with a rotation gizmo.
            if (length2(gizmo_old_point) == 0) 
                gizmo_start_point = gizmo_new_point;
            
            if (length2(gizmo_old_point) != 0) {
                V3 a = normalize(gizmo_old_point - c);
                V3 b = normalize(gizmo_new_point - c);
                
                f32 cos   = dot(a, b);
                f32 sin   = dot(cross(a, b), n);
                f32 theta = _arctan2(sin, cos);
                
                Quaternion q = quaternion_from_axis_angle(n, theta);
                
                selected_entity->orientation = q * selected_entity->orientation;
            }
            
            gizmo_old_point = gizmo_new_point;
        }
        
    }
}