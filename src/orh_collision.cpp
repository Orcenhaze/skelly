/* orh_collision.cpp - v0.00 - C++ collision routines.

REVISION HISTORY:

NOTE:
 * Things you pass to collision functions are assumed to be in the same "space" or "coordinate system",
and so all the Hit_Result info and distances that will be returned will be in said coordinate system.

 Let's say you did the tests in some mesh object-space. The distances returned by our routines won't
 always represent the distances in world-space because you may have applied some scaling when transforming
points and directions from world-space to object-space. Solution:
- If you know the scale you applied is uniform, you can simply multiply the returned distances by the
 scale factor to convert it back to world-space distance. 
- For non-uniform scaling it's trickier; probably the best way is to transform the intersection point 
back to world-space and then re-measure the distance in world-space.

This is a general explanation, of course... but really, you usually have a "center" point somewhere
in the "original" space before you transformed it to the desired space. Well, our functions return
 intersection points in desired space, so you can just transform those points back to original space
 and re-measure the distance using the original "center" point.

* Variables in implementation look cryptic (I know it sucks), but the algorithms involve many operations
 and having descriptive names for stuff would be kinda "chaotic". So, I _try_ to explain what each 
function does if it isn't obvious AND also refer to resources that explain things better.

TODO:
[] Traces for lines, boxes, spheres, and capsules.

*/

#include "orh.h"

union Ray
{
    struct { V3  origin; V3 direction; f32 t; };
    struct { V3  o;      V3 d;         f32 t; };
    
};


////////////////////////////////
//~ 3D

FUNCTION f32 closest_point_line_point(V3 const &c, V3 const &a, V3 const &b,
                                      f32 *t_out = NULL, V3 *p_out = NULL)
{
    // @Note: Christer Ericson - Real Time Collision Detection - P.127;
    //
    // Returns _squared_ closest distance between point c and line ab.
    //
    // Just project c onto ab.
    
    V3  ab   = b - a;
    f32 len2 = dot(ab, ab);
    
    f32 t;
    V3  p;
    if (nearly_zero(len2)) {
        // If line degenerates into point.
        t = 0.0f;
        p = a;
    } else {
        t = dot(c - a, ab) / dot(ab, ab);
        p = a + t * ab;
    }
    
    if (t_out) *t_out = t;
    if (p_out) *p_out = p;
    f32 result = length2(c - p);
    return result;
}

FUNCTION f32 closest_point_segment_point(V3 const &c, V3 const &a, V3 const &b,
                                         f32 *t_out = NULL, V3 *p_out = NULL)
{
    // @Note: Christer Ericson - Real Time Collision Detection - P.127;
    //
    // Returns _squared_ closest distance between point c and line segment ab.
    //
    // Just project c onto ab, then clamp the t value to end points of the line segment.
    
    V3  ab   = b - a;
    f32 len2 = dot(ab, ab);
    
    f32 t;
    V3  p;
    if (nearly_zero(len2)) {
        // If segment degenerates into point.
        t = 0.0f;
        p = a;
    } else {
        t = dot(c - a, ab) / dot(ab, ab);
        t = CLAMP01(t);
        p = a + t * ab;
    }
    
    if (t_out) *t_out = t;
    if (p_out) *p_out = p;
    f32 result = length2(c - p);
    return result;
}

FUNCTION f32 closest_point_line_line(V3 const &a1, V3 const &b1, V3 const &a2, V3 const &b2,
                                     f32 *t1_out = NULL, V3 *p1_out = NULL, 
                                     f32 *t2_out = NULL, V3 *p2_out = NULL)
{
    // @Note: Christer Ericson - Real Time Collision Detection - P.146;
    //
    // Returns _squared_ closest distance between the two lines.
    //
    // If the lines are not parallel, we need to find two points (p1 and p2) on ab1 and ab2, such that
    // (p1 - p2) is perpendicular to both lines.
    //
    // If the lines are parallel, then the distance between them is constant.
    // We can simply project any point on ab1 onto ab2. For this, we use closest_point_line_point().
    
    V3 d1 = b1 - a1; // ab1 direction vector.
    V3 d2 = b2 - a2; // ab2 direction vector.
    
    f32 a = dot(d1, d1);
    f32 e = dot(d2, d2);
    f32 b = dot(d1, d2);
    f32 d = (a*e) - (b*b); // This should always be >= 0.0f. But FP errors...
    
    f32 t1, t2;
    V3  p1, p2;
    
    // Check if lines degenerate into points.
    b32 line1_is_point = nearly_zero(a);
    b32 line2_is_point = nearly_zero(e);
    if (line1_is_point && line2_is_point) {
        t1 = t2 = 0.0f;
        p1 = a1;
        p2 = a2;
    } else if (line1_is_point) {
        t1 = 0.0f;
        p1 = a1;
        closest_point_line_point(p1, a2, b2, &t2, &p2);
    } else if (line2_is_point) {
        t2 = 0.0f;
        p2 = a2;
        closest_point_line_point(p2, a1, b1, &t1, &p1);
    } else {
        // Nondegenerate cases.
        if (nearly_zero(d)) {
            // Lines are parallel.
            t1 = 0.0f;
            p1 = a1;
            closest_point_line_point(p1, a2, b2, &t2, &p2);
        } else {
            // Common case.
            V3 r  = a1 - a2;
            f32 c = dot(d1, r);
            f32 f = dot(d2, r);
            
            t1 = ((b*f) - (c*e)) / d;
            t2 = ((a*f) - (b*c)) / d;
            p1 = a1 + t1 * d1;
            p2 = a2 + t2 * d2;
        }
    }
    
    if (t1_out) *t1_out = t1;
    if (t2_out) *t2_out = t2;
    if (p1_out) *p1_out = p1;
    if (p2_out) *p2_out = p2;
    f32 result = length2(p1 - p2);
    return result;
}

FUNCTION f32 closest_point_segment_segment(V3 const &a1, V3 const &b1, V3 const &a2, V3 const &b2,
                                           f32 *t1_out = NULL, V3 *p1_out = NULL, 
                                           f32 *t2_out = NULL, V3 *p2_out = NULL)
{
    // @Note: Christer Ericson - Real Time Collision Detection - P.148;
    //
    // Returns _squared_ closest distance between the two line segments.
    //
    // L1 is line of ab1 --- S1 is segment of ab1.
    //
    // We first treat the segments as lines and try to find closest points of those lines. 
    // If both closest points of the lines lie on the segments, then the same method for
    // closest_point_line_line() applies.
    //
    // If one of the closest points (of line) lies outside of the segment, then we clamp it to the appropriate
    // endpoint, then do closest_point_segment_point(endpoint, a, b, 0, &on_segment).
    // Your closest points are then "endpoint" and "on_segment".
    //
    // If both closest points (of lines) lie outside of the segments. We do the clamping process twice:
    // Clamp closest point of line1 to endpoint1 and do closest_point_line_point(endpoint1, a2, b2, 0, &on_line2).
    // Then clamp on_line2 to endpoint2 and do closest_point_segment_point(endpoint2, a1, b1, 0, &on_segment1).
    // Your closest points are then "on_segment1" and "endpoint2".
    //
    // This is the conceptual explanation, we won't be calling those functions (although we could).
    
    V3 d1 = b1 - a1; // ab1 direction vector.
    V3 d2 = b2 - a2; // ab2 direction vector.
    V3 r  = a1 - a2;
    
    f32 a = dot(d1, d1); // Squared length of segment1, always nonnegative.
    f32 e = dot(d2, d2); // Squared length of segment2, always nonnegative.
    f32 f = dot(d2, r);
    
    f32 t1, t2;
    V3  p1, p2;
    
    // Check if segments degenerate into points.
    b32 seg1_is_point = nearly_zero(a);
    b32 seg2_is_point = nearly_zero(e);
    if (seg1_is_point && seg2_is_point) {
        t1 = t2 = 0.0f;
        p1 = a1;
        p2 = a2;
    } else if (seg1_is_point) {
        t1 = 0.0f;
        p1 = a1;
        t2 = f/e;
        t2 = CLAMP01(t2);
        p2 = a2 + t2 * d2;
    } else {
        f32 c = dot(d1, r);
        if (seg2_is_point) {
            t2 = 0.0f;
            p2 = a2;
            t1 = -c/a;
            t1 = CLAMP01(t1);
            p1 = a1 + t1 * d1;
        } else {
            // Nondegenerate case.
            f32 b = dot(d1, d2);
            f32 d = (a*e) - (b*b); // denom: should always be >= 0.0f. But FP errors...
            
            if (nearly_zero(d)) {
                // Segments are parallel - pick arbitrary t1.
                t1 = 0.0f;
            } else {
                // Otherwise, compute closest point on L1 to L2 and clamp to S1 endpoint.
                t1 = ((b*f) - (c*e)) / d;
                t1 = CLAMP01(t1);
            }
            
            // t2 is equal to "((b*t1) + f) / e"; 
            // But defer division by e until we know t2 is in range [0, 1], i.e, it lies on S2.
            f32 t2nom = (b*t1) + f;
            if (t2nom < 0.0f) {
                // t2 is outside S2 (behind a2).
                t2 = 0.0f;
                t1 = -c/a;
                t1 = CLAMP01(t1);
            } else if (t2nom > e) {
                // t2 is outside S2 (after b2).
                t2 = 1.0f;
                t1 = (b - c) / a;
                t1 = CLAMP01(t1);
            } else {
                // t2 lies on S2.
                t2 = t2nom / e;
            }
            
            p1 = a1 + t1 * d1;
            p2 = a2 + t2 * d2;
        }
    }
    
    if (t1_out) *t1_out = t1;
    if (t2_out) *t2_out = t2;
    if (p1_out) *p1_out = p1;
    if (p2_out) *p2_out = p2;
    f32 result = length2(p1 - p2);
    return result;
}

struct Plane
{
    V3 center;
    V3 normal;
};

struct Circle
{
    V3  center;
    V3  normal;
    f32 radius;
};

FUNCTION b32 point_inside_box_test(V3 p, Rect3 box)
{
    b32 result = false;
    
    if((p.x >= box.min.x) && (p.x <= box.max.x) &&
       (p.y >= box.min.y) && (p.y <= box.max.y) &&
       (p.z >= box.min.z) && (p.z <= box.max.z))
        result = true;
    
    return result;
}

FUNCTION b32 box_overlap_test(Rect3 A, Rect3 B)
{
    b32 result = ((A.max.x > B.min.x) &&
                  (A.min.x < B.max.x) && 
                  (A.max.y > B.min.y) &&
                  (A.min.y < B.max.y) &&
                  (A.max.z > B.min.z) &&
                  (A.min.z < B.max.z));
    
    return result;
}

FUNCTION b32 ray_box_intersect(Ray *ray, Rect3 box)
{
    V3 t_bmin = hadamard_div((box.min - ray->o), ray->d);
    V3 t_bmax = hadamard_div((box.max - ray->o), ray->d);
    
    V3 t_min3 = { MIN(t_bmin.x, t_bmax.x), MIN(t_bmin.y, t_bmax.y), MIN(t_bmin.z, t_bmax.z) };
    V3 t_max3 = { MAX(t_bmin.x, t_bmax.x), MAX(t_bmin.y, t_bmax.y), MAX(t_bmin.z, t_bmax.z) };
    
    f32 t_min = MAX3(t_min3.x, t_min3.y, t_min3.z);
    f32 t_max = MIN3(t_max3.x, t_max3.y, t_max3.z);
    
    // @Note: Ignore collisions that are behind us, i.e. (t_min < 0.0f).
    b32 result = ((t_min > 0.0f) && (t_min < t_max));
    
    if (result)
        ray->t = t_min;
    else
        ray->t = F32_MAX;
    
    return result;
}

FUNCTION b32 ray_plane_intersect(Ray *ray, Plane plane)
{
    f32 d      = dot(plane.normal, plane.center);
    f32 t      = (d - dot(plane.normal, ray->o)) / dot(plane.normal, ray->d);
    b32 result = (t > 0.0f) && (t < F32_MAX);
    
    if (result)
        ray->t = t;
    else
        ray->t = F32_MAX;
    
    return result;
}

FUNCTION b32 ray_triangle_intersect(Ray *ray, V3 p0, V3 p1, V3 p2)
{
    ray->t = F32_MAX;
    
    V3 e1 = p1 - p0, e2 = p2 - p0, h = cross(ray->direction, e2);
    f32 a = dot(e1, h);
    if (ABS(a) == 0) return FALSE;
    
    f32 f = 1 / a;
    V3  s = ray->origin - p0;
    f32 u = f * dot(s, h);
    if (u < 0 || u > 1) return FALSE;
    
    V3  q = cross(s, e1);
    f32 v = f * dot(ray->direction, q);
    if (v < 0 || u + v > 1) return FALSE;
    
    f32 t = f * dot(e2, q);
    if (t < 0) return FALSE;
    
    ray->t = t;
    return TRUE;
}

FUNCTION b32 ray_mesh_intersect(Ray *ray, s64 num_vertices, V3 *vertices, s64 num_indices, u32 *indices)
{
    ASSERT((num_indices % 3) == 0);
    
    f32 best_t = F32_MAX;
    Ray temp_r = *ray;
    
    for (s32 i = 0; i < num_indices; i += 3) {
        V3 p0 = vertices[indices[i + 0]];
        V3 p1 = vertices[indices[i + 1]];
        V3 p2 = vertices[indices[i + 2]];
        
        b32 is_hit = ray_triangle_intersect(&temp_r, p0, p1, p2);
        if (is_hit && (temp_r.t < best_t)) {
            best_t = temp_r.t;
        }
    }
    
    ray->t = best_t;
    return (best_t != F32_MAX);
}

FUNCTION f32 closest_distance_ray_ray(Ray *r0, Ray *r1)
{
    // @Note: Ray directions must be normalized!
    
    // @Note: Originally looked at code from https://nelari.us/post/gizmos/,
    // but it seemingly had a mistake when returning a result, so I corrected the math using
    // Christer_Ericson_Real-Time_Collision_Detection Ch.5.1.8 Page.185.
    //
    // Basically, if the rays are parallel, then the distance between them is constant.
    // We compute it by getting the length of the vector v going from some point p0 on r0 to 
    // a point facing it p1 on r1. 
    // A faster way is to use the parallelogram which is formed by the vector v from r0's and
    // r1's origin and r0's  direction vector. But since the ray direction is normalized,
    // we can simplify further.
    // Check method #3 from: https://www.youtube.com/watch?v=9wznbg_aKOo
    //
    // If they are not parallel, we need to find 2 points (on r0 and r1), such that v is
    // perpendicular to both rays.
    
    f32 result  = F32_MAX;
    
    //r0->d = normalize(r0->d);
    //r1->d = normalize(r1->d);
    
    V3 vec_o  = r0->o - r1->o;
    f32 dot_d = dot(r0->d, r1->d);
    f32 det   = 1.0f - SQUARE(dot_d);
    
    if (ABS(det) > F32_MIN) {
        //
        // Rays are non-parallel.
        //
        f32 inv_det = 1.0f / det;
        
        f32 c = dot(vec_o, r0->d);
        f32 f = dot(vec_o, r1->d);
        
        r0->t = (dot_d*f - c)       * inv_det;
        r1->t = (f       - c*dot_d) * inv_det;
        
        V3 p0 = r0->o + r0->d*r0->t;
        V3 p1 = r1->o + r1->d*r1->t;
        
        result = length(p0 - p1);
    } else {
        //
        // Rays are parallel.
        //
        result = length(cross(vec_o, r0->d));
    }
    
    return result;
}