#ifndef ORH_COLLISION_H
#define ORH_COLLISION_H

/* orh_collision.h - v0.01 - C++ collision routines.

#include "orh.h" before this file.

REVISION HISTORY:

*/

union Ray
{
    struct { V3  origin; V3 direction; f32 t; };
    struct { V3  o;      V3 d;         f32 t; };
    
};

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

FUNCTION Rect3 get_plane_bounds(V3 center, V3 normal, f32 scale)
{
    V3 tangent, bitangent;
    calculate_tangents(normal, &tangent, &bitangent);
    
    f32 half_scale = 0.5f * scale;
    
    V3 p0 = center - tangent*half_scale - bitangent*half_scale;
    V3 p1 = center + tangent*half_scale + bitangent*half_scale;
    
    Rect3 result = {min_v3(p0, p1), max_v3(p0, p1)};
    return result;
}

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


#endif //ORH_COLLISION_H
