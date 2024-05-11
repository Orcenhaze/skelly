/* orh_collision.cpp - v0.00 - C++ collision routines.

REVISION HISTORY:

NOTE:
 * Things you pass to collision functions are assumed to be in the same "space" or "coordinate system",
and so all the Hit_Result info and distances returned will be in said coordinate system.

Let's give this coordinate system a name: the "collision-space".

This collision-space could simply be the world space OR it could be the mesh object-space OR whatever.

An important thing to note is that most of the time, the user cares about values in world-space.
But let's say you did the tests in some mesh object-space, i.e., the "collision-space" here is the mesh 
object-space, the distances returned by our routines WON'T represent the distances in world-space 
 because you may have applied some scaling when transforming points and directions from world-space to
 object-space. Solution:
- If you know the scale you applied is uniform, you can simply multiply the returned distances by the
 scale factor to convert it back to world-space distance. Then re-measure points using new distance.
- For non-uniform scaling it's trickier; probably the best way is to transform the intersection point 
back to world-space and then re-measure the distance in world-space.

 Maybe we could implement higher-level functions that just lets user pass everything in world space and
do things automatically for him.

* The second collision shape we pass to functions (the one on the right by convention), is the shape we
 test _against_.
 Both shapes, as we said before, must be in collision-space and if necessary (not for all tests) we
transform the FIRST shape from collision-space to local-space of SECOND shape.

* Variables in implementation look cryptic (I know it sucks), but the algorithms involve many operations
 and having descriptive names for stuff would be kinda "chaotic". So, I _try_ to explain what each 
function does if it isn't obvious AND also refer to resources that explain things better.

TODO:
[] GJK for convex-hulls.
[] Traces for lines, boxes, spheres, and capsules. Note that for traces, there's this concept of being
 _already_ in collision before trace starts where you can return early.
[] Make Hit_Result ALWAYS return things in world-space (kinda tricky atm).
[] 2D collision stuff.

*/

#include "orh.h"

////////////////////////////////
//~ 3D

FUNCTION b32 aabb_overlap(V3 const &a_min, V3 const &a_max, V3 const &b_min, V3 const &b_max)
{
    // @Note: Returns whether a and b overlap.
    b32 result = ((a_max.x > b_min.x) &&
                  (a_min.x < b_max.x) && 
                  (a_max.y > b_min.y) &&
                  (a_min.y < b_max.y) &&
                  (a_max.z > b_min.z) &&
                  (a_min.z < b_max.z));
    
    return result;
}

FUNCTION b32 point_inside_aabb(V3 const &p, V3 const &min, V3 const &max)
{
    // @Note: Returns whether point p is inside aabb defined by min and max.
    // It's for AABBs, so pass stuff in world-space, I guess.
    b32 result = FALSE;
    if((p.x >= min.x) && (p.x <= max.x) &&
       (p.y >= min.y) && (p.y <= max.y) &&
       (p.z >= min.z) && (p.z <= max.z))
        result = TRUE;
    return result;
}

FUNCTION b32 segment_aabb_intersect(V3 const &a, V3 const &b, V3 const &min, V3 const &max)
{
    // @Note:
    //
    // Returns whether line segment defined by a and b intersect the axis-aligned box defined by 
    // min and max.
    //
    // It's for AABBs, so pass stuff in world-space, I guess.
    
    V3 d = b-a;
    
    V3 t_bmin = hadamard_div((min - a), d);
    V3 t_bmax = hadamard_div((max - a), d);
    
    V3 t_min3 = { MIN(t_bmin.x, t_bmax.x), MIN(t_bmin.y, t_bmax.y), MIN(t_bmin.z, t_bmax.z) };
    V3 t_max3 = { MAX(t_bmin.x, t_bmax.x), MAX(t_bmin.y, t_bmax.y), MAX(t_bmin.z, t_bmax.z) };
    
    f32 t_min = MAX3(t_min3.x, t_min3.y, t_min3.z);
    f32 t_max = MIN3(t_max3.x, t_max3.y, t_max3.z);
    
    b32 result = ((t_min > 0.0f) && (t_min < t_max));
    return result;
}

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
    // Also fills t values as percents in range [0, 1] and fills closest points.
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

struct Hit_Result
{
    // @Note: Currently, all the values here are in collision-space, which is NOT world-space necessarily.
    //
    // Be AWARE when moving the shapes from world-space to collision-space using any type of scaling, 
    // uniform or non-uniform. If you did, then certain values like penetration_depth won't be correct
    // in world space as it needs to be scaled accordingly.
    //
    // The result of length(impact_point - start) will be different in collision-space compared to
    // the space you care about (like world) because scaling was applied. It is @important to note
    // however, that because we are using a "percent" instead of a "distance from start along start-end"
    // then that value doesn't change, because the relationship doesn't change even after scaling.
    // You can use transform_hit_result() to transform the important values using some affine matrix.
    
    // For traces and line segment functions only. Zero vectors otherwise.
    V3 start;
    V3 end;
    
    // The point on the surface of the second shape (the one we are testing against) on which the collision occured.
    // If no collision occured, this will be a zero vector.
    V3 impact_point;
    
    // The normal vector of the surface of the second shape (the one we are testing against) on which the collision occured.
    // If no collision occured, this will be a zero vector.
    V3 impact_normal;
    
    // normal = normalize(sphere.center - impact_point);
    // OR
    // normal = normalize(closest_on_capsule_axis - impact_point);
    // We move along this vector by "penetration_depth" amount in order to "resolve" the collision.
    // It is calculated for spheres and capsules only, otherwise it will be the same as impact_normal.
    V3 normal;
    
    // location = shape1.center + normal * penetration_depth;
    // This is the "resolved" center of the "first" collision shape.
    // When using tracing, it's the center of the traced shape when collision occured.
    // Equivalent to impact_point when using line traces and segments tests.
    V3 location;
    
    // This is a percent value in range [0, 1].
    // It is used for traces and line segment intersection functions and it indicates the percentage from
    // start to end points we moved until collision occured.
    // You can use it to lerp between start and end points to get the impact_point essentially.
    // When the value is 1, it _most likey_ means there was no collision and we were able to traverse
    // 100% of the trace/segment direction.
    f32 percent;
    
    //f32 distance;
    
    // True if we intersect, false otherwise.
    u8 result;
};

FUNCTION inline Hit_Result make_hit_result(V3 const &start, V3 const &end)
{
    Hit_Result result = {};
    result.start   = start;
    result.end     = end;
    result.percent = 1.0f; // Assume there was no collision.
    return result;
}

FUNCTION inline Hit_Result transform_hit_result(M4x4 const &affine, Hit_Result const &hit)
{
    Hit_Result result    = make_hit_result (hit.start, hit.end);
    result.start         = transform_point (affine, hit.start);
    result.end           = transform_point (affine, hit.end);
    result.impact_point  = transform_point (affine, hit.impact_point);
    result.impact_normal = transform_vector(affine, hit.impact_normal);
    result.normal        = transform_vector(affine, hit.normal);
    result.location      = transform_point (affine, hit.location);
    result.percent       = hit.percent;
    result.result        = hit.result;
    return result;
    
    // @Note: These will be different if matrix contains scaling.
    //f32 len1 = length(hit.impact_point - hit.start);
    //f32 len2 = length(result.impact_point - result.start);
}

FUNCTION b32 segment_triangle_intersect(V3 const &a, V3 const &b, 
                                        V3 const &p1,V3 const &p2, V3 const &p3,
                                        Hit_Result *hit_out,
                                        V3 *barycentric_out = NULL)
{
    // @Note: From: https://iquilezles.org/articles/hackingintersector/
    //
    // Returns whether segment pierces the triangle. Also fills Hit_Result and barycentric coords.
    
    // Init hit result.
    *hit_out = make_hit_result(a, b);
    
    V3 e1 = p2-p1, e2 = p3-p1, to_a = a-p1;
    V3 ab = b-a;
    
    ASSERT(nearly_zero(dot(ab, ab)) == FALSE);
    
    V3  n = cross(e1, e2);
    V3  q = cross(to_a, ab);
    f32 d = 1.0f/dot( n, ab);
    f32 u =    d*dot(-q, e2);
    f32 v =    d*dot( q, e1);
    f32 t =    d*dot(-n, to_a);
    f32 w = 1.0f - u - v;;
    
    // t here already represents percent (instead of a distance from segment origin along direction).
    f32 percent = t;
    if ((u < 0.0f) || (v < 0.0f) || ((u+v) > 1.0f) || (percent < 0.0f) || (percent > 1.0f)) {
        // Also ignore intersections that are behind a and ahead of b.
        if (barycentric_out) *barycentric_out = {-1.0f, -1.0f, -1.0f};
        return FALSE;
    } else {
        n = normalize_or_zero(n);
        hit_out->impact_point  = a + t*ab; //lerp(a, percent, b);
        hit_out->impact_normal = n;
        hit_out->normal        = n;
        hit_out->location      = hit_out->impact_point;
        hit_out->percent       = percent;
        hit_out->result        = TRUE;
        if (barycentric_out) *barycentric_out = {u, v, w};
        return TRUE;
    }
}

FUNCTION b32 segment_mesh_intersect(V3 const &a, V3 const &b,
                                    V3 const *vertices, s64 num_vertices,
                                    u32 const *indices, s64 num_indices,
                                    Hit_Result *hit_out)
{
    ASSERT((num_indices % 3) == 0);
    
    Hit_Result best_hit = make_hit_result(a, b);
    f32 best_percent    =  F32_MAX;
    for (s64 i = 0; i < num_indices; i += 3) {
        V3 p1 = vertices[indices[i + 0]];
        V3 p2 = vertices[indices[i + 1]];
        V3 p3 = vertices[indices[i + 2]];
        
        Hit_Result hit;
        segment_triangle_intersect(a, b, p1, p2, p3, &hit);
        if (hit.result && (hit.percent < best_percent)) {
            best_percent = hit.percent;
            best_hit     = hit;
        }
    }
    
    *hit_out = best_hit;
    return best_hit.result;
}

FUNCTION b32 segment_plane_intersect(V3 const &a, V3 const &b,
                                     V3 const &p, V3 const &n,
                                     Hit_Result *hit_out)
{
    // @Note: Returns whether segment defined by a and b pierces the plane defined by
    // normal n and point on the plane p. Also fills Hit_Result
    //
    // Assumes n is normalized.
    
    // Init hit result.
    *hit_out = make_hit_result(a, b);
    
    V3 ab     = normalize_or_zero(b-a);
    f32 denom = dot(n, ab);
    if (nearly_zero(denom, KINDA_SMALL_NUMBER))
        return FALSE;
    V3 ap = p-a;
    f32 t = dot(ap, n) / denom;
    
    f32 percent = t / length(b-a);
    hit_out->impact_point  = a + t*ab; //lerp(a, percent, b);
    hit_out->impact_normal = n;
    hit_out->normal        = n;
    hit_out->location      = hit_out->impact_point;
    hit_out->percent       = percent;
    hit_out->result        = TRUE;
    return TRUE;
}

enum Collision_Shape_Type
{
    CollisionShapeType_BOX,
    CollisionShapeType_SPHERE,
    CollisionShapeType_CAPSULE,
};

struct Collision_Shape
{
    // These define our shape local_space_to_collision_space_matrix. 
    // Read first note in the beginning of file to know what that means.
    Quaternion rotation;
    V3         center;
    
    union
    {
        struct
        {
            V3 half_extents;
        }; // Box
        
        struct
        {
            f32 radius;
        }; // Sphere
        
        struct
        {
            f32 radius;
            
            // Distance (in local-space) from the center of the capsule to the tip of the capsule.
            // (half_height - radius) gives you the "half axis height" that you can use to calculate the center of the upper and lower spheres/caps,
            f32 half_height;
        }; // Capsule
    };
    
    Collision_Shape_Type type;
};

FUNCTION inline Collision_Shape make_aabb(V3 const &center, V3 const &half_extents)
{
    // To avoid weird errors like division by zero and such.
    V3 half_ext         = max_v3(half_extents, v3(KINDA_SMALL_NUMBER));
    Collision_Shape result;
    result.rotation     = quaternion_identity();
    result.center       = center;
    result.half_extents = half_ext;
    result.type         = CollisionShapeType_BOX;
    return result;
}

FUNCTION inline Collision_Shape make_obb(V3 const &center, V3 const &half_extents, Quaternion const &rotation)
{
    V3 half_ext         = max_v3(half_extents, v3(KINDA_SMALL_NUMBER));
    Collision_Shape result;
    result.rotation     = rotation;
    result.center       = center;
    result.half_extents = half_ext;
    result.type         = CollisionShapeType_BOX;
    return result;
}

FUNCTION inline Collision_Shape make_sphere(V3 const &center, f32 radius)
{
    radius              = MAX(radius, KINDA_SMALL_NUMBER);
    Collision_Shape result;
    result.rotation     = quaternion_identity();
    result.center       = center;
    result.radius       = radius;
    result.type         = CollisionShapeType_SPHERE;
    return result;
}

FUNCTION inline Collision_Shape make_capsule(V3 const &center, f32 radius, f32 half_height, Quaternion const &rotation)
{
    radius              = MAX(radius, KINDA_SMALL_NUMBER);
    half_height         = MAX3(KINDA_SMALL_NUMBER, radius, half_height);
    Collision_Shape result;
    result.rotation     = rotation;
    result.center       = center;
    result.radius       = radius;
    result.half_height  = half_height;
    result.type         = CollisionShapeType_CAPSULE;
    return result;
}

FUNCTION f32 get_capsule_axis(Collision_Shape const &capsule, V3 *start, V3 *end)
{
    // @Note:
    //
    // Returns half axis height of capsule. Also fills the centers of the top and bottom caps/spheres in collision-space.
    
    ASSERT(start && end);
    
    f32 result = capsule.half_height - capsule.radius;
    V3 up      = capsule.rotation * V3_Y_AXIS;
    *start     = capsule.center + up * -result; // Center of bottom cap.
    *end       = capsule.center + up *  result; // Center of top cap.
    return result;
}

FUNCTION b32 segment_box_intersect(V3 const &a, V3 const &b, 
                                   Collision_Shape const &box,
                                   Hit_Result *hit_out)
{
    // @Note:
    // 
    // Returns whether segment pierces the box. Also fills Hit_Result.
    
    ASSERT(box.type == CollisionShapeType_BOX);
    
    // We don't need to normalize the direction here.
    V3 o = a;
    V3 d = b-a;
    
    ASSERT(nearly_zero(dot(d, d)) == FALSE);
    
    // Init hit result.
    *hit_out = make_hit_result(a, b);
    
    // Transform segment to local space of box.
    //
    // @Speed: This branch could be easily mispredicted if we test lots of boxes. Probably better to
    // split AABBs and OBBs into separate functions. 
    //
    b32 is_aabb = equal(ABS(box.rotation.w), 1.0f);
    if (is_aabb) {
        // Box is an AABB.
        o = o - box.center;
    } else {
        // Box is an OBB.
        V3 xaxis = box.rotation * V3_X_AXIS;
        V3 yaxis = box.rotation * V3_Y_AXIS;
        V3 zaxis = box.rotation * V3_Z_AXIS;
        M4x4 to_obb_matrix =
        {
            xaxis.x, xaxis.y, xaxis.z, -dot(xaxis, box.center),
            yaxis.x, yaxis.y, yaxis.z, -dot(yaxis, box.center),
            zaxis.x, zaxis.y, zaxis.z, -dot(zaxis, box.center),
            0.0f, 0.0f, 0.0f, 1.0f
        };
        
        o = transform_point (to_obb_matrix, o);
        d = transform_vector(to_obb_matrix, d);
    }
    
    V3 box_min = box.center - box.half_extents;
    V3 box_max = box.center + box.half_extents;
    
    V3 t_bmin = hadamard_div((-box.half_extents - o), d);
    V3 t_bmax = hadamard_div(( box.half_extents - o), d);
    
    V3 t_min3 = { MIN(t_bmin.x, t_bmax.x), MIN(t_bmin.y, t_bmax.y), MIN(t_bmin.z, t_bmax.z) };
    V3 t_max3 = { MAX(t_bmin.x, t_bmax.x), MAX(t_bmin.y, t_bmax.y), MAX(t_bmin.z, t_bmax.z) };
    
    f32 t_min = MAX3(t_min3.x, t_min3.y, t_min3.z);
    f32 t_max = MIN3(t_max3.x, t_max3.y, t_max3.z);
    
    // t value here is already a percent (instead of a distance from segment origin along direction).
    f32 percent = t_min;
    if ((t_min < 0.0f) || (t_min > t_max) || (percent < 0.0f) || (percent > 1.0f)) {
        // Also ignore t values that are behind a and ahead of b.
        return FALSE;
    } else {
        // Hit point.
        V3 p = o + t_min * d;
        
        // Compute hit normal.
        V3 pn      = hadamard_div(p,  box.half_extents); // Hit point "normalized"
        V3 pa      = abs(pn);                            // Hit point "normalized + absolute"
        s32 max_i  = (pa.x > pa.y)? ((pa.x > pa.z)? 0 : 2) : ((pa.y > pa.z)? 1: 2);
        V3 n       = {};
        n.I[max_i] = (f32)SIGN(pn.I[max_i]);
        
        // Transform the hit point and normal back to original collision-space.
        //
        if (is_aabb) {
            // Box is an AABB.
            p = p + box.center;
        } else {
            // Box is an OBB.
            // @Speed: Is there a faster way to do this transform?
            M4x4 to_collision_space = m4x4_from_translation_rotation_scale(box.center, box.rotation, v3(1));
            p = transform_point (to_collision_space, p);
            n = transform_vector(to_collision_space, n);
        }
        
        hit_out->impact_point  = p;
        hit_out->impact_normal = n;
        hit_out->normal        = n;
        hit_out->location      = p;
        hit_out->percent       = percent;
        hit_out->result        = TRUE;
        return TRUE;
    }
}

FUNCTION b32 segment_sphere_intersect(V3 const &a, V3 const &b, 
                                      Collision_Shape const &sphere,
                                      Hit_Result *hit_out)
{
    // @Note: Read ScratchPixel's article about ray-sphere intersection.
    // Or watch this video: https://www.youtube.com/watch?v=HFPlKQGChpE
    // 
    // Returns whether segment pierces the sphere. Also fills Hit_Result.
    //
    // No need to transform segment to local space of sphere as long as both are in 
    // the same collision-space.
    
    ASSERT(sphere.type == CollisionShapeType_SPHERE);
    
    V3 o = a;
    V3 d = normalize_or_zero(b-a);
    
    ASSERT(nearly_zero(dot(d, d)) == FALSE);
    
    // Init hit result.
    *hit_out = make_hit_result(a, b);
    
    V3 oc  = sphere.center - o;
    f32 tc = dot(oc, d);
    f32 y2 = dot(oc, oc) - tc*tc;
    f32 r2 = sphere.radius * sphere.radius;
    if (y2 > r2) {
        // We're not hitting the sphere.
        return FALSE;
    } else {
        // Calculate distance to entry and exit points of segment intersection with the sphere.
        f32 x  = _sqrt(r2 - y2);
        f32 t0 = tc - x; // A.k.a. t_min
        f32 t1 = tc + x; // A.k.a. t_max
        
        // @Sanity:
        if (t0 > t1)
            SWAP(t0, t1, f32);
        
        if (t0 < 0.0f) {
            // Entry point is behind segment origin, but t1 could still be in front. 
            // This would mean segment origin is _inside_ the sphere.
            t0 = t1;
            if (t0 < 0.0f) {
                // Both entry and exit points are behind the segment origin.
                return FALSE;
            }
        }
        
        f32 percent = t0 / length(b-a);
        if ((percent < 0.0f) || (percent > 1.0f))
            return FALSE;
        
        hit_out->impact_point  = o + t0 * d;
        hit_out->impact_normal = normalize_or_zero(hit_out->impact_point - sphere.center);
        hit_out->normal        = hit_out->impact_normal;
        hit_out->location      = hit_out->impact_point;
        hit_out->percent       = percent;
        hit_out->result        = TRUE;
        return TRUE;
    }
}

FUNCTION b32 segment_capsule_intersect(V3 const &a, V3 const &b, 
                                       Collision_Shape const &capsule,
                                       Hit_Result *hit_out)
{
    // @Note: Inigo Quilez: https://iquilezles.org/articles/intersectors/
    //
    // Returns whether segment pierces capsule. Also fills Hit_Result.
    
    ASSERT(capsule.type == CollisionShapeType_CAPSULE);
    
    V3 o = a;
    V3 d = normalize_or_zero(b-a);
    
    ASSERT(nearly_zero(dot(d, d)) == FALSE);
    
    // Init hit result.
    *hit_out = make_hit_result(a, b);
    
    // Calculate the capsule axis points, i.e., the center of the upper and lower spheres/caps in capsule's local space, and transform them to collision-space.
    f32 ra   = capsule.radius;
    V3 pa    = (capsule.rotation * -V3_UP*(capsule.half_height-ra)) + capsule.center;
    V3 pb    = (capsule.rotation *  V3_UP*(capsule.half_height-ra)) + capsule.center;
    
    V3  ba   = pb - pa;
    V3  oa   = o - pa;
    f32 baba = dot(ba, ba);
    f32 bard = dot(ba,  d);
    f32 baoa = dot(ba, oa);
    f32 rdoa = dot( d, oa);
    f32 oaoa = dot(oa, oa);
    f32 a1   = baba      - bard*bard;
    f32 b1   = baba*rdoa - baoa*bard;
    f32 c    = baba*oaoa - baoa*baoa - ra*ra*baba;
    f32 h    = b1*b1 - a1*c;
    if(h >= 0.0f)
    {
        f32 t = (-b1 - _sqrt(h)) / a1;
        f32 y = baoa + t*bard;
        
        if((y > 0.0f && y < baba) == FALSE) {
            // Didn't hit body, let's check if we hit the caps.
            V3 oc = (y <= 0.0)? oa : o - pb;
            b1    = dot( d, oc);
            c     = dot(oc, oc) - ra*ra;
            h     = b1*b1 - c;
            if(h > 0.0f)
                t = -b1 - _sqrt(h);  // We hit the caps, update t-value.
            else
                return FALSE;        // We hit nothing.
        }
        
        // @Todo: We don't care about this now, but it might be useful to know how to calculate "t_max",
        // i.e., the distance to the _exit_ point of the intersection.
        
        f32 percent = t / length(b-a);
        if ((percent < 0.0f) || (percent > 1.0f)) 
            return FALSE;
        
        // Impact point.
        V3 p = o + t * d;
        
        // Calculate impact normal.
        V3  pap = p - pa;
        f32 h   = CLAMP01(dot(pap,ba) / dot(ba,ba));
        V3 n    = (pap - h*ba) / ra;
        
        hit_out->impact_point  = p;
        hit_out->impact_normal = n;
        hit_out->normal        = n;
        hit_out->location      = p;
        hit_out->percent       = percent;
        hit_out->result        = TRUE;
        return TRUE;
    }
    
    return FALSE;
}

FUNCTION f32 closest_point_box_point(V3 const &c, Collision_Shape const &box, V3 *p_out = NULL, V3 *n_out = NULL)
{
    // @Note:
    //
    // Returns _squared_ distance between c (point outside) and closest point on box.
    //
    // To obtain closest point on box, we simply clamp c to extents of box.
    // If we are dealing with an OBB, we transform c to local space of OBB and do same thing.
    //
    // If c is inside the box, we will return c itself.
    
    ASSERT(box.type == CollisionShapeType_BOX);
    
    V3 o = c;
    
    // @Speed: This branch could be easily mispredicted if we test lots of boxes. Probably better to
    // split AABBs and OBBs into separate functions. 
    //
    b32 is_aabb = equal(ABS(box.rotation.w), 1.0f);
    if (is_aabb) {
        // Box is an AABB.
        o = o - box.center;
    } else {
        // Box is an OBB.
        V3 xaxis = box.rotation * V3_X_AXIS;
        V3 yaxis = box.rotation * V3_Y_AXIS;
        V3 zaxis = box.rotation * V3_Z_AXIS;
        M4x4 to_obb_matrix =
        {
            xaxis.x, xaxis.y, xaxis.z, -dot(xaxis, box.center),
            yaxis.x, yaxis.y, yaxis.z, -dot(yaxis, box.center),
            zaxis.x, zaxis.y, zaxis.z, -dot(zaxis, box.center),
            0.0f, 0.0f, 0.0f, 1.0f
        };
        
        o = transform_point(to_obb_matrix, o);
    }
    
    V3 p = clamp(-box.half_extents, o, box.half_extents);
    
    // Compute normal.
    V3 pn      = hadamard_div(p,  box.half_extents); // Hit point "normalized"
    V3 pa      = abs(pn);                            // Hit point "normalized + absolute"
    s32 max_i  = (pa.x > pa.y)? ((pa.x > pa.z)? 0 : 2) : ((pa.y > pa.z)? 1: 2);
    V3 n       = {};
    n.I[max_i] = (f32)SIGN(pn.I[max_i]);
    
    // Transform closest point back to original collision-space.
    if (is_aabb) {
        p = p + box.center;
    } else {
        // @Speed: Is there a faster way to do this transform?
        M4x4 to_collision_space = m4x4_from_translation_rotation_scale(box.center, box.rotation, v3(1));
        p = transform_point (to_collision_space, p);
        n = transform_vector(to_collision_space, n);
    }
    
    if (p_out) *p_out = p;
    if (n_out) *n_out = n;
    f32 result = length2(c - p);
    return result;
}

FUNCTION b32 box_box_intersect(Collision_Shape const &a, Collision_Shape const &b, Hit_Result *hit_out)
{
    // @Note: Christer Ericson - Real Time Collision Detection - P.101;
    //
    // Returns whether the two boxes overlap. And also fills out the Hit_Result.
    //
    // We will use the Separating Axis Theorem (SAT) to determine whether boxes overlap, while also 
    // calculating the Minimum Translation Vector (MTV) for resolving the collision.
    //
    // If both boxes are aligned with world axes (AABBs), we only have 3 axes to test; the world axes.
    // If one (or both) of them is an OBB, we will have 15 axes to test in total. Namely, the 3 local
    // axes of a, the 3 local axes of b, and the axes perpendicular to each previous axes (which are 9).
    //
    // In OBBs case, we can save a lot of operations by doing the test in the local space of one of
    // the boxes. That way we ensure that 3 axes are just the world axes:
    // {1, 0, 0} X
    // {0, 1, 0} Y
    // {0, 0, 1} Z
    // This helps us omit some multiplies as some terms will be multiplied by zero, and others by one.
    //
    // @DEBUG: RESOLVING two dynamically rotated OBBs (i.e., every frame) is not working properly.
    
    ASSERT(a.type == CollisionShapeType_BOX);
    ASSERT(b.type == CollisionShapeType_BOX);
    
    // Init hit result.
    *hit_out = make_hit_result({}, {});
    
    // MTV in collision-space.
    V3  best_normal            = {};
    f32 best_penetration_depth = F32_MAX;
    
    // @Speed: This branch could be easily mispredicted if we test lots of boxes. Probably better to
    // split AABBs and OBBs into separate functions. 
    //
    b32 a_is_aabb = equal(ABS(a.rotation.w), 1.0f);
    b32 b_is_aabb = equal(ABS(b.rotation.w), 1.0f);
    if (a_is_aabb && b_is_aabb) {
        // We only have 3 axes to test. The world axes.
        
        V3 t = b.center - a.center;
        
        for (s32 i = 0; i < 3; i++) {
            
            // Project extents onto the axis.
            f32 ra = a.half_extents.I[i];
            f32 rb = b.half_extents.I[i];
            
            // Project the translation_vector onto the axis.
            f32 tl     = t.I[i];
            f32 abs_tl = ABS(tl);
            if (abs_tl > (ra + rb))
                return FALSE;
            
            f32 penetration_depth = (ra + rb) - abs_tl;
            if (penetration_depth < best_penetration_depth) {
                best_penetration_depth = penetration_depth;
                V3 normal              = {};
                if (tl > 0.0f)      normal.I[i] = -1.0f;
                else if (tl < 0.0f) normal.I[i] =  1.0f;
                best_normal = normal;
            }
        }
    } else {
        // We have 15 axes to test.
        
        // Local axes of a and b.
        V3 a_axes[3] = {a.rotation*V3_X_AXIS, a.rotation*V3_Y_AXIS, a.rotation*V3_Z_AXIS};
        V3 b_axes[3] = {b.rotation*V3_X_AXIS, b.rotation*V3_Y_AXIS, b.rotation*V3_Z_AXIS};
        
        f32 ra, rb;
        M3x3 r, abs_r;
        
        // Construct 3x3 rotation matrix that expresses b in a's space. Transpose is a in b's space.
        for(s32 i = 0; i < 3; i++) {
            for(s32 j = 0; j < 3; j++) {
                r.II[i][j] = dot(a_axes[i], b_axes[j]);
            }
        }
        
        // Only test positive axes, and also add epsilon to account for when two edges are
        // parallel and we get a near zero cross product.
        for(s32 i = 0; i < 3; i++) {
            for(s32 j = 0; j < 3; j++) {
                abs_r.II[i][j] = ABS(r.II[i][j]) + KINDA_SMALL_NUMBER;
            }
        }
        
        // Compute translation vector from a to b (in a's space).
        V3 t = quaternion_inverse(a.rotation) * (b.center - a.center);
        
        // Test the 3 axes of a.
        for (s32 i = 0; i < 3; i++) {
            ra = a.half_extents.I[i];
            rb = dot(b.half_extents, get_row(abs_r, i));
            
            f32 tl     = t.I[i];
            f32 abs_tl = ABS(tl);
            if (abs_tl > (ra + rb))
                return FALSE;
            
            f32 penetration_depth = (ra + rb) - abs_tl;
            if (penetration_depth < best_penetration_depth) {
                best_penetration_depth = penetration_depth;
                V3 normal              = {};
                if (tl > 0.0f)      normal.I[i] = -1.0f;
                else if (tl < 0.0f) normal.I[i] =  1.0f;
                best_normal = a.rotation * normal;
            }
        }
        
        // Test the 3 axes of b.
        for (s32 i = 0; i < 3; i++) {
            ra = dot(a.half_extents, get_column(abs_r, i));
            rb = b.half_extents.I[i];
            
            f32 tl     = dot(t, get_column(r, i));
            f32 abs_tl = ABS(tl);
            if (abs_tl > (ra + rb))
                return FALSE;
            
            f32 penetration_depth = (ra + rb) - abs_tl;
            if (penetration_depth < best_penetration_depth) {
                best_penetration_depth = penetration_depth;
                V3 normal              = {};
                if (tl > 0.0f)      normal.I[i] = -1.0f;
                else if (tl < 0.0f) normal.I[i] =  1.0f;
                best_normal = a.rotation * normal;
            }
        }
        
        // Test the 9 axes.
        
        // @Note: If cross product is near zero, it means we already tested that axis before.
        //
#define UPDATE_MTV(a_axis_in_a, b_axis_in_a)                \
f32 penetration_depth = (ra + rb) - abs_tl;             \
if (penetration_depth < best_penetration_depth) {       \
V3 normal_in_a = cross(a_axis_in_a, b_axis_in_a);   \
if (!nearly_zero(normal_in_a, KINDA_SMALL_NUMBER)) {\
if (tl > 0.0f) normal_in_a = -normal_in_a;      \
best_normal = a.rotation * normal_in_a;         \
best_penetration_depth = penetration_depth;     \
}                                                   \
}
        // Test axis = a.x cross b.x
        {
            ra = a.half_extents.y * abs_r._31 + a.half_extents.z * abs_r._21;
            rb = b.half_extents.y * abs_r._13 + b.half_extents.z * abs_r._12;
            f32 tl     = t.z * r._21 - t.y * r._31;
            f32 abs_tl = ABS(tl);
            if (abs_tl > (ra + rb))
                return FALSE;
            UPDATE_MTV(V3_X_AXIS, get_row(abs_r, 0))
        }
        
        // Test axis = a.x cross b.y
        {
            ra = a.half_extents.y * abs_r._32 + a.half_extents.z * abs_r._22;
            rb = b.half_extents.x * abs_r._13 + b.half_extents.z * abs_r._11;
            f32 tl     = t.z * r._22 - t.y * r._32;
            f32 abs_tl = ABS(tl);
            if (abs_tl > (ra + rb))
                return FALSE;
            UPDATE_MTV(V3_X_AXIS, get_row(abs_r, 1))
        }
        
        // Test axis = a.x cross b.z
        {
            ra = a.half_extents.y * abs_r._33 + a.half_extents.z * abs_r._23;
            rb = b.half_extents.x * abs_r._12 + b.half_extents.y * abs_r._11;
            f32 tl     = t.z * r._23 - t.y * r._33;
            f32 abs_tl = ABS(tl);
            if (abs_tl > (ra + rb))
                return FALSE;
            UPDATE_MTV(V3_X_AXIS, get_row(abs_r, 2))
        }
        
        // Test axis = a.y cross b.x
        {
            ra = a.half_extents.x * abs_r._31 + a.half_extents.z * abs_r._11;
            rb = b.half_extents.y * abs_r._23 + b.half_extents.z * abs_r._22;
            f32 tl     = t.x * r._31 - t.z * r._11;
            f32 abs_tl = ABS(tl);
            if (abs_tl > (ra + rb))
                return FALSE;
            UPDATE_MTV(V3_Y_AXIS, get_row(abs_r, 0))
        }
        
        // Test axis = a.y cross b.y
        {
            ra = a.half_extents.x * abs_r._32 + a.half_extents.z * abs_r._12;
            rb = b.half_extents.x * abs_r._23 + b.half_extents.z * abs_r._21;
            f32 tl     = t.x * r._32 - t.z * r._12;
            f32 abs_tl = ABS(tl);
            if (abs_tl > (ra + rb))
                return FALSE;
            UPDATE_MTV(V3_Y_AXIS, get_row(abs_r, 1))
        }
        
        // Test axis = a.y cross b.z
        {
            ra = a.half_extents.x * abs_r._33 + a.half_extents.z * abs_r._13;
            rb = b.half_extents.x * abs_r._22 + b.half_extents.y * abs_r._21;
            f32 tl     = t.x * r._33 - t.z * r._13;
            f32 abs_tl = ABS(tl);
            if (abs_tl > (ra + rb))
                return FALSE;
            UPDATE_MTV(V3_Y_AXIS, get_row(abs_r, 2))
        }
        
        // Test axis = a.z cross b.x
        {
            ra = a.half_extents.x * abs_r._21 + a.half_extents.y * abs_r._11;
            rb = b.half_extents.y * abs_r._33 + b.half_extents.z * abs_r._32;
            f32 tl     = t.y * r._11 - t.x * r._21;
            f32 abs_tl = ABS(tl);
            if (abs_tl > (ra + rb))
                return FALSE;
            UPDATE_MTV(V3_Z_AXIS, get_row(abs_r, 0))
        }
        
        // Test axis = a.z cross b.y
        {
            ra = a.half_extents.x * abs_r._22 + a.half_extents.y * abs_r._12;
            rb = b.half_extents.x * abs_r._33 + b.half_extents.z * abs_r._31;
            f32 tl     = t.y * r._12 - t.x * r._22;
            f32 abs_tl = ABS(tl);
            if (abs_tl > (ra + rb))
                return FALSE;
            UPDATE_MTV(V3_Z_AXIS, get_row(abs_r, 1))
        }
        
        // Test axis = a.z cross b.z
        {
            ra = a.half_extents.x * abs_r._23 + a.half_extents.y * abs_r._13;
            rb = b.half_extents.x * abs_r._32 + b.half_extents.y * abs_r._31;
            f32 tl     = t.y * r._13 - t.x * r._23;
            f32 abs_tl = ABS(tl);
            if (abs_tl > (ra + rb))
                return FALSE;
            UPDATE_MTV(V3_Z_AXIS, get_row(abs_r, 2))
        }
    }
    
    // @Note: I'm adding epsilon to penetration depth to ensure there's no collision after resolving.
    //
    best_penetration_depth += KINDA_SMALL_NUMBER;
    best_normal = normalize_or_zero(best_normal);
    V3 location = a.center + best_normal*best_penetration_depth;
    V3 closest_point;
    closest_point_box_point(location, b, &closest_point);
    
    hit_out->impact_point  = closest_point;
    hit_out->impact_normal = best_normal;
    hit_out->normal        = best_normal;
    hit_out->location      = location;
    hit_out->result        = TRUE;
    return TRUE;
}

FUNCTION b32 sphere_box_intersect(Collision_Shape const &sphere, Collision_Shape const &box, Hit_Result *hit_out)
{
    // @Note:
    //
    // Returns whether sphere collides with box. Also fills Hit_Result.
    //
    // We find the neartest point on the box to the center of the sphere and check if the distance to it
    // is smaller/larger the radius of the sphere.
    //
    // @DEBUG: What happens when the sphere is _inside_ the box?
    
    ASSERT(sphere.type == CollisionShapeType_SPHERE);
    ASSERT(box.type == CollisionShapeType_BOX);
    
    // Init hit result.
    *hit_out = make_hit_result({}, {});
    
    V3 on_box, box_normal;
    f32 dist2 = closest_point_box_point(sphere.center, box, &on_box, &box_normal);
    
    if (dist2 > (sphere.radius * sphere.radius))
        return FALSE;
    
    f32 penetration_depth  = sphere.radius - _sqrt(dist2) + KINDA_SMALL_NUMBER;
    hit_out->impact_point  = on_box;
    hit_out->impact_normal = box_normal;
    hit_out->normal        = normalize_or_zero(sphere.center - on_box);
    hit_out->location      = sphere.center + hit_out->normal * penetration_depth;
    hit_out->result        = TRUE;
    return TRUE;
}

FUNCTION b32 capsule_box_intersect(Collision_Shape const &capsule, Collision_Shape const &box, Hit_Result *hit_out)
{
    // @Note:
    //
    // Returns whether capsule collides with box. Also fills Hit_Result.
    //
    // This is very similar to the sphere vs. box intersection test actually.
    // The only thing is: we need to find the "box radius". 
    //
    // We first try to find the closest point on the axis of the capsule to the box center.
    // Then we define a line from the point we found and the box center. Then we do a line vs. box
    // intersection to see where we "pierce" the box. The length of the vector from that "pierce point"
    // and the box center is the "box radius".
    //
    // To calculate the impact point, we find the closest point on the box to the closest point on the
    // axis we computed earlier.
    
    ASSERT(capsule.type == CollisionShapeType_CAPSULE);
    ASSERT(box.type == CollisionShapeType_BOX);
    
    // Init hit result.
    *hit_out = make_hit_result({}, {});
    
    V3 start, end;
    get_capsule_axis(capsule, &start, &end);
    
    // Point on capsule axis that is the closest to box center.
    V3 on_axis;
    f32 dist2 = closest_point_segment_point(box.center, start, end, NULL, &on_axis);
    
    // Pretty convoluted way for calculating the "radius" of the box.
    Hit_Result hit;
    segment_box_intersect(on_axis, box.center, box, &hit);
    f32 box_radius = length(box.center - hit.impact_point);
    
    if (dist2 > SQUARE(capsule.radius + box_radius))
        return FALSE;
    
    // Get closest point on box surface (and normal) to point on axis.
    V3 on_box, box_normal;
    closest_point_box_point(on_axis, box, &on_box, &box_normal);
    
    f32 penetration_depth  = (capsule.radius + box_radius) - _sqrt(dist2) + KINDA_SMALL_NUMBER;
    hit_out->impact_point  = on_box;
    hit_out->impact_normal = box_normal;
    hit_out->normal        = normalize_or_zero(on_axis - on_box);
    hit_out->location      = capsule.center + hit_out->normal * penetration_depth;
    hit_out->result        = TRUE;
    return TRUE;
}

FUNCTION b32 sphere_sphere_intersect(Collision_Shape const &a, Collision_Shape const &b, Hit_Result *hit_out)
{
    // @Note:
    //
    // Returns whether spheres overlap. Also fills Hit_Result.
    //
    // Basically check if vector from b center to a center is larger/smaller than sum of their radii.
    
    ASSERT(a.type == CollisionShapeType_SPHERE);
    ASSERT(b.type == CollisionShapeType_SPHERE);
    
    // Init hit result.
    *hit_out = make_hit_result({}, {});
    
    f32 d_len             = length(a.center - b.center);
    f32 penetration_depth = (a.radius + b.radius) - d_len;
    if (penetration_depth < 0.0f)
        return FALSE;
    
    penetration_depth     += KINDA_SMALL_NUMBER;
    V3 normal              = (a.center - b.center) / d_len;
    hit_out->impact_point  = b.center + normal*b.radius;
    hit_out->impact_normal = normal;
    hit_out->normal        = normal;
    hit_out->location      = a.center + normal*penetration_depth;
    hit_out->result        = TRUE;
    return TRUE;
}

FUNCTION b32 sphere_capsule_intersect(Collision_Shape const &sphere, Collision_Shape const &capsule, Hit_Result *hit_out)
{
    // @Note:
    //
    // Returns whether sphere overlaps with capsule. Also fills Hit_Result.
    
    ASSERT(sphere.type == CollisionShapeType_SPHERE);
    ASSERT(capsule.type == CollisionShapeType_CAPSULE);
    
    V3 start, end;
    get_capsule_axis(capsule, &start, &end);
    
    V3 on_axis;
    f32 dist2 = closest_point_segment_point(sphere.center, start, end, NULL, &on_axis);
    if (dist2 > SQUARE(sphere.radius + capsule.radius))
        return FALSE;
    
    b32 result = sphere_sphere_intersect(sphere, make_sphere(on_axis, capsule.radius), hit_out);
    return result;
}

FUNCTION b32 capsule_capsule_intersect(Collision_Shape const &a, Collision_Shape const &b, Hit_Result *hit_out)
{
    // @Note:
    //
    // Returns whether sphere overlaps with capsule. Also fills Hit_Result.
    
    ASSERT(a.type == CollisionShapeType_CAPSULE);
    ASSERT(b.type == CollisionShapeType_CAPSULE);
    
    V3 a_start, a_end, b_start, b_end;
    get_capsule_axis(a, &a_start, &a_end);
    get_capsule_axis(b, &b_start, &b_end);
    
    V3 on_a_axis, on_b_axis;
    f32 dist2 = closest_point_segment_segment(a_start, a_end, b_start, b_end, NULL, &on_a_axis, NULL, &on_b_axis);
    
    f32 d_len             = length(on_a_axis - on_b_axis);
    f32 penetration_depth = (a.radius + b.radius) - d_len;
    if (penetration_depth < 0.0f)
        return FALSE;
    
    penetration_depth     += KINDA_SMALL_NUMBER;
    V3 normal              = (on_a_axis - on_b_axis) / d_len;
    hit_out->impact_point  = on_b_axis + normal*b.radius;
    hit_out->impact_normal = normal;
    hit_out->normal        = normal;
    hit_out->location      = a.center + normal*penetration_depth;
    hit_out->result        = TRUE;
    return TRUE;
}
