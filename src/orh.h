/* orh.h - v0.88 - C++ utility library. Includes types, math, string, memory arena, and other stuff.

In _one_ C++ file, #define ORH_IMPLEMENTATION before including this header to create the
 implementation. 
Like this:
#include ...
#include ...
#define ORH_IMPLEMENTATION
#include "orh.h"

REVISION HISTORY:
0.88 - added clamp_angle() and fixed get_euler(). Add macros for small fractions.
0.87 - added base_mouse_resolution to OS_State. Added euler to quaternion conversion.
0.86 - added clamp_axis(), normalize_axis(), normalize_half_axis(). Added Cursor_Mode and Display_Mode
0.85 - added raw input for mouse deltas. Added enable and disable cursor. Added get_euler() from quat.
0.84 - added V4s type. Added M3x3 type. Added some M4x4 helper functions. Removed operator*(M4x4, V3). Added FUNCDEF/inline to definitions as well.
0.83 - fixed array_remove_range() assert. Added "found" parameter to table_find().
0.82 - added per-tick and per-frame inputs for keys.
0.81 - improved move_towards() api. Removed instrumentation and created orh_profiler.cpp instead.
0.80 - added initial instrumentation.
0.79 - fixed calculate_tangents().
0.78 - nlerp() does normalize_or_identity() now. Added m4x4_from_translation_rotation_scale();
0.77 - added array_add_unique(). Added operator== for V3.
0.76 - no need to pass scale to get_rotation(). Added invert() to invert M4x4. Fixed SIGN() to include 0.
0.75 - quaternion_get_axis() uses normalize_or_zero now in case the vector part of the quaternion is zero.
0.74 - fixed F32_MIN and F64_MIN, we had minimum normalized positive floating-point number.
0.73 - array_add() now returns pointer to newly added item.
0.72 - fixed bug with string path helper functions.
0.71 - renamed print() and added ability to load File_Info and File_Group and some string functions.
0.70 - changed array_init() and did some cleanup.
0.69 - added array_resize() for if we want to allocate memory upfront and fill data using indexing.
0.68 - added TRUE and FALSE macros.
0.67 - added clear_key_states() and clear_key_states_all().
0.66 - added unlerp() and remap().
0.65 - fixed perspective projections to use z range [0, 1].
0.64 - corrected frac() implementation.
0.63 - smooth_step correction. Removed V2, V3, and V4 variants.
0.62 - removed memory_copy(), memory_set() and arena_push_set(). Added alignment to arena_push().
0.61 - added table_find_pointer() and initialize parameter for Arrays.
0.60 - added Sound.
0.59 - added audio output to OS_State.
0.58 - some fixes.
0.57 - added [0-1] hsv() that gives us V4 rgba.
0.56 - added arena_push_set() and PUSH_ARRAY_SET.
0.55 - added pow() for V2 and V3.
0.54 - added random_next64() and random_nextd().
0.53 - added random_rangef().
0.52 - cleaned up rect functions.
0.51 - added array_find_index().
0.50 - Improved arenas and added thread local scratch arenas.
0.49 - added context cracking.
0.48 - added operator!= for String8.
0.47 - added fps_max option.
0.46 - arena_init() now takes max size as parameter. And changed default auto_align from 8 to 1.
0.45 - added f32 version of move_towards().
0.44 - added fullscreen flag. OS layer has to check it after update and toggle fullscreen.
0.43 - removed mouse_screen from OS_State.
0.42 - added useful functions for Array: array_remove_range(), array_pop_().
0.41 - added random_range_v2().
0.40 - added rotate_point_around_pivot().
0.39 - completely reworked key state processing and implemented move_towards().
0.38 - added more functions to dynamic array.
0.37 - added templated String8 helper function get().
0.36 - added was_released().
0.35 - added round, clamp, ceil, for V2.
0.34 - Added section for helper functions and implemented ones for u32 flags.
0.33 - Expanded Button_State to have was_pressed() with duration_until_trigger_seconds.
0.32 - added frac().
0.31 - added random for v3 and random_nextf() for floats.
0.30 - added mouse_scroll.
0.29 - changed Rect size to half_size, added utils for Rect, and added overloads for hadamard_mul.
0.28 - added quaternion_identity().
0.27 - added min and max for V3s.
0.26 - added os->time, repeat(), ping_pong(), clamp for V3, bezier2() and other things.
0.25 - added PRNG.
0.24 - refined and tested generic hash table.
0.23 - implemented generic hash table, but still untested and incomplete.
0.22 - added generic dynamic arrays using templates. 
0.21 - added defer() and made many improvements to string formatting and arena APIs.
0.20 - renamed String8 functions and added sb_reset() for String_Builder.
0.19 - added more operator overloads for V2s and fixed mistake with some overloads.
0.18 - made small functions inline.
0.17 - added operator overloads for V2s.
0.16 - added write_entire_file in OS_State, prefixed String_Builder functions with sb_.
0.15 - added smooth_step() and smoother_step() for V2, V3, and V4.
0.14 - fixed bug in string_format_list()
0.13 - you can now specify angles in turns using sin_turns(), cos_turns(), and so on. Replaced to_radians() and to_degrees() with macros.
0.12 - added unproject().
0.11 - replaced some u32 indices with s32.
0.10 - fixed bug in String_Builder append().
0.09 - made mouse_screen and mouse_ndc V3 with .z = 0.
0.08 - expanded enum for keyboard keys.
0.07 - added orthographic_2d and orthographic_3d.
0.06 - added V3R, V3U and V3F.
0.05 - added V2s.
0.04 - re-structured functions to use result value to make debugging easier.
0.03 - added user input utilities and expanded OS_State, added CLAMP01_RANGE(), added print().
0.02 - added V2u, expanded OS_State, added aspect_ratio_fit().

CONVENTIONS:
* CCW winding order for front faces.
* Right-handed coordinate system: +X is right // +Y is up // -Z is forward.
* Matrices are row-major with column vectors.
* First pixel pointed to is top-left-most pixel in image.
* UV-coords origin is at top-left corner (DOESN'T match with vertex coordinates).
* When storing paths, if string name has "folder" in it, then it ends with '/' or '\\'.

TODO:
[] Per-frame and per-tick input for gamepads!
[] Dynamically growing arenas (maybe make a list instead of asserting when we go past arena->max).
[] Helper functions that return forward/right/up vectors from passed transform matrix.
[] arenas never decommit memory. Find a good way to add that.

MISC:
=== === ===

 About Flags:
 Toggle     : dst  ^=  src;
 Set        : dst  |=  src;
 clear      : dst  &= ~src;
 is_set     : (src & flag) == flag;
 is_cleared : (src & flag) == 0;

=== === ===

About affine transformation matrices:
Our "object to world" matrices are considered affine transforms. They're also known as "model matrices".
   Our affine transforms _in most cases_ contain only translation, rotation, and scale information.

 Composing affine transformation matrices:
We can compose an affine matrix with T = translation, R = rotation, S = scale matrices by doing:

M = (T * (R * S)) --> We apply scale first, then rotation, then translation.

  We can optimize by not having to do all these matrix muls:
  Doing (R * S) only fills the upper 3x3 "linear" part and M._44 remains 1.0f, we can instead just multiply
  columns of rotation matrix with components of scale vector and insert the translation vector in the last "4th" column.



To compose the inverse (i- prefix denotes inverse):
    iM = (iS * (iR * iT))
    
    iS = scale matrix using (1.0f / scale vector).
    iR = transpose of original rotation matrix.
    iT = translation matrix using (-translation vector).

If the transform has no scaling, we can optimize with this:
   https://stackoverflow.com/a/2625420

=== === ===

About matrix "pictures" - HMH EP.362:

Let's say we are multiplying a matrix M and a vector v, then there are two modes/pictures/perspectives/conceptualizations of that multiplication:

1) The row picture.
2) The column picture.

The row picture is essentially doing the dot product of the each row of the matrix M with the vector v:
result.x = dot(M.row[0], v);
result.y = dot(M.row[1], v);
result.z = dot(M.row[2], v);

The column picture is "scaling" the columns of the matrix M by corresponding components of vector v:
result   = M.column[0]*v.x + M.column[1]*v.y + M.column[2]*v.z;

Both produce the same result, only we are looking at the multiplication in two different ways.

=== === ===

About multiplication order and WORLD vs LOCAL:

Multiplying quaternions to "add" rotation:
Let's say we have an orientation represented by a quaternion (ori), and we wanted to modify that orientation by adding a rotation around some axis, then:
We create a new quaternion that represents the rotation around said axis by some amount (call it q).

// To rotate around WORLD axis.
ori = q * ori; 

// To rotate around LOCAL axis.
ori = ori * q;

This is also true of matrix multiplication.

=== === ===

About euler angles:

 The order of applying euler rotations is: yaw -> pitch -> roll

=== === ===

*/

//
// THE SHELF - DEPRECATED/UNUSED STUFF
//
#if 0
FUNCDEF inline M4x4 translate_world(M4x4 affine, V3 t); // Along world axes.
FUNCDEF inline M4x4 translate_local(M4x4 affine, V3 t); // Along local axes.
FUNCDEF inline M4x4 translate(M4x4 affine, V3 t);
M4x4 translate_world(M4x4 affine, V3 t)
{
    M4x4 result = affine;
    result._14 += t.x;
    result._24 += t.y;
    result._34 += t.z;
    return result;
}
M4x4 translate_local(M4x4 affine, V3 t)
{
    M4x4 result  = affine;
    result._14  += dot(result.row[0].xyz, t);
    result._24  += dot(result.row[1].xyz, t);
    result._34  += dot(result.row[2].xyz, t);
    return result;
}
M4x4 translate(M4x4 affine, V3 t)
{
    return translate_world(affine, t);
}
#endif


#ifndef ORH_H
#define ORH_H

/////////////////////////////////////////
//~
// Context cracking
//
// @Note: From Allen "mr4th" Webster.
#if defined(__clang__)
#    define COMPILER_CLANG 1
#    if defined(_WIN32)
#        define OS_WINDOWS 1
#    elif defined(__gnu_linux__)
#        define OS_LINUX 1
#    elif defined(__APPLE__) && defined(__MACH__)
#        define OS_MAC 1
#    else
#        error missing OS detection
#    endif
#    if defined(__amd64__)
#        define ARCH_X64 1
#    elif defined(__i386__) // Verify this works on clang.
#        define ARCH_X86 1
#    elif defined(__arm__) // Verify this works on clang.
#        define ARCH_ARM 1
#    elif defined(__aarch64__) // Verify this works on clang.
#        define ARCH_ARM64 1
#    else
#        error missing ARCH detection
#    endif
#elif defined(_MSC_VER)
#    define COMPILER_CL 1
#    if defined(_WIN32)
#        define OS_WINDOWS 1
#    else
#        error missing OS detection
#    endif
#    if defined(_M_AMD64)
#        define ARCH_X64 1
#    elif defined(_M_I86)
#        define ARCH_X86 1
#    elif defined(_M_ARM)
#        define ARCH_ARM 1
#    else
#        error missing ARCH detection
#    endif
#elif defined(__GNUC__)
#    define COMPILER_GCC 1
#    if defined(_WIN32)
#        define OS_WINDOWS 1
#    elif defined(__gnu_linux__)
#        define OS_LINUX 1
#    elif defined(__APPLE__) && defined(__MACH__)
#        define OS_MAC 1
#    else
#        error missing OS detection
#    endif
#    if defined(__amd64__)
#        define ARCH_X64 1
#    elif defined(__i386__)
#        define ARCH_X86 1
#    elif defined(__arm__)
#        define ARCH_ARM 1
#    elif defined(__aarch64__)
#        define ARCH_ARM64 1
#    else
#        error missing ARCH detection
#    endif
#else
#    error no context cracking for this compiler
#endif

#if !defined(COMPILER_CL)
#    define COMPILER_CL 0
#endif
#if !defined(COMPILER_CLANG)
#    define COMPILER_CLANG 0
#endif
#if !defined(COMPILER_GCC)
#    define COMPILER_GCC 0
#endif
#if !defined(OS_WINDOWS)
#    define OS_WINDOWS 0
#endif
#if !defined(OS_LINUX)
#    define OS_LINUX 0
#endif
#if !defined(OS_MAC)
#    define OS_MAC 0
#endif
#if !defined(ARCH_X64)
#    define ARCH_X64 0
#endif
#if !defined(ARCH_X86)
#    define ARCH_X86 0
#endif
#if !defined(ARCH_ARM)
#    define ARCH_ARM 0
#endif
#if !defined(ARCH_ARM64)
#    define ARCH_ARM64 0
#endif

/////////////////////////////////////////
//~
// Types
//
#ifndef ORH_NO_STD_MATH
#include <math.h> 
#endif

#ifndef FUNCDEF
#    ifdef ORH_STATIC
#        define FUNCDEF static
#    else
#        define FUNCDEF extern
#    endif
#endif

#include <stddef.h>
#include <stdint.h>

typedef int8_t             s8;
typedef int16_t            s16;
typedef int32_t            s32;
typedef int32_t            b32;
typedef int64_t            s64;

typedef uint8_t            u8;
typedef uint16_t           u16;
typedef uint32_t           u32;
typedef uint64_t           u64;

typedef uintptr_t          umm;
typedef intptr_t           smm;

typedef float              f32;
typedef double             f64;

#define FUNCTION           static
#define LOCAL_PERSIST      static
#define GLOBAL             static

#define S8_MIN             (-S8_MAX  - 1)
#define S8_MAX             0x7F
#define S16_MIN            (-S16_MAX - 1)
#define S16_MAX            0x7FFF
#define S32_MIN            (-S32_MAX - 1)
#define S32_MAX            0x7FFFFFFF
#define S64_MIN            (-S64_MAX - 1)
#define S64_MAX            0x7FFFFFFFFFFFFFFFLL

// Un-signed minimum is zero.  
#define U8_MAX             0xFFU
#define U16_MAX            0xFFFFU
#define U32_MAX            0xFFFFFFFFU
#define U64_MAX            0xFFFFFFFFFFFFFFFFULL

#define F32_MIN_POSITIVE   1.17549435e-38F
#define F32_MIN            -F32_MAX
#define F32_MAX            3.40282347e+38F

#define F64_MIN_POSITIVE   2.2250738585072014e-308
#define F64_MIN            -F64_MAX
#define F64_MAX            1.7976931348623157e+308

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#if COMPILER_CL
#    define threadvar __declspec(thread)
#elif COMPILER_CLANG
#    define threadvar __thread
#elif COMPILER_GCC
#    define threadvar __thread
#else
#    error threadvar not defined for this compiler
#endif

// Nesting macros in if-statements that have no curly-brackets causes issues. Using DOWHILE() avoids all of them.
#define DOWHILE(s)         do{s}while(0)
#define SWAP(a, b, Type)   DOWHILE(Type _t = a; a = b; b = _t;)

#if DEVELOPER
#    if COMPILER_CL
#        define ASSERT(expr) DOWHILE(if (!(expr)) __debugbreak();)
#    else
#        define ASSERT(expr) DOWHILE(if (!(expr)) __builtin_trap();)
#    endif
#    if OS_WINDOWS
#        define ASSERT_HR(hr) ASSERT(SUCCEEDED(hr))
#    endif
#else
#    define ASSERT(expr)      (void)(expr)
#    define ASSERT_HR(expr)   (void)(expr)
#endif

#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

#define STRINGIFY_(S) #S
#define STRINGIFY(s) STRINGIFY_(s)
#define GLUE_(a, b) a##b
#define GLUE(a, b) GLUE_(a, b)

#define STATIC_ASSERT(cond, name) typedef u8 GLUE(name,__LINE__) [(cond)?1:-1]

/////////////////////////////////////////
//~
// Defer Scope
//
// @Note: From gingerBill and Ignacio Castano. Similar to defer() in Go but executes at end of scope.
template <typename F>
struct DeferScope 
{
    DeferScope(F f) : f(f) {}
    ~DeferScope() { f(); }
    F f;
};

template <typename F>
DeferScope<F> MakeDeferScope(F f) { return DeferScope<F>(f); };

#define defer(code) \
auto GLUE(__defer_, __COUNTER__) = MakeDeferScope([&](){code;})

/////////////////////////////////////////
//~
// Math
//
#define PI    3.14159265358979323846
#define TAU   6.28318530717958647692
#define PI32  3.14159265359f
#define TAU32 6.28318530718f
#define RADS_TO_DEGS  (360.0f / TAU32)
#define RADS_TO_TURNS (1.0f   / TAU32)
#define DEGS_TO_RADS  (TAU32  / 360.0f)
#define DEGS_TO_TURNS (1.0f   / 360.0f)
#define TURNS_TO_RADS (TAU32  / 1.0f)
#define TURNS_TO_DEGS (360.0f / 1.0f)

#define SMALL_NUMBER              (1.e-8f)
#define KINDA_SMALL_NUMBER        (1.e-4f)
#define BIG_NUMBER                (3.4e+38f)
#define DOUBLE_SMALL_NUMBER       (1.e-8)
#define DOUBLE_KINDA_SMALL_NUMBER (1.e-4)
#define DOUBLE_BIG_NUMBER         (3.4e+38)

#define MIN(A, B)              ((A < B) ? (A) : (B))
#define MAX(A, B)              ((A > B) ? (A) : (B))
#define MIN3(A, B, C)          MIN(MIN(A, B), C)
#define MAX3(A, B, C)          MAX(MAX(A, B), C)

#define CLAMP_UPPER(upper, x)      MIN(upper, x)
#define CLAMP_LOWER(x, lower)      MAX(x, lower)
#define CLAMP(lower, x, upper)    (CLAMP_LOWER(CLAMP_UPPER(upper, x), lower))
#define CLAMP01(x)                 CLAMP(0, x, 1)
#define CLAMP01_RANGE(min, x, max) CLAMP01(safe_div0(x-min, max-min))

#define SQUARE(x) ((x) * (x))
#define CUBE(x)   ((x) * (x) * (x))
#define ABS(x)    ((x) > 0 ? (x) : -(x))
#define SIGN(x)   ((x > 0) - (x < 0))

#if COMPILER_CL
#    pragma warning(push)
#    pragma warning(disable:4201)
#endif
union V2
{
    struct { f32 x, y; };
    struct { f32 u, v; };
    f32 I[2];
};

union V2u
{
    struct { u32 x, y; };
    struct { u32 w, h; };
    u32 I[2];
};

union V2s
{
    struct { s32 x, y; };
    struct { s32 s, t; };
    s32 I[2];
};

union V3
{
    // @Note: pitch around right (x-axis); yaw around up (y-axis); roll around -forward (z-axis)
    struct { f32 x, y, z; };
    struct { V2 xy; f32 ignored0_; };
    struct { f32 ignored1_; V2 yz; };
    struct { f32 r, g, b; };
    struct { f32 pitch, yaw, roll; };
    f32 I[3];
};
extern const V3 V3_ZERO;
extern const V3 V3_X_AXIS;
extern const V3 V3_Y_AXIS;
extern const V3 V3_Z_AXIS;
extern const V3 V3_RIGHT;
extern const V3 V3_UP;
extern const V3 V3_FORWARD;

union V4
{
    struct 
    { 
        union
        {
            V3 xyz;
            struct { f32 x, y, z; };
        };
        f32 w;
    };
    struct { V2 xy, zw; };
    struct { f32 ignored0_; V2 yz; f32 ignored1_; };
    struct 
    { 
        union
        {
            V3 rgb;
            struct { f32 r, g, b; };
        };
        f32 a;
    };
    
    f32 I[4];
};

union V4s
{
    struct { s32 x, y, z, w; };
    s32 I[4];
};

union Quaternion
{
    struct
    {
        union
        {
            V3 xyz, v;
            struct { f32 x, y, z; };
        };
        f32 w;
    };
    V4 xyzw;
    f32 I[4];
};

union M3x3
{
    // @Note: We use row-major with column vectors!
    
    // @Note: If you decide to switch to column-major, make sure to modify functions that use [] indexing.
    // Also make sure you're consistent with graphics API.
    
    f32 II [3][3];
    f32 I     [9];
    V3  row   [3];
    struct
    {
        // First index is row.
        f32 _11, _12, _13;
        f32 _21, _22, _23;
        f32 _31, _32, _33;
    };
};

union M4x4
{
    // @Note: We use row-major with column vectors!
    
    // @Note: If you decide to switch to column-major, make sure to modify functions that use [] indexing.
    // Also make sure you're consistent with graphics API.
    
    f32 II [4][4];
    f32 I    [16];
    V4  row   [4];
    struct
    {
        // First index is row.
        f32 _11, _12, _13, _14;
        f32 _21, _22, _23, _24;
        f32 _31, _32, _33, _34;
        f32 _41, _42, _43, _44;
    };
};

struct M4x4_Inverse
{
    M4x4 forward;
    M4x4 inverse;
};

struct Range { f32 min, max; };
struct Rect2 { V2  min, max; };
struct Rect3 { V3  min, max; };
#if COMPILER_CL
#    pragma warning(pop)
#endif

FUNCDEF inline f32 clamp_axis(f32 radians);                    // Maps angle to range [0, 360).
FUNCDEF inline f32 normalize_axis(f32 radians);                // Maps angle to range (-180, 180].
FUNCDEF inline f32 normalize_half_axis(f32 radians);           // Maps angle to range (-90, 90].
FUNCDEF inline f32 clamp_angle(f32 min, f32 radians, f32 max); // Returns clamped angle in range -180..180.
FUNCDEF inline V3  clamp_euler(V3 euler);
FUNCDEF inline V3  normalize_euler(V3 euler);
FUNCDEF inline V3  normalize_half_euler(V3 euler);

FUNCDEF inline f32 _pow(f32 x, f32 y); // x^y
FUNCDEF inline f32 _mod(f32 x, f32 y); // x%y
FUNCDEF inline f32 _sqrt(f32 x);
FUNCDEF inline f32 _sin(f32 radians);
FUNCDEF inline f32 _cos(f32 radians);
FUNCDEF inline f32 _tan(f32 radians);

FUNCDEF inline f32 _arcsin(f32 x);
FUNCDEF inline f32 _arccos(f32 x);
FUNCDEF inline f32 _arctan(f32 x);
FUNCDEF inline f32 _arctan2(f32 y, f32 x); // Returns angle in range -180..180.
FUNCDEF inline f32 _round(f32 x); // Towards nearest integer.
FUNCDEF inline f32 _floor(f32 x); // Towards negative infinity.
FUNCDEF inline f32 _ceil(f32 x);  // Towards positive infinity.
FUNCDEF inline f32 _sin_turns(f32 turns); // Takes angle in turns.
FUNCDEF inline f32 _cos_turns(f32 turns); // Takes angle in turns.
FUNCDEF inline f32 _tan_turns(f32 turns); // Takes angle in turns.
FUNCDEF inline f32 _arcsin_turns(f32 x);         // Returns angle in turns.
FUNCDEF inline f32 _arccos_turns(f32 x);         // Returns angle in turns.
FUNCDEF inline f32 _arctan_turns(f32 x);         // Returns angle in turns.
FUNCDEF inline f32 _arctan2_turns(f32 y, f32 x); // Returns angle in turns.

FUNCDEF inline V2 pow(V2 v, f32 y);
FUNCDEF inline V3 pow(V3 v, f32 y);
FUNCDEF inline V2 clamp(V2 min, V2 x, V2 max);
FUNCDEF inline V3 clamp(V3 min, V3 x, V3 max);
FUNCDEF inline V2 round(V2 v);
FUNCDEF inline V3 round(V3 v);
FUNCDEF inline V2 floor(V2 v);
FUNCDEF inline V3 floor(V3 v);
FUNCDEF inline V2 ceil(V2 v);
FUNCDEF inline V3 ceil(V3 v);

FUNCDEF inline f32 frac(f32 x); // Output in range [0, 1]
FUNCDEF inline V2  frac(V2 v);
FUNCDEF inline V3  frac(V3 v);

FUNCDEF inline f32 safe_div(f32 x, f32 y, f32 n);
FUNCDEF inline f32 safe_div0(f32 x, f32 y);
FUNCDEF inline f32 safe_div1(f32 x, f32 y);

FUNCDEF inline b32 nearly_zero(f32 x, f32 tolerance = SMALL_NUMBER);

FUNCDEF inline V2 v2(f32 x, f32 y);
FUNCDEF inline V2 v2(f32 c);

FUNCDEF inline V2u v2u(u32 x, u32 y);
FUNCDEF inline V2u v2u(u32 c);

FUNCDEF inline V2s v2s(s32 x, s32 y);
FUNCDEF inline V2s v2s(s32 c);

FUNCDEF inline V3 v3(f32 x, f32 y, f32 z);
FUNCDEF inline V3 v3(f32 c);
FUNCDEF inline V3 v3(V2 xy, f32 z);

FUNCDEF inline V4 v4(f32 x, f32 y, f32 z, f32 w);
FUNCDEF inline V4 v4(f32 c);
FUNCDEF inline V4 v4(V3 xyz, f32 w);
FUNCDEF inline V4 v4(V2 xy, V2 zw);

FUNCDEF inline V4s v4s(s32 c);

FUNCDEF inline V2 hadamard_mul(V2 a, V2 b);
FUNCDEF inline V3 hadamard_mul(V3 a, V3 b);
FUNCDEF inline V4 hadamard_mul(V4 a, V4 b);

FUNCDEF inline V2 hadamard_div(V2 a, V2 b);
FUNCDEF inline V3 hadamard_div(V3 a, V3 b);
FUNCDEF inline V4 hadamard_div(V4 a, V4 b);

FUNCDEF inline f32 dot(V2 a, V2 b);
FUNCDEF inline s32 dot(V2s a, V2s b);
FUNCDEF inline f32 dot(V3 a, V3 b);
FUNCDEF inline f32 dot(V4 a, V4 b);

FUNCDEF inline V2  perp(V2 v); // Counter-Clockwise
FUNCDEF inline V3  cross(V3 a, V3 b);

FUNCDEF inline f32 length2(V2 v);
FUNCDEF inline f32 length2(V3 v);
FUNCDEF inline f32 length2(V4 v);

FUNCDEF inline f32 length(V2 v);
FUNCDEF inline f32 length(V3 v);
FUNCDEF inline f32 length(V4 v);

FUNCDEF inline V2 min_v2(V2 a, V2 b);
FUNCDEF inline V3 min_v3(V3 a, V3 b);
FUNCDEF inline V4 min_v4(V4 a, V4 b);

FUNCDEF inline V2 max_v2(V2 a, V2 b);
FUNCDEF inline V3 max_v3(V3 a, V3 b);
FUNCDEF inline V4 max_v4(V4 a, V4 b);

FUNCDEF inline V2 normalize(V2 v);
FUNCDEF inline V3 normalize(V3 v);
FUNCDEF inline V4 normalize(V4 v);

FUNCDEF inline V2 normalize_or_zero(V2 v);
FUNCDEF inline V3 normalize_or_zero(V3 v);
FUNCDEF inline V4 normalize_or_zero(V4 v);

FUNCDEF inline V3 normalize_or_x_axis(V3 v);
FUNCDEF inline V3 normalize_or_y_axis(V3 v);
FUNCDEF inline V3 normalize_or_z_axis(V3 v);

FUNCDEF inline V3 reflect(V3 incident, V3 normal);

FUNCDEF inline f32        dot(Quaternion a, Quaternion b);
FUNCDEF inline f32        length(Quaternion q);
FUNCDEF inline Quaternion normalize(Quaternion q);
FUNCDEF inline Quaternion normalize_or_identity(Quaternion q);

FUNCDEF inline Quaternion quaternion(f32 x, f32 y, f32 z, f32 w);
FUNCDEF inline Quaternion quaternion(V3 v, f32 w);
FUNCDEF inline Quaternion quaternion_identity();

FUNCDEF inline V3         get_euler(Quaternion q); // Returns euler angles in range (-180, 180].
FUNCDEF inline Quaternion quaternion_from_euler(V3 euler);
FUNCDEF        Quaternion quaternion_from_m3x3(M3x3 const &affine);
FUNCDEF inline Quaternion quaternion_from_m4x4(M4x4 const &affine);
FUNCDEF inline Quaternion quaternion_look_at(V3 direction, V3 up);
FUNCDEF inline Quaternion quaternion_from_axis_angle(V3 axis, f32 radians);
FUNCDEF inline Quaternion quaternion_from_axis_angle_turns(V3 axis, f32 turns); // Rotation around _axis_ by _angle_ turns.

FUNCDEF        Quaternion quaternion_from_two_vectors_helper(V3 a, V3 b, f32 len_ab);
FUNCDEF inline Quaternion quaternion_from_two_vectors(V3 a, V3 b);
FUNCDEF inline Quaternion quaternion_from_two_normals(V3 a, V3 b);

FUNCDEF inline Quaternion quaternion_conjugate(Quaternion q);
FUNCDEF inline Quaternion quaternion_inverse(Quaternion q);
FUNCDEF inline V3         quaternion_get_axis(Quaternion q);
FUNCDEF inline f32        quaternion_get_angle(Quaternion q);
FUNCDEF inline f32        quaternion_get_angle_turns(Quaternion q);

FUNCDEF inline M3x3 m3x3_identity();
FUNCDEF inline M3x3 m3x3(M4x4 const &m);
FUNCDEF        M3x3 m3x3_from_quaternion(Quaternion q);

FUNCDEF inline M3x3 get_rotation(M3x3 const &m);
FUNCDEF inline V3   get_scale(M3x3 const &m);

FUNCDEF inline M3x3 transpose(M3x3 const &m);
FUNCDEF inline V3   transform(M3x3 const &m, V3 v);

FUNCDEF inline M4x4 m4x4_identity();
FUNCDEF inline void m4x4_set_upper(M4x4 *m, M3x3 const &upper);
FUNCDEF inline M4x4 m4x4_from_quaternion(Quaternion q);
FUNCDEF        M4x4 m4x4_from_axis_angle(V3 axis, f32 radians);
FUNCDEF inline M4x4 m4x4_from_axis_angle_turns(V3 axis, f32 turns);
FUNCDEF inline M4x4 m4x4_from_translation_rotation_scale(V3 t, Quaternion r, V3 s);

FUNCDEF inline V4   get_column(M4x4 const &m, u32 c);
FUNCDEF inline V4   get_row(M4x4 const &m, u32 r);
FUNCDEF inline V3   get_x_axis(M4x4 const &affine);
FUNCDEF inline V3   get_y_axis(M4x4 const &affine);
FUNCDEF inline V3   get_z_axis(M4x4 const &affine);
FUNCDEF inline V3   get_forward(M4x4 const &affine);
FUNCDEF inline V3   get_right(M4x4 const &affine);
FUNCDEF inline V3   get_up(M4x4 const &affine);
FUNCDEF inline V3   get_translation(M4x4 const &affine);
FUNCDEF inline M4x4 get_rotation(M4x4 const &affine);
FUNCDEF inline V3   get_scale(M4x4 const &affine);

FUNCDEF inline M4x4 transpose(M4x4 const &m);
FUNCDEF        b32  invert(M4x4 const &m, M4x4 *result);

FUNCDEF inline V4   transform(M4x4 const &m, V4 v);
FUNCDEF inline V3   transform_point(M4x4 const &m, V3 p);
FUNCDEF inline V3   transform_vector(M4x4 const &m, V3 v);


// Linear interpolation. Returns value between a and b based on fraction t.
FUNCDEF inline f32 lerp(f32 a, f32 t, f32 b);
FUNCDEF inline V2  lerp(V2 a, f32 t, V2 b);
FUNCDEF inline V3  lerp(V3 a, f32 t, V3 b);
FUNCDEF inline V4  lerp(V4 a, f32 t, V4 b);

// Inverse of lerp. Returns fraction t, based on a value between a and b.
FUNCDEF inline f32 unlerp(f32 a, f32 value, f32 b);
FUNCDEF inline f32 unlerp(V2 a, V2 v, V2 b);
FUNCDEF inline f32 unlerp(V3 a, V3 v, V3 b);
FUNCDEF inline f32 unlerp(V4 a, V4 v, V4 b);

// Remaps value from input range to output range.
FUNCDEF inline f32 remap(f32 value, f32 imin, f32 imax, f32 omin, f32 omax);
FUNCDEF inline V2  remap(V2 value, V2 imin, V2 imax, V2 omin, V2 omax);
FUNCDEF inline V3  remap(V3 value, V3 imin, V3 imax, V3 omin, V3 omax);
FUNCDEF inline V4  remap(V4 value, V4 imin, V4 imax, V4 omin, V4 omax);

FUNCDEF inline Quaternion   lerp(Quaternion a, f32 t, Quaternion b);
FUNCDEF inline f32        unlerp(Quaternion a, Quaternion q, Quaternion b);
FUNCDEF inline Quaternion  nlerp(Quaternion a, f32 t, Quaternion b);
FUNCDEF        Quaternion  slerp(Quaternion a, f32 t, Quaternion b);

FUNCDEF inline f32 smooth_step  (f32 edge0, f32 x, f32 edge1); // Output in range [0, 1]
FUNCDEF inline f32 smoother_step(f32 edge0, f32 x, f32 edge1); // Output in range [0, 1]

FUNCDEF        f32 move_towards(f32 current, f32 target, f32 delta_time, f32 speed);
FUNCDEF        V2  move_towards(V2 current, V2 target, f32 delta_time, f32 speed);
FUNCDEF        V3  move_towards(V3 current, V3 target, f32 delta_time, f32 speed);
FUNCDEF inline V2  rotate_point_around_pivot(V2 point, V2 pivot, Quaternion q);
FUNCDEF inline V3  rotate_point_around_pivot(V3 point, V3 pivot, Quaternion q);

FUNCDEF inline Range range(f32 min, f32 max);
FUNCDEF inline Rect2 rect2(V2  min, V2  max);
FUNCDEF inline Rect2 rect2(f32 x0, f32 y0, f32 x1, f32 y1);
FUNCDEF inline Rect3 rect3(V3  min, V3  max);
FUNCDEF inline Rect3 rect3(f32 x0, f32 y0, f32 z0, f32 x1, f32 y1, f32 z1);

FUNCDEF inline V2 get_center(Rect2 rect);
FUNCDEF inline V3 get_center(Rect3 rect);

FUNCDEF inline V2 get_size(Rect2 rect);
FUNCDEF inline V3 get_size(Rect3 rect);

FUNCDEF inline f32 get_width(Rect2 rect);
FUNCDEF inline f32 get_height(Rect2 rect);

FUNCDEF inline f32 repeat(f32 x, f32 max_y);
FUNCDEF inline f32 ping_pong(f32 x, f32 max_y);

FUNCDEF inline V3 bezier2(V3 p0, V3 p1, V3 p2, f32 t);

FUNCDEF V4 hsv(f32 h, f32 s, f32 v);

FUNCDEF M4x4_Inverse look_at(V3 pos, V3 at, V3 up);
FUNCDEF M4x4_Inverse perspective(f32 fov_radians, f32 aspect, f32 n, f32 f);   // Right-handed; NDC z in range [0, 1]
FUNCDEF M4x4_Inverse infinite_perspective(f32 fov_radians, f32 aspect, f32 n); // Read notes in definition.
FUNCDEF M4x4_Inverse orthographic_3d(f32 left, f32 right, f32 bottom, f32 top, f32 n, f32 f, b32 normalized_z);
FUNCDEF M4x4_Inverse orthographic_2d(f32 left, f32 right, f32 bottom, f32 top);

FUNCDEF void calculate_tangents(V3 normal, V3 *tangent_out, V3 *bitangent_out);

////////////////////////////////
//~
// Operator overloading
//
#if COMPILER_GCC
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wattributes"
#    pragma GCC diagnostic ignored "-Wmissing-braces"
#elif COMPILER_CLANG
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wattributes"
#    pragma clang diagnostic ignored "-Wmissing-braces"
#endif

inline V2 operator+(V2 v)         
{ 
    V2 result = v;
    return result;
}
inline V2 operator-(V2 v)         
{ 
    V2 result = {-v.x, -v.y};
    return result;
}
inline V2 operator+(V2 a, V2 b)   
{ 
    V2 result = {a.x+b.x, a.y+b.y};
    return result;
}
inline V2 operator-(V2 a, V2 b)   
{ 
    V2 result = {a.x-b.x, a.y-b.y};
    return result;
}
inline V2 operator*(V2 v, f32 s)  
{ 
    V2 result = {v.x*s, v.y*s};
    return result;
}
inline V2 operator*(f32 s, V2 v)  
{ 
    V2 result = operator*(v, s);
    return result;
}
inline V2 operator*(V2 a, V2 b)  
{ 
    V2 result = hadamard_mul(a, b);
    return result;
}
inline V2 operator/(V2 v, f32 s)  
{ 
    V2 result = {v.x/s, v.y/s};
    return result;
}
inline V2& operator+=(V2 &a, V2 b) 
{ 
    a = a + b;
    return a;
}
inline V2& operator-=(V2 &a, V2 b) 
{ 
    a = a - b;
    return a;
}
inline V2& operator*=(V2 &v, f32 s)
{ 
    v = v * s;
    return v;
}
inline V2& operator/=(V2 &v, f32 s)
{ 
    v = v / s;
    return v;
}

inline V2s operator+(V2s v)         
{ 
    V2s result = v;
    return result;
}
inline V2s operator-(V2s v)         
{ 
    V2s result = {-v.x, -v.y};
    return result;
}
inline V2s operator+(V2s a, V2s b)   
{ 
    V2s result = {a.x+b.x, a.y+b.y};
    return result;
}
inline V2s operator-(V2s a, V2s b)   
{ 
    V2s result = {a.x-b.x, a.y-b.y};
    return result;
}
inline V2s operator*(V2s v, s32 s)  
{ 
    V2s result = {v.x*s, v.y*s};
    return result;
}
inline V2s operator*(s32 s, V2s v)  
{ 
    V2s result = operator*(v, s);
    return result;
}
inline V2s& operator+=(V2s &a, V2s b) 
{ 
    a = a + b;
    return a;
}
inline V2s& operator-=(V2s &a, V2s b) 
{ 
    a = a - b;
    return a;
}
inline b32 operator==(V2s a, V2s b)
{
    b32 result = ((a.x == b.x) && (a.y == b.y));
    return result;
}
inline b32 operator!=(V2s a, V2s b)
{
    b32 result = !(a == b);
    return result;
}

inline b32 operator==(V2u a, V2u b)
{
    b32 result = ((a.x == b.x) && (a.y == b.y));
    return result;
}
inline b32 operator!=(V2u a, V2u b)
{
    b32 result = !(a == b);
    return result;
}

inline V3 operator+(V3 v)         
{ 
    V3 result = v;
    return result;
}
inline V3 operator-(V3 v)         
{ 
    V3 result = {-v.x, -v.y, -v.z};
    return result;
}
inline V3 operator+(V3 a, V3 b)   
{ 
    V3 result = {a.x+b.x, a.y+b.y, a.z+b.z};
    return result;
}
inline V3 operator-(V3 a, V3 b)   
{ 
    V3 result = {a.x-b.x, a.y-b.y, a.z-b.z};
    return result;
}
inline V3 operator*(V3 v, f32 s)  
{ 
    V3 result = {v.x*s, v.y*s, v.z*s};
    return result;
}
inline V3 operator*(f32 s, V3 v)  
{ 
    V3 result = operator*(v, s);
    return result;
}
inline V3 operator*(V3 a, V3 b)  
{ 
    V3 result = hadamard_mul(a, b);
    return result;
}
inline V3 operator/(V3 a, V3 b)  
{ 
    V3 result = hadamard_div(a, b);
    return result;
}
inline V3& operator*=(V3 &a, V3 b)
{ 
    a = a * b;
    return a;
}
inline V3 operator/(V3 v, f32 s)  
{ 
    V3 result = {v.x/s, v.y/s, v.z/s};
    return result;
}
inline b32 operator==(V3 a, V3 b)
{
    b32 result = (length2(a - b) < 9.99999944E-11f);
    return result;
}
inline b32 operator!=(V3 a, V3 b)
{
    b32 result = !(a == b);
    return result;
}
inline V3& operator+=(V3 &a, V3 b) 
{ 
    a = a + b;
    return a;
}
inline V3& operator-=(V3 &a, V3 b) 
{ 
    a = a - b;
    return a;
}
inline V3& operator*=(V3 &v, f32 s)
{ 
    v = v * s;
    return v;
}
inline V3& operator/=(V3 &v, f32 s)
{ 
    v = v / s;
    return v;
}

inline V4 operator+(V4 v)         
{ 
    V4 result = v;
    return result;
}
inline V4 operator-(V4 v)         
{ 
    V4 result = {-v.x, -v.y, -v.z, -v.w};
    return result;
}
inline V4 operator+(V4 a, V4 b)   
{ 
    V4 result = {a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w};
    return result;
}
inline V4 operator-(V4 a, V4 b)   
{ 
    V4 result = {a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w};
    return result;
}
inline V4 operator*(V4 v, f32 s)  
{ 
    V4 result = {v.x*s, v.y*s, v.z*s, v.w*s};
    return result;
}
inline V4 operator*(f32 s, V4 v)  
{ 
    V4 result = operator*(v, s);
    return result;
}
inline V4 operator/(V4 v, f32 s)  
{ 
    V4 result = {v.x/s, v.y/s, v.z/s, v.w/s};
    return result;
}
inline V4& operator+=(V4 &a, V4 b) 
{ 
    a = a + b;
    return a;
}
inline V4& operator-=(V4 &a, V4 b) 
{ 
    a = a - b;
    return a;
}
inline V4& operator*=(V4 &v, f32 s)
{ 
    v = v * s;
    return v;
}
inline V4& operator/=(V4 &v, f32 s)
{ 
    v = v / s;
    return v;
}

inline Quaternion operator+(Quaternion v)                
{ 
    Quaternion result = v;
    return result;
}
inline Quaternion operator-(Quaternion v)                
{ 
    Quaternion result = {-v.x, -v.y, -v.z, -v.w};
    return result;
}
inline Quaternion operator+(Quaternion a, Quaternion b)  
{ 
    Quaternion result = {a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w};
    return result;
}
inline Quaternion operator-(Quaternion a, Quaternion b)  
{ 
    Quaternion result = {a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w};
    return result;
}
inline Quaternion operator*(Quaternion v, f32 s)         
{ 
    Quaternion result = {v.x*s, v.y*s, v.z*s, v.w*s};
    return result;
}
inline Quaternion operator*(f32 s, Quaternion v)         
{ 
    Quaternion result = operator*(v, s);
    return result;
}
inline Quaternion operator/(Quaternion v, f32 s)         
{ 
    Quaternion result = operator*(v, 1.0f/s);
    return result;
}
inline Quaternion& operator+=(Quaternion &a, Quaternion b)
{ 
    a = a + b;
    return a;
}
inline Quaternion& operator-=(Quaternion &a, Quaternion b)
{ 
    a = a - b;
    return a;
}
inline Quaternion& operator*=(Quaternion &v, f32 s)       
{ 
    v = v * s;
    return v;
}
inline Quaternion& operator/=(Quaternion &v, f32 s)       
{ 
    v = v / s;
    return v;
}
inline Quaternion operator*(Quaternion a, Quaternion b)
{ 
    Quaternion result = 
    {
        a.w*b.v + b.w*a.v + cross(a.v, b.v),
        a.w*b.w - dot(a.v, b.v)
    };
    return result; 
}
inline V3 operator*(Quaternion q, V3 v)
{
    // Rotating vector `v` by quaternion `q`:
    // Formula by the optimization expert Fabian "ryg" Giesen, check his blog:
    // https://fgiesen.wordpress.com/2019/02/09/rotating-a-single-vector-using-a-quaternion/
    
    // Canonical formula for rotating v by q (slower):
    // result = q * quaternion(v, 0) * quaternion_conjugate(q);
    
    // Fabian's method (faster):
    V3 t      = 2 * cross(q.v, v);
    V3 result = v + q.w*t + cross(q.v, t);
    return result;
}

inline V3 operator*(M3x3 m, V3 v)
{ 
    V3 result = transform(m, v);
    return result;
}

inline V4 operator*(M4x4 m, V4 v)
{ 
    V4 result = transform(m, v);
    return result;
}
inline M4x4 operator*(M4x4 a, M4x4 b)
{
    M4x4 result = {};
    for(s32 r = 0; r < 4; r++)
    {for(s32 c = 0; c < 4; c++)
        {for(s32 i = 0; i < 4; i++)
            {result.II[r][c] += a.II[r][i] * b.II[i][c];}}}
    return result;
}

#if COMPILER_GCC
#    pragma GCC diagnostic pop
#elif COMPILER_CLANG
#    pragma clang diagnostic pop
#endif

/////////////////////////////////////////
//~
// PRNG
//
struct Random_PCG
{
    u64 state;
};
FUNCDEF inline Random_PCG random_seed(u64 seed = 78953890);
FUNCDEF inline u32 random_next(Random_PCG *rng);
FUNCDEF inline u64 random_next64(Random_PCG *rng);
FUNCDEF inline f32 random_nextf(Random_PCG *rng);         // [0, 1) interval
FUNCDEF inline f64 random_nextd(Random_PCG *rng);         // [0, 1) interval
FUNCDEF inline u32 random_range(Random_PCG *rng, u32 min, u32 max);  // [min, max) interval.
FUNCDEF inline f32 random_rangef(Random_PCG *rng, f32 min, f32 max); // [min, max) interval.
FUNCDEF inline V2  random_range_v2(Random_PCG *rng, V2 min, V2 max); // [min, max) interval.
FUNCDEF inline V3  random_range_v3(Random_PCG *rng, V3 min, V3 max); // [min, max) interval.


/////////////////////////////////////////
//~
// Memory Arena
//
#define KILOBYTES(x)   (         (x) * 1024LL)
#define MEGABYTES(x)   (KILOBYTES(x) * 1024LL)
#define GIGABYTES(x)   (MEGABYTES(x) * 1024LL)
#define TERABYTES(x)   (GIGABYTES(x) * 1024LL)

#define ALIGN_UP(x, pow2)   (((x) + ((pow2) - 1)) & ~((pow2) - 1))
#define ALIGN_DOWN(x, pow2) ( (x)                 & ~((pow2) - 1))

#define PUSH_STRUCT(arena, T)          ((T *) arena_push     (arena, sizeof(T),       alignof(T)))
#define PUSH_STRUCT_ZERO(arena, T)     ((T *) arena_push_zero(arena, sizeof(T),       alignof(T)))
#define PUSH_ARRAY(arena, T, c)        ((T *) arena_push     (arena, sizeof(T) * (c), alignof(T)))
#define PUSH_ARRAY_ZERO(arena, T, c)   ((T *) arena_push_zero(arena, sizeof(T) * (c), alignof(T)))

#define MEMORY_SET(m, v, z)            memset((m), (v), (z))
#define MEMORY_ZERO(m, z)              MEMORY_SET((m), 0, (z))
#define MEMORY_ZERO_STRUCT(s)          MEMORY_ZERO((s), sizeof(*(s)))
#define MEMORY_ZERO_ARRAY(a)           MEMORY_ZERO((a), sizeof(a))

#define MEMORY_COPY(d, s, z)           memmove((d), (s), (z))
#define MEMORY_COPY_STRUCT(d, s)       MEMORY_COPY((d), (s), MIN(sizeof(*(d)), sizeof(*(s))))

#define ARENA_MAX_DEFAULT GIGABYTES(8)
#define ARENA_COMMIT_SIZE KILOBYTES(4)

#define ARENA_SCRATCH_COUNT 2

struct Arena
{
    u64 max;
    u64 used;
    u64 commit_used;
};

struct Arena_Temp
{
    Arena *arena;
    u64    used;
};

FUNCDEF        Arena*      arena_init(u64 max_size = ARENA_MAX_DEFAULT);
FUNCDEF inline void        arena_free(Arena *arena);
FUNCDEF        void*       arena_push(Arena *arena, u64 size, u64 alignment);
FUNCDEF inline void*       arena_push_zero(Arena *arena, u64 size, u64 alignment);
FUNCDEF inline void        arena_pop(Arena *arena, u64 size);
FUNCDEF inline void        arena_reset(Arena *arena);
FUNCDEF inline Arena_Temp  arena_temp_begin(Arena *arena);
FUNCDEF inline void        arena_temp_end(Arena_Temp temp);
FUNCDEF        Arena_Temp  get_scratch(Arena **conflict_array, s32 count);
#define free_scratch(temp) arena_temp_end(temp)

/////////////////////////////////////////
//~
// String8
//
#define S8LIT(literal) str8((u8 *) literal, ARRAY_COUNT(literal) - 1)
#define S8ZERO         str8(0, 0)

// @Note: Allocation size is count+1 to account for null terminator.
struct String8
{
    u8 *data;
    u64 count;
    
    inline u8& operator[](s64 index)
    {
        ASSERT(index < (s64)count);
        return data[index];
    }
};

FUNCDEF String8    str8(u8 *data, u64 count);
FUNCDEF String8    str8_cstring(const char *c_string);
FUNCDEF u64        str8_length(const char *c_string);
FUNCDEF String8    str8_copy(Arena *arena, String8 s);
FUNCDEF String8    str8_cat(Arena *arena, String8 a, String8 b);
FUNCDEF inline b32 str8_empty(String8 s);
FUNCDEF b32        str8_match(String8 a, String8 b);

inline b32 operator==(String8 lhs, String8 rhs)
{
    return str8_match(lhs, rhs);
}
inline b32 operator!=(String8 lhs, String8 rhs)
{
    return !(lhs == rhs);
}

/////////////////////////////////////////
//~
// String Helper Functions
//

//~ String ctor helpers
// @Note: Assumes passed strings are immutable! We won't allocate memory for the result string!
FUNCDEF String8 str8_skip(String8 str, u64 amount);

//~ String character helpers
FUNCDEF inline b32  is_spacing(char c);
FUNCDEF inline b32  is_end_of_line(char c);
FUNCDEF inline b32  is_whitespace(char c);
FUNCDEF inline b32  is_alpha(char c);
FUNCDEF inline b32  is_numeric(char c);
FUNCDEF inline b32  is_alphanumeric(char c);
FUNCDEF inline b32  is_file_slash(char c);

//~ String path helpers
// @Note: Assumes passed strings are immutable! We won't allocate memory for the result string!
FUNCDEF String8 chop_extension(String8 path);
FUNCDEF String8 extract_file_name(String8 path); // Includes extension.
FUNCDEF String8 extract_base_name(String8 path); // Doesn't include extension.
FUNCDEF String8 extract_parent_folder(String8 path);

//~ String misc helpers
FUNCDEF inline void advance(String8 *s, u64 count);
FUNCDEF inline void get(String8 *s, void *data, u64 size);
FUNCDEF        u32  get_hash(String8 s);

template<typename T>
void get(String8 *s, T *value)
{
    ASSERT(sizeof(T) <= s->count);
    
    MEMORY_COPY(value, s->data, sizeof(T));
    advance(s, sizeof(T));
}

/////////////////////////////////////////
//~
// String Format
//
#include <stdarg.h>

FUNCDEF void    put_char(String8 *dest, char c);
FUNCDEF void    put_c_string(String8 *dest, const char *c_string);
FUNCDEF void    u64_to_ascii(String8 *dest, u64 value, u32 base, char *digits);
FUNCDEF void    f64_to_ascii(String8 *dest, f64 value, u32 precision);
FUNCDEF u64     ascii_to_u64(char **at);
FUNCDEF u64     string_format_list(char *dest_start, u64 dest_count, const char *format, va_list arg_list);
FUNCDEF u64     string_format(char *dest_start, u64 dest_count, const char *format, ...);
FUNCDEF void    debug_print(const char *format, ...);
FUNCDEF String8 sprint(Arena *arena, const char *format, ...);

/////////////////////////////////////////
//~
// String Builder
//
#define SB_BLOCK_SIZE KILOBYTES(4)

struct String_Builder
{
    Arena *arena;
    String8 buffer;
    
    u8 *start;
    u64 length;
    u64 capacity;
};

FUNCDEF String_Builder sb_init(u64 capacity = SB_BLOCK_SIZE);
FUNCDEF void           sb_free(String_Builder *builder);
FUNCDEF void           sb_reset(String_Builder *builder);
FUNCDEF void           sb_append(String_Builder *builder, void *data, u64 size);
FUNCDEF void           sb_appendf(String_Builder *builder, char *format, ...);
FUNCDEF String8        sb_to_string(String_Builder *builder, Arena *arena);
template<typename T>
void sb_append(String_Builder *builder, T *data)
{
    sb_append(builder, data, sizeof(T));
}

/////////////////////////////////////////
//~
// Dynamic Array
//
#define ARRAY_SIZE_MIN 32
template<typename T>
struct Array
{
    Arena *arena;
    T     *data;
    s64    count;
    s64    capacity;
    
    inline T& operator[](s64 index)
    {
        ASSERT((index >= 0) && (index < count));
        return data[index];
    }
    inline const T& operator[](s64 index) const
    {
        ASSERT((index >= 0) && (index < count));
        return data[index];
    }
};

template<typename T>
void array_init(Array<T> *array)
{
    array->arena     = arena_init();
    array->data      = 0;
    array->count     = 0;
    array->capacity  = 0;
}

template<typename T>
void array_init_and_reserve(Array<T> *array, s64 desired_items)
{
    array_init(array);
    array_reserve(array, desired_items);
}

template<typename T>
void array_init_and_resize(Array<T> *array, s64 desired_items)
{
    array_init(array);
    array_resize(array, desired_items);
}

template<typename T>
void array_free(Array<T> *array)
{
    arena_free(array->arena);
}

template<typename T>
void array_reserve(Array<T> *array, s64 desired_items)
{
    if (desired_items <= array->capacity) 
        return;
    
    array->capacity = desired_items;
    
    arena_reset(array->arena);
    array->data = PUSH_ARRAY_ZERO(array->arena, T, desired_items);
}

template<typename T>
void array_resize(Array<T> *array, s64 size)
{
    // @Note: Use this function to reserve size upfront and fill items using array[index];
    array_reserve(array, size);
    array->count = size;
}

template<typename T>
void array_copy(Array<T> *dst, Array<T> src)
{
    if (src.count > dst->capacity)
        array_reserve(dst, src.count);
    
    dst->count = src.count;
    MEMORY_COPY(dst->data, src.data, src.count * sizeof(T));
}

template<typename T>
void array_expand(Array<T> *array)
{
    s64 new_size = array->capacity * 2;
    if (new_size < ARRAY_SIZE_MIN) new_size = ARRAY_SIZE_MIN;
    
    Array<T> old_array;
    array_init_and_reserve(&old_array, array->count);
    array_copy(&old_array, *array);
    
    array_reserve(array, new_size);
    
    // Copy data.
    array_copy(array, old_array);
    
    array_free(&old_array);
}

template<typename T>
void array_reset(Array<T> *array)
{
    array->count = 0;
}

template<typename T>
T* array_add(Array<T> *array, T item)
{
    if (array->count >= array->capacity)
        array_expand(array);
    
    array->data[array->count] = item;
    array->count++;
    
    return &array->data[array->count-1];
}

template<typename T>
void array_remove_range(Array<T> *array, s32 start_index, s32 end_index)
{
    if (array->count <= 0)
        return;
    
    ASSERT((start_index >= 0) && (end_index < array->count) && (start_index <= end_index));
    
    s32 num_elements = end_index - start_index + 1;
    for (s32 i = start_index; i < end_index+1; i++)
        array->data[i] = array->data[i+num_elements];
    array->count -= num_elements;
}

template<typename T>
void array_unordered_remove_by_index(Array<T> *array, s32 index)
{
    ASSERT((index >= 0) && (index < array->count));
    
    array->data[index] = array->data[array->count-1];
    array->count--;
}

template<typename T>
void array_ordered_remove_by_index(Array<T> *array, s32 index)
{
    ASSERT((index >= 0) && (index < array->count));
    
    for (s32 i = index; i < array->count-1; i++)
        array->data[i] = array->data[i+1];
    array->count--;
}

template<typename T>
T array_pop_front(Array<T> *array)
{
    if (array->count <= 0) {
        T dummy = {};
        return dummy;
    }
    
    T result = array->data[0];
    array_ordered_remove_by_index(array, 0);
    
    return result;
}

template<typename T>
T array_pop_back(Array<T> *array)
{
    if (array->count <= 0) {
        T dummy = {};
        return dummy;
    }
    
    T result = array->data[array->count-1];
    array->count--;
    
    return result;
}

template<typename T>
s32 array_find_index(Array<T> *array, T item)
{
    s32 result = -1;
    
    for (s32 index = 0; index < array->count; index++) {
        if (array->data[index] == item) {
            result = index;
            break;
        }
    }
    
    return result;
}

template<typename T>
void array_add_unique(Array<T> *array, T item)
{
    s32 find_index = array_find_index(array, item);
    if (find_index < 0) {
        array_add(array, item);
    }
}

/////////////////////////////////////////
//~
// Hash Table
//

// @Note: I don't like this... this header is needed to make generic hashing work.
//
#include <typeinfo> 

#define TABLE_SIZE_MIN 32
template<typename Key_Type, typename Value_Type>
struct Table
{
    s64 count;
    s64 capacity;
    
    Array<Key_Type>   keys;
    Array<Value_Type> values;
    Array<b32>        occupancy_mask;
    Array<u32>        hashes;
};

template<typename K, typename V>
void table_init(Table<K, V> *table, s64 size = 0)
{
    table->count    = 0;
    table->capacity = size;
    
    array_init_and_resize(&table->keys,           size);
    array_init_and_resize(&table->values,         size);
    array_init_and_resize(&table->occupancy_mask, size);
    array_init_and_resize(&table->hashes,         size);
}

template<typename K, typename V>
void table_free(Table<K, V> *table)
{
    array_free(&table->keys);
    array_free(&table->values);
    array_free(&table->occupancy_mask);
    array_free(&table->hashes);
}

template<typename K, typename V>
void table_reset(Table<K, V> *table)
{
    table->count = 0;
    MEMORY_ZERO(table->occupancy_mask.data, table->occupancy_mask.capacity * sizeof(table->occupancy_mask[0]));
}

template<typename K, typename V>
void table_expand(Table<K, V> *table)
{
    auto old_keys   = table->keys;
    auto old_values = table->values;
    auto old_mask   = table->occupancy_mask;
    auto old_hashes = table->hashes;
    
    s64 new_size    = table->capacity * 2;
    if (new_size < TABLE_SIZE_MIN) new_size = TABLE_SIZE_MIN;
    
    table_init(table, new_size);
    
    // Copy old data.
    for (s32 i = 0; i < old_mask.capacity; i++) {
        if (old_mask[i])
            table_add(table, old_keys[i], old_values[i]);
    }
    
    array_free(&old_keys);
    array_free(&old_values);
    array_free(&old_mask);
    array_free(&old_hashes);
}

template<typename K, typename V>
V* table_add(Table<K, V> *table, K key, V value)
{
    if ((table->count * 2) >= table->capacity)
        table_expand(table);
    
    ASSERT(table->count <= table->capacity);
    
    // @Note: Some structs need padding; compiler will set padding bytes to arbitrary values. 
    // If we run into issues, we should manually add padding bytes for these types or use some pragmas.
    //
    String8 s = {};
    if (typeid(K) == typeid(String8)) { s = *(String8*)&key; }
    else                              { s = str8((u8*)&key, sizeof(K)); }
    
    u32 hash  = get_hash(s);
    s32 index = hash % table->capacity;
    
    // Linear probing.
    while (table->occupancy_mask[index]) {
        // @Note: Key_Types need to to have valid operator==().
        //
        if ((table->hashes[index] == hash) && (table->keys[index] == key)) {
            ASSERT(!"The passed key is already in use; Override is not allowed!");
        }
        
        index++;
        if (index >= table->capacity) index = 0;
    }
    
    table->count++;
    table->keys[index]           = key;
    table->values[index]         = value;
    table->occupancy_mask[index] = TRUE;
    table->hashes[index]         = hash;
    
    return &table->values[index];
}

template<typename K, typename V>
V table_find(Table<K, V> *table, K key, b32 *found = 0)
{
    // @Todo: Return b32 and pass return value as parameter?
    
    if (!table->capacity) {
        V dummy = {};
        if (found)
            *found = FALSE;
        return dummy;
    }
    
    // @Note: Some structs need padding; compiler will set padding bytes to arbitrary values. 
    // If we run into issues, we should manually add padding bytes for these types.
    //
    String8 s = {};
    if (typeid(K) == typeid(String8)) { s = *(String8*)&key; }
    else                              { s = str8((u8*)&key, sizeof(K)); }
    
    u32 hash  = get_hash(s);
    s32 index = hash % table->capacity;
    
    while (table->occupancy_mask[index]) {
        // @Note: Key_Types need to to have valid operator==().
        //
        if ((table->hashes[index] == hash) &&  (table->keys[index] == key)) {
            if (found)
                *found = TRUE;
            return table->values[index];
        }
        
        index++;
        if (index >= table->capacity) index = 0;
    }
    
    V dummy = {};
    if (found)
        *found = FALSE;
    return dummy;
}

template<typename K, typename V>
V* table_find_pointer(Table<K, V> *table, K key, b32 *found = 0)
{
    // @Note: Almost same as table_find, except it returns pointer to value in the table.
    
    if (!table->capacity) {
        if (found)
            *found = FALSE;
        return 0;
    }
    
    // @Note: Some structs need padding; compiler will set padding bytes to arbitrary values. 
    // If we run into issues, we should manually add padding bytes for these types.
    //
    String8 s = {};
    if (typeid(K) == typeid(String8)) { s = *(String8*)&key; }
    else                              { s = str8((u8*)&key, sizeof(K)); }
    
    u32 hash  = get_hash(s);
    s32 index = hash % table->capacity;
    
    while (table->occupancy_mask[index]) {
        // @Note: Key_Types need to to have valid operator==().
        //
        if ((table->hashes[index] == hash) &&  (table->keys[index] == key)) {
            if (found)
                *found = TRUE;
            return &table->values[index];
        }
        
        index++;
        if (index >= table->capacity) index = 0;
    }
    
    if (found)
        *found = FALSE;
    return 0;
}

/////////////////////////////////////////
//~
// Sound
//
struct Sound
{
    s16 *samples; // Mono 16-bit.
    u32  count;
    u32  pos;     // 0 to play from start, "count" to stop (if not looping).
    
    f32  volume;  // In range [0, 1].
    b32  loop;
};

FUNCDEF void sound_update(Sound *sound, u32 samples_to_advance);
FUNCDEF void sound_mix(const Sound *sound, f32 volume, f32 *samples_out, u32 samples_to_write);

/////////////////////////////////////////
//~
// OS
//

// Configs.
//
// Based on m_yaw/m_pitch in source engine; use for FPS games.
#define BASE_MOUSE_RESOLUTION_SOURCE (0.022 * DEGS_TO_RADS)
// Based on Unreal Engine's mouse sensitivity; use for third-person games.
#define BASE_MOUSE_RESOLUTION_UNREAL (0.070 * DEGS_TO_RADS)

// Keyboards keys and mouse buttons.
//
enum Key
{
    Key_NONE,
    
    Key_F1,
    Key_F2,
    Key_F3,
    Key_F4,
    Key_F5,
    Key_F6,
    Key_F7,
    Key_F8,
    Key_F9,
    Key_F10,
    Key_F11,
    Key_F12,
    
    Key_0,
    Key_1,
    Key_2,
    Key_3,
    Key_4,
    Key_5,
    Key_6,
    Key_7,
    Key_8,
    Key_9,
    
    Key_A,
    Key_B,
    Key_C,
    Key_D,
    Key_E,
    Key_F,
    Key_G,
    Key_H,
    Key_I,
    Key_J,
    Key_K,
    Key_L,
    Key_M,
    Key_N,
    Key_O,
    Key_P,
    Key_Q,
    Key_R,
    Key_S,
    Key_T,
    Key_U,
    Key_V,
    Key_W,
    Key_X,
    Key_Y,
    Key_Z,
    
    Key_ESCAPE,
    Key_BACKSPACE,
    Key_TAB,
    Key_ENTER,
    Key_SHIFT,
    Key_CONTROL,
    Key_ALT,
    Key_SPACE,
    
    // Arrow keys.
    Key_LEFT,
    Key_UP,
    Key_RIGHT,
    Key_DOWN,
    
    // Mouse buttons.
    Key_MLEFT,
    Key_MRIGHT,
    Key_MMIDDLE,
    
    Key_COUNT,
};

struct Input_State
{
    // Key states.
    b32 pressed [Key_COUNT];
    b32 held    [Key_COUNT];
    b32 released[Key_COUNT];
    
    // Delta values since last frame.
    V2s mouse_delta_raw;     // The raw mouse delta as reported by the os.
    V2  mouse_delta;         // The raw mouse delta scaled by os->base_mouse_resolution with y negated.
    s32 mouse_wheel_raw;     // The raw mouse wheel value as reported by os - +1 up, -1 down.
};

struct Queued_Input
{
    s32 key;
    b32 down;
};

enum Gamepad_Button
{
    GamepadButton_NONE,
    
    GamepadButton_DPAD_UP,
    GamepadButton_DPAD_DOWN,
    GamepadButton_DPAD_LEFT,
    GamepadButton_DPAD_RIGHT,
    GamepadButton_START,
    GamepadButton_BACK,
    GamepadButton_LEFT_THUMB,
    GamepadButton_RIGHT_THUMB,
    GamepadButton_LEFT_BUMPER,
    GamepadButton_RIGHT_BUMPER,
    GamepadButton_A,
    GamepadButton_B,
    GamepadButton_X,
    GamepadButton_Y,
    
    GamepadButton_COUNT,
};

#define GAMEPADS_MAX 16
struct Gamepad
{
    b32 connected;
    
    // Analog direction vectors in range [-1, 1].
    V2  left_stick;
    V2  right_stick;
    
    // Analog in range [0, 1].
    f32 left_trigger;
    f32 right_trigger;
    
    // @Todo: Per-tick and per-frame input for gameoad buttons!
    b32 pressed [GamepadButton_COUNT];
    b32 held    [GamepadButton_COUNT];
    b32 released[GamepadButton_COUNT];
};

struct File_Info
{
    File_Info *next;
    
    // @Note: This is a 64-bit number that _means_ the date to the platform, but doesn't have to be understood by the app as any particular date.
    u64 file_date;
    u64 file_size;
    
    String8 full_path;
    String8 base_name; // Doesn't include a path or an extension.
};

struct File_Group
{
    u64        file_count;
    File_Info *first_file_info;
};

enum Cursor_Mode
{
    CursorMode_NORMAL,
    CursorMode_DISABLED, // Hide, capture, and always center the mouse cursor.
    CursorMode_CAPTURED, // Capture/trap the mouse cursor to window rect.
    CursorMode_HIDDEN,   // Hide the mouse cursor.
};

enum Display_Mode
{
    DisplayMode_BORDER,
    DisplayMode_FULLSCREEN, // Borderless.
};

struct OS_State
{
    // Meta-data.
    String8 exe_full_path;
    String8 exe_parent_folder;
    String8 data_folder;
    
    // Arenas.
    Arena *permanent_arena;
    
    // User Input.
    // Keyboard and mouse stuff.
    Array<Queued_Input> inputs_to_process;
    Input_State         tick_input;   // Per-tick key states.
    Input_State         frame_input;  // Per-frame key states.
    V3  mouse_ndc;                    // Mouse position relative to drawing_rect - in [-1, 1] interval.
    
    //
    // Gamepad/controller stuff. Can only care about gamepads[0] for singleplayer.
    Gamepad gamepads[GAMEPADS_MAX];
    
    // Audio Output.
    // These values are constant and initialized in OS layer at startup after initializing audio API.
    u32 sample_rate;      // Typically 48000.
    u32 bytes_per_sample; // Typically 8 bytes (1 sample = 2 floats for stereo).
    //
    // These values must be set each frame by OS layer before asking game to fill sound buffer with samples.
    f32 *samples_out;        // The sound buffer to fill (stereo, interleaved) in range [-1, 1]. 
    u32  samples_to_write;   // Number of samples we want to write. 
    u32  samples_to_advance; // Number of samples that were actually submitted last time (not necessarily all of what we intended, which was samples_to_write). Advance sound playback positions by this amount, i.e. pass to sound_update().
    
    // @Todo: Bit field instead of a bunch of b32s.
    //
    // Options.
    f32          base_mouse_resolution;
    volatile b32 exit;
    Display_Mode display_mode;
    Cursor_Mode  cursor_mode;
    b32 vsync;
    b32 fix_aspect_ratio;
    V2u render_size;         // Determines aspect ratio.
    V2u window_size;         // Client window width and height.
    Rect2 drawing_rect;      // The viewport. In pixel space (min top-left) - relative to client window.
    f32 dt;                  // The timestep - must be fixed!
    s32 fps_max;             // FPS limiter when vsync is off. Set to 0 for unlimited.
    f64 time;                // Incremented by dt at the end of each update.
    
    // Functions.
    void       (*print_to_debug_output)(String8 text);
    void*      (*reserve) (u64 size);
    void       (*release) (void *memory);
    b32        (*commit)  (void *memory, u64 size);
    void       (*decommit)(void *memory, u64 size);
    String8    (*read_entire_file)(String8 full_path);
    b32        (*write_entire_file)(String8 full_path, String8 data);
    void       (*free_file_memory)(void *memory);  // @Redundant: Does same thing as release().
    File_Group (*get_all_files_in_path)(Arena *arena, String8 path_wildcard);
    Sound      (*sound_load)(String8 full_path, u32 sample_rate);
    b32        (*set_display_mode)(Display_Mode mode);
    b32        (*set_cursor_mode)(Cursor_Mode mode);
};
extern OS_State *os;

// Input helper functions. 
FUNCDEF        b32   key_pressed     (Input_State *input, s32 key, b32 capture = FALSE);
FUNCDEF        b32   key_held        (Input_State *input, s32 key, b32 capture = FALSE);
FUNCDEF        b32   key_released    (Input_State *input, s32 key, b32 capture = FALSE);
FUNCDEF inline void  reset_input     (Input_State *input);
FUNCDEF inline void  clear_input     (Input_State *input);

FUNCDEF Rect2 aspect_ratio_fit(V2u render_dim, V2u window_dim);
FUNCDEF V3    unproject(V3 camera_position, f32 Zworld_distance_from_camera, V3 mouse_ndc, M4x4_Inverse world_to_view, M4x4_Inverse view_to_proj);

#endif //ORH_H





#if defined(ORH_IMPLEMENTATION) && !defined(ORH_IMPLEMENTATION_LOCK)
#define ORH_IMPLEMENTATION_LOCK

/////////////////////////////////////////
//~
// Math Implementation
//
#if COMPILER_GCC
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wattributes"
#    pragma GCC diagnostic ignored "-Wmissing-braces"
#elif COMPILER_CLANG
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wattributes"
#    pragma clang diagnostic ignored "-Wmissing-braces"
#endif

const V3 V3_ZERO    = {};
const V3 V3_X_AXIS  = {1.0f, 0.0f, 0.0f};
const V3 V3_Y_AXIS  = {0.0f, 1.0f, 0.0f};
const V3 V3_Z_AXIS  = {0.0f, 0.0f, 1.0f};
const V3 V3_RIGHT   =  V3_X_AXIS;
const V3 V3_UP      =  V3_Y_AXIS;
const V3 V3_FORWARD = -V3_Z_AXIS;

FUNCDEF inline f32 clamp_axis(f32 radians)
{
    // From Unreal Engine:
    // Maps angle to range [0, 360).
    
    // Make angle in range (-360, 360)
    f32 result = _mod(radians, TAU32);
    if (result < 0.0f)
        result += TAU32; // Shift to range [0, 360)
    return result;
}
FUNCDEF inline f32 normalize_axis(f32 radians)
{
    // From Unreal Engine:
    // Maps angle to range (-180, 180].
    
    // Make angle in range [0, 360).
    f32 result = clamp_axis(radians);
    if (result > PI32)
        result -= TAU32; // Shift to range (-180, 180]
    return result;
}
FUNCDEF inline f32 normalize_half_axis(f32 radians)
{
    // From Unreal Engine:
    // Maps angle to range (-90, 90].
    
    // Make angle in range (-180, 180].
    f32 result = normalize_axis(radians);
    if (ABS(result) > (PI32 * 0.5f))
        result += -SIGN(result) * PI32; // Shift to range (-90, 90]
    return result;
}
FUNCDEF inline f32 clamp_angle(f32 min, f32 radians, f32 max)
{
    // From Unreal Engine:
    // Returns clamped angle in range -180..180.
    
    f32 max_delta         = clamp_axis(max - min) * 0.5f;		   // 0..180
    f32 range_center      = clamp_axis(min + max_delta);			// 0..360
    f32 delta_from_center = normalize_axis(radians - range_center); // -180..180
    
    // maybe clamp to nearest edge
    if (delta_from_center > max_delta)
        return normalize_axis(range_center + max_delta);
    else if (delta_from_center < -max_delta)
        return normalize_axis(range_center - max_delta);
    
    // already in range, just return it
    return normalize_axis(radians);
}
FUNCDEF inline V3 clamp_euler(V3 euler)
{
    V3 result;
    result.pitch = clamp_axis(euler.pitch);
    result.yaw   = clamp_axis(euler.yaw);
    result.roll  = clamp_axis(euler.roll);
    return result;
}
FUNCDEF inline V3 normalize_euler(V3 euler)
{
    V3 result;
    result.pitch = normalize_axis(euler.pitch);
    result.yaw   = normalize_axis(euler.yaw);
    result.roll  = normalize_axis(euler.roll);
    return result;
}
FUNCDEF inline V3 normalize_half_euler(V3 euler)
{
    V3 result;
    result.pitch = normalize_half_axis(euler.pitch);
    result.yaw   = normalize_half_axis(euler.yaw);
    result.roll  = normalize_half_axis(euler.roll);
    return result;
}

#if !defined(ORH_NO_STD_MATH) || COMPILER_CL
#include <math.h>
FUNCDEF inline f32 _pow(f32 x, f32 y)     
{
    f32 result = powf(x, y); 
    return result;
}
FUNCDEF inline f64 _mod(f64 x, f64 y)     
{
    f64 result = fmod(x, y); 
    return result;
}
FUNCDEF inline f32 _mod(f32 x, f32 y)     
{
    f32 result = fmodf(x, y); 
    return result;
}
FUNCDEF inline f32 _sqrt(f32 x)           
{
    f32 result = sqrtf(x); 
    return result;
}
FUNCDEF inline f32 _sin(f32 radians)      
{
    f32 result = sinf(radians); 
    return result;
}
FUNCDEF inline f32 _cos(f32 radians)      
{
    f32 result = cosf(radians); 
    return result;
}
FUNCDEF inline f32 _tan(f32 radians)      
{
    f32 result = tanf(radians); 
    return result;
}
FUNCDEF inline f32 _arcsin(f32 x)         
{
    f32 result = asinf(x); 
    return result;
}
FUNCDEF inline f32 _arccos(f32 x)         
{
    f32 result = acosf(x); 
    return result;
}
FUNCDEF inline f32 _arctan(f32 x)         
{
    f32 result = atanf(x); 
    return result;
}
FUNCDEF inline f32 _arctan2(f32 y, f32 x) 
{
    f32 result = atan2f(y, x); 
    return result;
}
FUNCDEF inline f32 _round(f32 x)          
{
    f32 result = roundf(x); 
    return result;
}
FUNCDEF inline f32 _floor(f32 x)          
{
    f32 result = floorf(x); 
    return result;
}
FUNCDEF inline f32 _ceil(f32 x)           
{
    f32 result = ceilf(x); 
    return result;
}
#else
FUNCDEF inline f32 _pow(f32 x, f32 y)     
{
    f32 result = __builtin_powf(x, y); 
    return result;
}
FUNCDEF inline f64 _mod(f64 x, f64 y)     
{
    f64 result = __builtin_fmod(x, y);
    return result;
}
FUNCDEF inline f32 _mod(f32 x, f32 y)     
{
    f32 result = __builtin_fmodf(x, y); 
    return result;
}
FUNCDEF inline f32 _sqrt(f32 x)           
{
    f32 result = __builtin_sqrtf(x); 
    return result;
}
FUNCDEF inline f32 _sin(f32 radians)      
{
    f32 result = __builtin_sinf(radians); 
    return result;
}
FUNCDEF inline f32 _cos(f32 radians)      
{
    f32 result = __builtin_cosf(radians); 
    return result;
}
FUNCDEF inline f32 _tan(f32 radians)      
{
    f32 result = __builtin_tanf(radians); 
    return result;
}
FUNCDEF inline f32 _arcsin(f32 x)         
{
    f32 result = __builtin_asinf(x); 
    return result;
}
FUNCDEF inline f32 _arccos(f32 x)         
{
    f32 result = __builtin_acosf(x); 
    return result;
}
FUNCDEF inline f32 _arctan(f32 x)
{
    f32 result = __builtin_atanf(x); 
    return result;
}
FUNCDEF inline f32 _arctan2(f32 y, f32 x)
{
    f32 result = __builtin_atan2f(y, x); 
    return result;
}
FUNCDEF inline f32 _round(f32 x)          
{
    f32 result = __builtin_roundf(x); 
    return result;
}
FUNCDEF inline f32 _floor(f32 x)          
{
    f32 result = __builtin_floorf(x); 
    return result;
}
FUNCDEF inline f32 _ceil(f32 x)           
{
    f32 result = __builtin_ceilf(x); 
    return result;
}
#endif
FUNCDEF inline f32 _sin_turns(f32 turns)
{
    return _sin(turns * TURNS_TO_RADS);
}
FUNCDEF inline f32 _cos_turns(f32 turns)
{
    return _cos(turns * TURNS_TO_RADS);
}
FUNCDEF inline f32 _tan_turns(f32 turns)
{
    return _tan(turns * TURNS_TO_RADS);
}
FUNCDEF inline f32 _arcsin_turns(f32 x)
{
    return _arcsin(x) * RADS_TO_TURNS;
}
FUNCDEF inline f32 _arccos_turns(f32 x)
{
    return _arccos(x) * RADS_TO_TURNS;
}
FUNCDEF inline f32 _arctan_turns(f32 x)
{
    return _arctan(x) * RADS_TO_TURNS;
}
FUNCDEF inline f32 _arctan2_turns(f32 y, f32 x)
{
    return clamp_axis(_arctan2(y, x)) * RADS_TO_TURNS;
}

FUNCDEF inline V2 pow(V2 v, f32 y)
{
    V2 result = {};
    result.x = _pow(v.x, y);
    result.y = _pow(v.y, y);
    return result;
}
FUNCDEF inline V3 pow(V3 v, f32 y)
{
    V3 result = {};
    result.x = _pow(v.x, y);
    result.y = _pow(v.y, y);
    result.z = _pow(v.z, y);
    return result;
}
FUNCDEF inline V2 clamp(V2 min, V2 x, V2 max)
{
    V2 result = {};
    result.x = CLAMP(min.x, x.x, max.x);
    result.y = CLAMP(min.y, x.y, max.y);
    return result;
}
FUNCDEF inline V3 clamp(V3 min, V3 x, V3 max)
{
    V3 result = {};
    result.x = CLAMP(min.x, x.x, max.x);
    result.y = CLAMP(min.y, x.y, max.y);
    result.z = CLAMP(min.z, x.z, max.z);
    return result;
}
FUNCDEF inline V2 round(V2 v)
{
    V2 result = {};
    result.x  = _round(v.x);
    result.y  = _round(v.y);
    return result;
}
FUNCDEF inline V3 round(V3 v)
{
    V3 result = {};
    result.x  = _round(v.x);
    result.y  = _round(v.y);
    result.z  = _round(v.z);
    return result;
}
FUNCDEF inline V2 floor(V2 v)
{
    V2 result = {};
    result.x  = _floor(v.x);
    result.y  = _floor(v.y);
    return result;
}
FUNCDEF inline V3 floor(V3 v)
{
    V3 result = {};
    result.x  = _floor(v.x);
    result.y  = _floor(v.y);
    result.z  = _floor(v.z);
    return result;
}
FUNCDEF inline V2 ceil(V2 v)
{
    V2 result = {};
    result.x  = _ceil(v.x);
    result.y  = _ceil(v.y);
    return result;
}
FUNCDEF inline V3 ceil(V3 v)
{
    V3 result = {};
    result.x  = _ceil(v.x);
    result.y  = _ceil(v.y);
    result.z  = _ceil(v.z);
    return result;
}

FUNCDEF inline f32 frac(f32 x)
{
    f32 result = x - _floor(x);
    return result;
}
FUNCDEF inline V2 frac(V2 v)
{
    V2 result = {};
    result.x  = frac(v.x);
    result.y  = frac(v.y);
    return result;
}
FUNCDEF inline V3 frac(V3 v)
{
    V3 result = {};
    result.x  = frac(v.x);
    result.y  = frac(v.y);
    result.z  = frac(v.z);
    return result;
}

FUNCDEF inline f32 safe_div(f32 x, f32 y, f32 n) 
{
    f32 result = y == 0.0f ? n : x/y; 
    return result;
}
FUNCDEF inline f32 safe_div0(f32 x, f32 y)       
{
    f32 result = safe_div(x, y, 0.0f); 
    return result;
}
FUNCDEF inline f32 safe_div1(f32 x, f32 y)       
{
    f32 result = safe_div(x, y, 1.0f); 
    return result;
}

FUNCDEF inline b32 nearly_zero(f32 x, f32 tolerance /*= SMALL_NUMBER*/)
{
    b32 result = (ABS(x) <= tolerance);
    return result;
}

FUNCDEF inline V2 v2(f32 x, f32 y)
{
    V2 v = {x, y}; 
    return v; 
}
FUNCDEF inline V2 v2(f32 c)
{
    V2 v = {c, c}; 
    return v; 
}

FUNCDEF inline V2u v2u(u32 x, u32 y)
{
    V2u v = {x, y}; 
    return v; 
}
FUNCDEF inline V2u v2u(u32 c)
{
    V2u v = {c, c}; 
    return v; 
}

FUNCDEF inline V2s v2s(s32 x, s32 y)
{
    V2s v = {x, y}; 
    return v; 
}
FUNCDEF inline V2s v2s(s32 c)
{
    V2s v = {c, c}; 
    return v; 
}

FUNCDEF inline V3 v3(f32 x, f32 y, f32 z)
{
    V3 v = {x, y, z}; 
    return v; 
}
FUNCDEF inline V3 v3(f32 c)
{
    V3 v = {c, c, c}; 
    return v; 
}
FUNCDEF inline V3 v3(V2 xy, f32 z)
{
    V3 v; v.xy = xy; v.z = z; 
    return v; 
}

FUNCDEF inline V4 v4(f32 x, f32 y, f32 z, f32 w)
{
    V4 v = {x, y, z, w}; 
    return v; 
}
FUNCDEF inline V4 v4(f32 c)
{
    V4 v = {c, c, c, c}; 
    return v; 
}
FUNCDEF inline V4 v4(V3 xyz, f32 w)
{
    V4 v; v.xyz = xyz; v.w = w; 
    return v; 
}
FUNCDEF inline V4 v4(V2 xy, V2 zw)
{
    V4 v; v.xy = xy; v.zw = zw;
    return v;
}

FUNCDEF inline V4s v4s(s32 c)
{
    V4s v = {c, c, c, c};
    return v;
}

FUNCDEF inline V2 hadamard_mul(V2 a, V2 b)
{
    V2 v = {a.x*b.x, a.y*b.y}; 
    return v; 
}
FUNCDEF inline V3 hadamard_mul(V3 a, V3 b)
{
    V3 v = {a.x*b.x, a.y*b.y, a.z*b.z}; 
    return v; 
}
FUNCDEF inline V4 hadamard_mul(V4 a, V4 b)
{
    V4 v = {a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w}; 
    return v; 
}

FUNCDEF inline V2 hadamard_div(V2 a, V2 b)
{
    V2 v = {a.x/b.x, a.y/b.y}; 
    return v; 
}
FUNCDEF inline V3 hadamard_div(V3 a, V3 b)
{
    V3 v = {a.x/b.x, a.y/b.y, a.z/b.z}; 
    return v; 
}
FUNCDEF inline V4 hadamard_div(V4 a, V4 b)
{
    V4 v = {a.x/b.x, a.y/b.y, a.z/b.z, a.w/b.w}; 
    return v; 
}

FUNCDEF inline f32 dot(V2 a, V2 b) 
{ 
    f32 result = a.x*b.x + a.y*b.y;
    return result;
}
FUNCDEF inline s32 dot(V2s a, V2s b) 
{ 
    s32 result = a.x*b.x + a.y*b.y; 
    return result;
}
FUNCDEF inline f32 dot(V3 a, V3 b) 
{ 
    f32 result = a.x*b.x + a.y*b.y + a.z*b.z; 
    return result;
}
FUNCDEF inline f32 dot(V4 a, V4 b) 
{ 
    f32 result = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; 
    return result;
}

FUNCDEF inline V2 perp(V2 v)
{
    // Counter-Clockwise.
    
    V2 result = {-v.y, v.x};
    return result;
}
FUNCDEF inline V3 cross(V3 a, V3 b) 
{
    V3 result =
    {
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    };
    return result;
}

FUNCDEF inline f32 length2(V2 v) 
{
    f32 result = dot(v, v); 
    return result;
}
FUNCDEF inline f32 length2(V3 v) 
{
    f32 result = dot(v, v); 
    return result;
}
FUNCDEF inline f32 length2(V4 v) 
{
    f32 result = dot(v, v); 
    return result;
}

FUNCDEF inline f32 length(V2 v) 
{
    f32 result = _sqrt(length2(v)); 
    return result;
}
FUNCDEF inline f32 length(V3 v) 
{
    f32 result = _sqrt(length2(v)); 
    return result;
}
FUNCDEF inline f32 length(V4 v) 
{
    f32 result = _sqrt(length2(v)); 
    return result;
}

FUNCDEF inline V2 min_v2(V2 a, V2 b)
{
    V2 result = 
    {
        MIN(a.x, b.x),
        MIN(a.y, b.y),
    };
    return result;
}
FUNCDEF inline V3 min_v3(V3 a, V3 b)
{
    V3 result = 
    {
        MIN(a.x, b.x),
        MIN(a.y, b.y),
        MIN(a.z, b.z),
    };
    return result;
}
FUNCDEF inline V4 min_v4(V4 a, V4 b)
{
    V4 result = 
    {
        MIN(a.x, b.x),
        MIN(a.y, b.y),
        MIN(a.z, b.z),
        MIN(a.w, b.w),
    };
    return result;
}

FUNCDEF inline V2 max_v2(V2 a, V2 b)
{
    V2 result = 
    {
        MAX(a.x, b.x),
        MAX(a.y, b.y),
    };
    return result;
}
FUNCDEF inline V3 max_v3(V3 a, V3 b)
{
    V3 result = 
    {
        MAX(a.x, b.x),
        MAX(a.y, b.y),
        MAX(a.z, b.z),
    };
    return result;
}
FUNCDEF inline V4 max_v4(V4 a, V4 b)
{
    V4 result = 
    {
        MAX(a.x, b.x),
        MAX(a.y, b.y),
        MAX(a.z, b.z),
        MAX(a.w, b.w),
    };
    return result;
}

FUNCDEF inline V2 normalize(V2 v) 
{
    V2 result = (v * (1.0f / length(v))); 
    return result;
}
FUNCDEF inline V3 normalize(V3 v) 
{
    V3 result = (v * (1.0f / length(v))); 
    return result;
}
FUNCDEF inline V4 normalize(V4 v) 
{
    V4 result = (v * (1.0f / length(v))); 
    return result;
}

FUNCDEF inline V2 normalize_or_zero(V2 v) 
{ 
    f32 len = length(v); 
    return len > 0 ? v/len : v2(0.0f); 
}
FUNCDEF inline V3 normalize_or_zero(V3 v) 
{ 
    f32 len = length(v); 
    return len > 0 ? v/len : v3(0.0f); 
}
FUNCDEF inline V4 normalize_or_zero(V4 v) 
{ 
    f32 len = length(v);
    return len > 0 ? v/len : v4(0.0f); 
}

FUNCDEF inline V3 normalize_or_x_axis(V3 v)
{
    f32 len = length(v);
    return len > 0 ? v/len : V3_X_AXIS; 
}
FUNCDEF inline V3 normalize_or_y_axis(V3 v)
{
    f32 len = length(v);
    return len > 0 ? v/len : V3_Y_AXIS; 
}
FUNCDEF inline V3 normalize_or_z_axis(V3 v)
{
    f32 len = length(v);
    return len > 0 ? v/len : V3_Z_AXIS;
}

FUNCDEF inline V3 reflect(V3 incident, V3 normal)
{
    V3 result = incident - 2.0f * dot(incident, normal) * normal;
    return result;
}

FUNCDEF inline f32 dot(Quaternion a, Quaternion b) 
{
    f32 result = dot(a.xyz, b.xyz) + a.w*b.w; 
    return result;
}
FUNCDEF inline f32 length(Quaternion q)            
{
    f32 result = _sqrt(dot(q, q)); 
    return result;
}
FUNCDEF inline Quaternion normalize(Quaternion q)  
{
    Quaternion result = (q * (1.0f / length(q))); 
    return result;
}
FUNCDEF inline Quaternion normalize_or_identity(Quaternion q)
{
    f32 len = length(q);
    return len > 0 ? (q * 1.0f/len) : quaternion_identity();
}

FUNCDEF inline Quaternion quaternion(f32 x, f32 y, f32 z, f32 w) 
{ 
    Quaternion q = {x, y, z, w}; 
    return q; 
}
FUNCDEF inline Quaternion quaternion(V3 v, f32 w)
{ 
    Quaternion q; q.xyz = v; q.w = w; 
    return q; 
}
FUNCDEF inline Quaternion quaternion_identity()
{
    Quaternion result = {0.0f, 0.0f, 0.0f, 1.0f};
    return result;
}


FUNCDEF inline V3 get_euler(Quaternion q)
{
    // From:
    // https://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/
    //
    // Returns euler angles in (-180, 180] range.
    
    q = normalize_or_identity(q);
    
    const f32 SINGULARITY_THRESHOLD = 0.4999995f;
    f32 singularity_test            = q.z*q.y + q.x*q.w;
    f32 pitch, yaw, roll;
	
	if (singularity_test < -SINGULARITY_THRESHOLD) { // singularity at south pole
		pitch = -PI32 / 2.0f;
		yaw   = -2.0f * _arctan2(q.z,q.w);
		roll  = 0.0f;
	} if (singularity_test > SINGULARITY_THRESHOLD) { // singularity at north pole
		pitch = PI32/2;
		yaw   = 2.0f * _arctan2(q.z,q.w);
		roll  = 0.0f;
	} else {
        f32 sqz = q.z*q.z;
        f32 sqy = q.y*q.y;
        f32 sqx = q.x*q.x;
        pitch = _arcsin (2.0f*singularity_test);
        yaw   = _arctan2(2.0f*q.y*q.w-2.0f*q.z*q.x , 1.0f - 2.0f*sqy - 2.0f*sqx);
        roll  = _arctan2(2.0f*q.z*q.w-2.0f*q.y*q.x , 1.0f - 2.0f*sqz - 2.0f*sqx);
    }
    
    V3 result = {pitch, yaw, roll};
    return result;
}
FUNCDEF inline Quaternion quaternion_from_euler(V3 euler)
{
    // From:
    // https://www.euclideanspace.com/maths/geometry/rotations/conversions/eulerToQuaternion/index.htm
    
    euler.pitch = _mod(euler.pitch, TAU32);
    euler.yaw   = _mod(euler.yaw,   TAU32);
    euler.roll  = _mod(euler.roll,  TAU32);
    
    f32 cp = _cos(euler.pitch * 0.5f);;
    f32 cy = _cos(euler.yaw   * 0.5f);
    f32 cr = _cos(euler.roll  * 0.5f);
    f32 sp = _sin(euler.pitch * 0.5f);
    f32 sy = _sin(euler.yaw   * 0.5f);
    f32 sr = _sin(euler.roll  * 0.5f);
    
    Quaternion result;
    result.x = sp*cy*cr - cp*sy*sr;
    result.y = cp*sy*cr + sp*cy*sr;
    result.z = sp*sy*cr + cp*cy*sr;
    result.w = cp*cy*cr - sp*sy*sr;
    
    return result;
}
Quaternion quaternion_from_m3x3(M3x3 const &affine)
{
    // @Note: From GLM. I think this implementation is based on the paper:
    // "Accurate Computation of Quaternions from Rotation Matrices" by Soheil Sarabandi and Federico Thomas.
    
    // @Sanity: Remove any potential scaling that might have been applied to the input matrix.
    M3x3 m = get_rotation(affine);
    
    f32 fourXSquaredMinus1 = m._11 - m._22 - m._33;
    f32 fourYSquaredMinus1 = m._22 - m._11 - m._33;
    f32 fourZSquaredMinus1 = m._33 - m._11 - m._22;
    f32 fourWSquaredMinus1 = m._11 + m._22 + m._33;
    
    s32 biggest_idx = 0;
    f32 fourBiggestSquaredMinus1 = fourWSquaredMinus1;
    if(fourXSquaredMinus1 > fourBiggestSquaredMinus1) {
        fourBiggestSquaredMinus1 = fourXSquaredMinus1;
        biggest_idx = 1;
    }
    if(fourYSquaredMinus1 > fourBiggestSquaredMinus1) {
        fourBiggestSquaredMinus1 = fourYSquaredMinus1;
        biggest_idx = 2;
    }
    if(fourZSquaredMinus1 > fourBiggestSquaredMinus1) {
        fourBiggestSquaredMinus1 = fourZSquaredMinus1;
        biggest_idx = 3;
    }
    
    f32 biggest_val = _sqrt(fourBiggestSquaredMinus1 + 1.0f) * 0.5f;
    f32 mult        = 0.25f / biggest_val;
    
    Quaternion result;
    switch(biggest_idx) {
        case 0:
        result = quaternion((m._32 - m._23) * mult, (m._13 - m._31) * mult, (m._21 - m._12) * mult, biggest_val); break;
        case 1:
        result = quaternion(biggest_val, (m._21 + m._12) * mult, (m._13 + m._31) * mult, (m._32 - m._23) * mult); break;
        case 2:
        result = quaternion((m._21 + m._12) * mult, biggest_val, (m._32 + m._23) * mult, (m._13 - m._31) * mult); break;
        case 3:
        result = quaternion((m._13 + m._31) * mult, (m._32 + m._23) * mult, biggest_val, (m._21 - m._12) * mult); break;
        default: // Silence a -Wswitch-default warning in GCC. Should never actually get here. Assert is just for sanity.
        ASSERT(false);
        result = quaternion(0, 0, 0, 1); break;
    }
    
    return result;
}
FUNCDEF inline Quaternion quaternion_from_m4x4(M4x4 const &affine)
{
    Quaternion result = quaternion_from_m3x3(m3x3(affine));
    return result;
}
FUNCDEF inline Quaternion quaternion_look_at(V3 direction, V3 up)
{
    // @Note: Axes will be normalized when calculating the quaternion.
    V3 zaxis = -direction;
    V3 xaxis = cross(up, zaxis);
    V3 yaxis = cross(zaxis, xaxis);
    M3x3 rot =
    {
        xaxis.x, yaxis.x, zaxis.x,
        xaxis.y, yaxis.y, zaxis.y,
        xaxis.z, yaxis.z, zaxis.z,
    };
    Quaternion result = quaternion_from_m3x3(rot);
    return result;
}
FUNCDEF inline Quaternion quaternion_from_axis_angle(V3 axis, f32 radians)
{
    // @Note: Rotation around _axis_ by _angle_ radians.
    Quaternion result;
    result.v = axis * _sin(radians*0.5f);
    result.w =        _cos(radians*0.5f);
    return result;
}
FUNCDEF inline Quaternion quaternion_from_axis_angle_turns(V3 axis, f32 turns)
{
    Quaternion result = quaternion_from_axis_angle(axis, turns * TURNS_TO_RADS);
    return result;
}

Quaternion quaternion_from_two_vectors_helper(V3 a, V3 b, f32 len_ab)
{
    // @Note: From: https://stackoverflow.com/questions/1171849/finding-quaternion-representing-the-rotation-from-one-vector-to-another
    // Unreal Engine approach.
    
    f32 w = len_ab + dot(a, b);
    
    Quaternion result;
    if (w >= 1e-6f * len_ab) {
        result = quaternion(cross(a, b), w);
    } else {
        // a and b point in opposite directions (we cross with orthogonal basis instead).
        w = 0.0f;
        f32 x = ABS(a.x);
        f32 y = ABS(a.y);
        f32 z = ABS(a.z);
        
        // Find orthogonal basis.
        V3 basis = (x>y && x>z)? v3(0.0f, 1.0f, 0.0f) : v3(-1.0f, 0.0f, 0.0f);
        result   = quaternion(cross(a, basis), w);
    }
    
    result = normalize(result);
    return result;
}
FUNCDEF inline Quaternion quaternion_from_two_vectors(V3 a, V3 b)
{
    f32 len = _sqrt(length2(a) * length2(b));
    Quaternion result = quaternion_from_two_vectors_helper(a, b, len);
    return result;
}
FUNCDEF inline Quaternion quaternion_from_two_normals(V3 a, V3 b)
{
    f32 len = 1.0f;
    Quaternion result = quaternion_from_two_vectors_helper(a, b, len);
    return result;
}

FUNCDEF inline Quaternion quaternion_conjugate(Quaternion q) 
{
    Quaternion result;
    result.v = -q.v;
    result.w =  q.w;
    return result;
}
FUNCDEF inline Quaternion quaternion_inverse(Quaternion q) 
{
    Quaternion result = quaternion_conjugate(q)/dot(q, q);
    return result;
}
FUNCDEF inline V3 quaternion_get_axis(Quaternion q)
{
    V3 result = normalize_or_zero(q.v);
    return result;
}
FUNCDEF inline f32 quaternion_get_angle(Quaternion q)
{
    f32 result = _arctan2(length(q.v), q.w) * 2.0f;
    return result;
}
FUNCDEF inline f32 quaternion_get_angle_turns(Quaternion q)
{
    f32 result = quaternion_get_angle(q) * RADS_TO_TURNS;
    return result;
}

FUNCDEF inline M3x3 m3x3_identity()
{
    M3x3 result = 
    {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
    };
    return result;
}
FUNCDEF inline M3x3 m3x3(M4x4 const &m)
{
    M3x3 result = 
    {
        m._11, m._12, m._13,
        m._21, m._22, m._23,
        m._31, m._32, m._33,
    };
    return result;
}
M3x3 m3x3_from_quaternion(Quaternion q)
{
    // Taken from:
    // https://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm
    
    /*     
        1 - 2*qy2 - 2*qz2	2*qx*qy - 2*qz*qw	2*qx*qz + 2*qy*qw;
        2*qx*qy + 2*qz*qw	1 - 2*qx2 - 2*qz2	2*qy*qz - 2*qx*qw;
        2*qx*qz - 2*qy*qw	2*qy*qz + 2*qx*qw	1 - 2*qx2 - 2*qy2;
         */
    
    M3x3 result;
    q = normalize_or_identity(q);
    
    f32 qyy = q.y * q.y; f32 qzz = q.z * q.z; f32 qxy = q.x * q.y; 
    f32 qxz = q.x * q.z; f32 qyw = q.y * q.w; f32 qzw = q.z * q.w; 
    f32 qxx = q.x * q.x; f32 qyz = q.y * q.z; f32 qxw = q.x * q.w; 
    
    result._11 = 1.0f - 2.0f * (qyy + qzz);
    result._12 =        2.0f * (qxy - qzw);
    result._13 =        2.0f * (qxz + qyw);
    
    result._21 =        2.0f * (qxy + qzw);
    result._22 = 1.0f - 2.0f * (qxx + qzz);
    result._23 =        2.0f * (qyz - qxw);
    
    result._31 =        2.0f * (qxz - qyw);
    result._32 =        2.0f * (qyz + qxw);
    result._33 = 1.0f - 2.0f * (qxx + qyy);
    
    return result;
}

FUNCDEF inline M3x3 get_rotation(M3x3 const &m)
{
    // Simply normalize the columns of the input matrix.
    V3 xaxis = normalize_or_x_axis(v3(m._11, m._21, m._31)); // x-axis column[0]
    V3 yaxis = normalize_or_y_axis(v3(m._12, m._22, m._32)); // y-axis column[1]
    V3 zaxis = normalize_or_z_axis(v3(m._13, m._23, m._33)); // z-axis column[2]
    M3x3 result =
    {
        xaxis.x, yaxis.x, zaxis.x,
        xaxis.y, yaxis.y, zaxis.y,
        xaxis.z, yaxis.z, zaxis.z,
    };
    return result;
}
FUNCDEF inline V3 get_scale(M3x3 const &m)
{
    V3 result =
    {
        length(v3(m._11, m._21, m._31)), // x-axis column[0]
        length(v3(m._12, m._22, m._32)), // y-axis column[1]
        length(v3(m._13, m._23, m._33))  // z-axis column[2]
    };
    return result;
}

FUNCDEF inline M3x3 transpose(M3x3 const &m)
{
    M3x3 result = 
    {
        m._11, m._21, m._31,
        m._12, m._22, m._32,
        m._13, m._23, m._33,
    };
    return result;
}
FUNCDEF inline V3 transform(M3x3 const &m, V3 v)
{
    V3 result = 
    {
        m._11*v.x + m._12*v.y + m._13*v.z,
        m._21*v.x + m._22*v.y + m._23*v.z,
        m._31*v.x + m._32*v.y + m._33*v.z,
    };
    return result;
}

FUNCDEF inline M4x4 m4x4_identity() 
{
    M4x4 result = 
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    return result;
}
FUNCDEF inline void m4x4_set_upper(M4x4 *m, M3x3 const &upper)
{
    m->row[0].xyz = upper.row[0];
    m->row[1].xyz = upper.row[1];
    m->row[2].xyz = upper.row[2];
}
FUNCDEF inline M4x4 m4x4_from_quaternion(Quaternion q)
{
    M3x3 rot    = m3x3_from_quaternion(q);
    M4x4 result = m4x4_identity();
    m4x4_set_upper(&result, rot);
    return result;
}
M4x4 m4x4_from_axis_angle(V3 axis, f32 radians)
{
    // @Note: From GLM.
    
    // @Hack: This is giving us a rotation in the wrong direction, so I will just negate the angle :)
    radians = -radians;
    f32 c   = _cos(radians);
    f32 s   = _sin(radians);
    
    axis    = normalize_or_zero(axis);
    V3 temp = (1.0f - c) * axis;
    
    M4x4 result = m4x4_identity();
    result._11 = c + temp.x * axis.x;          // XAXIS_x
    result._12 = temp.x * axis.y + s * axis.z; // YAXIS_x
    result._13 = temp.x * axis.z - s * axis.y; // ZAXIS_x
    
    result._21 = temp.y * axis.x - s * axis.z; // XAXIS_y
    result._22 = c + temp.y * axis.y;          // YAXIS_y
    result._23 = temp.y * axis.z + s * axis.x; // ZAXIS_y
    
    result._31 = temp.z * axis.x + s * axis.y; // XAXIS_z
    result._32 = temp.z * axis.y - s * axis.x; // YAXIS_z
    result._33 = c + temp.z * axis.z;          // ZAXIS_z
    
    return result;
}
FUNCDEF inline M4x4 m4x4_from_axis_angle_turns(V3 axis, f32 turns)
{
    M4x4 result = m4x4_from_axis_angle(axis, turns * TURNS_TO_RADS);
    return result;
}
FUNCDEF inline M4x4 m4x4_from_translation_rotation_scale(V3 t, Quaternion r, V3 s)
{
    // @Note: Instead of doing T*R*S, we use this faster way. Read Misc about composing affine transforms.
    
    M4x4 result = m4x4_from_quaternion(r);
    result.row[0].xyz *= s;
    result.row[1].xyz *= s;
    result.row[2].xyz *= s;
    
    result._14 = t.x;
    result._24 = t.y;
    result._34 = t.z;
    
    return result;
}

FUNCDEF inline V4 get_column(M4x4 const &m, u32 c) 
{
    V4 result = {m.II[0][c], m.II[1][c], m.II[2][c], m.II[3][c]};
    return result;
}
FUNCDEF inline V4 get_row(M4x4 const &m, u32 r)
{
    V4 result = {m.II[r][0], m.II[r][1], m.II[r][2], m.II[r][3]};
    return result;
}
FUNCDEF inline V3 get_x_axis(M4x4 const &affine)
{
    V3 result = {affine._11, affine._21, affine._31}; // column[0]
    return result;
}
FUNCDEF inline V3 get_y_axis(M4x4 const &affine)
{
    V3 result = {affine._12, affine._22, affine._32}; // column[1]
    return result;
}
FUNCDEF inline V3 get_z_axis(M4x4 const &affine)
{
    V3 result = {affine._13, affine._23, affine._33}; // column[2]
    return result;
}
FUNCDEF inline V3 get_forward(M4x4 const &affine)
{
    // Forward is negative z-axis.
    V3 result = -normalize_or_z_axis(get_z_axis(affine));
    return result;
}
FUNCDEF inline V3 get_right(M4x4 const &affine)
{
    V3 result = normalize_or_x_axis(get_x_axis(affine));
    return result;
}
FUNCDEF inline V3 get_up(M4x4 const &affine)
{
    V3 result = normalize_or_y_axis(get_y_axis(affine));
    return result;
}
FUNCDEF inline V3 get_translation(M4x4 const &affine)
{
    V3 result = get_column(affine, 3).xyz; 
    return result;
}
FUNCDEF inline M4x4 get_rotation(M4x4 const &affine)
{
    M4x4 result = m4x4_identity();
    M3x3 rot    = get_rotation(m3x3(affine));
    m4x4_set_upper(&result, rot);
    return result;
}
FUNCDEF inline V3 get_scale(M4x4 const &affine)
{
    V3 result =
    {
        length(get_x_axis(affine)),
        length(get_y_axis(affine)),
        length(get_z_axis(affine))
    };
    return result;
}

FUNCDEF inline M4x4 transpose(M4x4 const &m)
{
    M4x4 result = 
    {
        m._11, m._21, m._31, m._41,
        m._12, m._22, m._32, m._42,
        m._13, m._23, m._33, m._43,
        m._14, m._24, m._34, m._44
    };
    return result;
}
b32 invert(M4x4 const &m, M4x4 *result)
{
    // @Note: Call this to compute the inverse of a matrix of unknown origin.
    // Otherwise constructing the inverse of a matrix by hand could be faster.
    
    // @Note: Read up on how to compute the inverse:
    // https://mathworld.wolfram.com/MatrixInverse.html
    
    // @Note: Taken from the MESA implementation of the GLU libray:
    // https://www.mesa3d.org/
    // https://github.com/Starlink/mesa/blob/master/src/glu/sgi/libutil/project.c
    //
    //
    //
    // The implementation uses:
    //
    // inverse(M) = adjugate(M) / determinant(M);
    //
    // And the determinants are expanded using the Laplace expansion.
    
    f32 inv[16], det;
    
    inv[0] =   m.I[5]*m.I[10]*m.I[15] - m.I[5]*m.I[11]*m.I[14] - m.I[9]*m.I[6]*m.I[15]
        + m.I[9]*m.I[7]*m.I[14] + m.I[13]*m.I[6]*m.I[11] - m.I[13]*m.I[7]*m.I[10];
    inv[4] =  -m.I[4]*m.I[10]*m.I[15] + m.I[4]*m.I[11]*m.I[14] + m.I[8]*m.I[6]*m.I[15]
        - m.I[8]*m.I[7]*m.I[14] - m.I[12]*m.I[6]*m.I[11] + m.I[12]*m.I[7]*m.I[10];
    inv[8] =   m.I[4]*m.I[9]*m.I[15] - m.I[4]*m.I[11]*m.I[13] - m.I[8]*m.I[5]*m.I[15]
        + m.I[8]*m.I[7]*m.I[13] + m.I[12]*m.I[5]*m.I[11] - m.I[12]*m.I[7]*m.I[9];
    inv[12] = -m.I[4]*m.I[9]*m.I[14] + m.I[4]*m.I[10]*m.I[13] + m.I[8]*m.I[5]*m.I[14]
        - m.I[8]*m.I[6]*m.I[13] - m.I[12]*m.I[5]*m.I[10] + m.I[12]*m.I[6]*m.I[9];
    inv[1] =  -m.I[1]*m.I[10]*m.I[15] + m.I[1]*m.I[11]*m.I[14] + m.I[9]*m.I[2]*m.I[15]
        - m.I[9]*m.I[3]*m.I[14] - m.I[13]*m.I[2]*m.I[11] + m.I[13]*m.I[3]*m.I[10];
    inv[5] =   m.I[0]*m.I[10]*m.I[15] - m.I[0]*m.I[11]*m.I[14] - m.I[8]*m.I[2]*m.I[15]
        + m.I[8]*m.I[3]*m.I[14] + m.I[12]*m.I[2]*m.I[11] - m.I[12]*m.I[3]*m.I[10];
    inv[9] =  -m.I[0]*m.I[9]*m.I[15] + m.I[0]*m.I[11]*m.I[13] + m.I[8]*m.I[1]*m.I[15]
        - m.I[8]*m.I[3]*m.I[13] - m.I[12]*m.I[1]*m.I[11] + m.I[12]*m.I[3]*m.I[9];
    inv[13] =  m.I[0]*m.I[9]*m.I[14] - m.I[0]*m.I[10]*m.I[13] - m.I[8]*m.I[1]*m.I[14]
        + m.I[8]*m.I[2]*m.I[13] + m.I[12]*m.I[1]*m.I[10] - m.I[12]*m.I[2]*m.I[9];
    inv[2] =   m.I[1]*m.I[6]*m.I[15] - m.I[1]*m.I[7]*m.I[14] - m.I[5]*m.I[2]*m.I[15]
        + m.I[5]*m.I[3]*m.I[14] + m.I[13]*m.I[2]*m.I[7] - m.I[13]*m.I[3]*m.I[6];
    inv[6] =  -m.I[0]*m.I[6]*m.I[15] + m.I[0]*m.I[7]*m.I[14] + m.I[4]*m.I[2]*m.I[15]
        - m.I[4]*m.I[3]*m.I[14] - m.I[12]*m.I[2]*m.I[7] + m.I[12]*m.I[3]*m.I[6];
    inv[10] =  m.I[0]*m.I[5]*m.I[15] - m.I[0]*m.I[7]*m.I[13] - m.I[4]*m.I[1]*m.I[15]
        + m.I[4]*m.I[3]*m.I[13] + m.I[12]*m.I[1]*m.I[7] - m.I[12]*m.I[3]*m.I[5];
    inv[14] = -m.I[0]*m.I[5]*m.I[14] + m.I[0]*m.I[6]*m.I[13] + m.I[4]*m.I[1]*m.I[14]
        - m.I[4]*m.I[2]*m.I[13] - m.I[12]*m.I[1]*m.I[6] + m.I[12]*m.I[2]*m.I[5];
    inv[3] =  -m.I[1]*m.I[6]*m.I[11] + m.I[1]*m.I[7]*m.I[10] + m.I[5]*m.I[2]*m.I[11]
        - m.I[5]*m.I[3]*m.I[10] - m.I[9]*m.I[2]*m.I[7] + m.I[9]*m.I[3]*m.I[6];
    inv[7] =   m.I[0]*m.I[6]*m.I[11] - m.I[0]*m.I[7]*m.I[10] - m.I[4]*m.I[2]*m.I[11]
        + m.I[4]*m.I[3]*m.I[10] + m.I[8]*m.I[2]*m.I[7] - m.I[8]*m.I[3]*m.I[6];
    inv[11] = -m.I[0]*m.I[5]*m.I[11] + m.I[0]*m.I[7]*m.I[9] + m.I[4]*m.I[1]*m.I[11]
        - m.I[4]*m.I[3]*m.I[9] - m.I[8]*m.I[1]*m.I[7] + m.I[8]*m.I[3]*m.I[5];
    inv[15] =  m.I[0]*m.I[5]*m.I[10] - m.I[0]*m.I[6]*m.I[9] - m.I[4]*m.I[1]*m.I[10]
        + m.I[4]*m.I[2]*m.I[9] + m.I[8]*m.I[1]*m.I[6] - m.I[8]*m.I[2]*m.I[5];
    
    det = m.I[0]*inv[0] + m.I[1]*inv[4] + m.I[2]*inv[8] + m.I[3]*inv[12];
    if (det == 0)
        return FALSE;
    
    det = 1.0f / det;
    
    for (s32 i = 0; i < 16; i++)
        result->I[i] = inv[i] * det;
    
    return TRUE;
}

FUNCDEF inline V4 transform(M4x4 const &m, V4 v)
{
    V4 result = 
    {
        m._11*v.x + m._12*v.y + m._13*v.z + m._14*v.w,
        m._21*v.x + m._22*v.y + m._23*v.z + m._24*v.w,
        m._31*v.x + m._32*v.y + m._33*v.z + m._34*v.w,
        m._41*v.x + m._42*v.y + m._43*v.z + m._44*v.w
    };
    return result;
}
FUNCDEF inline V3 transform_point(M4x4 const &m, V3 p)
{
    V3 result = transform(m, {p.x, p.y, p.z, 1.0f}).xyz;
    return result;
}
FUNCDEF inline V3 transform_vector(M4x4 const &m, V3 v)
{
    V3 result = transform(m, {v.x, v.y, v.z, 0.0f}).xyz;
    return result;
}

FUNCDEF inline f32 lerp(f32 a, f32 t, f32 b) 
{
    f32 result = a*(1.0f - t) + b*t; 
    return result;
}
FUNCDEF inline V2 lerp(V2 a, f32 t, V2 b) 
{
    V2 result = a*(1.0f - t) + b*t; 
    return result;
}
FUNCDEF inline V3 lerp(V3 a, f32 t, V3 b) 
{
    V3 result = a*(1.0f - t) + b*t; 
    return result;
}
FUNCDEF inline V4 lerp(V4 a, f32 t, V4 b) 
{
    V4 result = a*(1.0f - t) + b*t; 
    return result;
}

FUNCDEF inline f32 unlerp(f32 a, f32 value, f32 b)
{
    f32 t = (value - a) / (b - a);
    return t;
}
FUNCDEF inline f32 unlerp(V2 a, V2 v, V2 b)
{
    V2 av = v-a, ab = b-a;
    f32 t = dot(av, ab) / dot(ab, ab);
    return t;
}
FUNCDEF inline f32 unlerp(V3 a, V3 v, V3 b)
{
    V3 av = v-a, ab = b-a;
    f32 t = dot(av, ab) / dot(ab, ab);
    return t;
}
FUNCDEF inline f32 unlerp(V4 a, V4 v, V4 b)
{
    V4 av = v-a, ab = b-a;
    f32 t = dot(av, ab) / dot(ab, ab);
    return t;
}

FUNCDEF inline f32 remap(f32 value, f32 imin, f32 imax, f32 omin, f32 omax)
{
    f32 t      = unlerp(imin, value, imax);
    f32 result = lerp(omin, t, omax);
    return result;
}
FUNCDEF inline V2 remap(V2 value, V2 imin, V2 imax, V2 omin, V2 omax)
{
    f32 t     = unlerp(imin, value, imax);
    V2 result = lerp(omin, t, omax);
    return result;
}
FUNCDEF inline V3 remap(V3 value, V3 imin, V3 imax, V3 omin, V3 omax)
{
    f32 t     = unlerp(imin, value, imax);
    V3 result = lerp(omin, t, omax);
    return result;
}
FUNCDEF inline V4 remap(V4 value, V4 imin, V4 imax, V4 omin, V4 omax)
{
    f32 t     = unlerp(imin, value, imax);
    V4 result = lerp(omin, t, omax);
    return result;
}

FUNCDEF inline Quaternion lerp(Quaternion a, f32 t, Quaternion b)
{
    Quaternion q;
    q.xyzw = lerp(a.xyzw, t, b.xyzw);
    return q;
}
FUNCDEF inline f32 unlerp(Quaternion a, Quaternion q, Quaternion b)
{
    Quaternion aq = q-a, ab = b-a;
    f32 t = dot(aq, ab) / dot(ab, ab);
    return t;
}
FUNCDEF inline Quaternion nlerp(Quaternion a, f32 t, Quaternion b)
{
    return normalize_or_identity(lerp(a, t, b));
}
Quaternion slerp(Quaternion a, f32 t, Quaternion b)
{
    // Quaternion slerp according to wiki:
    // result = (b * q0^-1)^t * q0
    
    // Geometric slerp according to wiki, and help from glm to resolve edge cases:
    Quaternion result;
    
    f32 cos_theta = dot(a, b);
    if (cos_theta < 0.0f) {
        b         = -b;
        cos_theta = -cos_theta;
    }
    
    // When cos_theta is 1.0f, sin_theta is 0.0f. 
    // Avoid the slerp formula when we get close to 1.0f and do a lerp() instead.
    if (cos_theta > 1.0f - 1.192092896e-07F) {
        result = lerp(a, t, b);
    } else {
        f32 theta = _arccos(cos_theta);
        result    = (_sin((1-t)*theta)*a + _sin(t*theta)*b) / _sin(theta);
    }
    
    return result;
}

FUNCDEF inline f32 smooth_step(f32 edge0, f32 x, f32 edge1) 
{
    x = CLAMP01((x - edge0) / (edge1 - edge0));
    x = SQUARE(x)*(3.0f - 2.0f*x);
    return x;
}
FUNCDEF inline f32 smoother_step(f32 edge0, f32 x, f32 edge1) 
{
    x = CLAMP01((x - edge0) / (edge1 - edge0));
    x = CUBE(x)*(x*(6.0f*x - 15.0f) + 10.0f);
    return x;
}

f32 move_towards(f32 current, f32 target, f32 delta_time, f32 speed)
{
    f32 max_distance = delta_time * speed;
    f32 delta        = target - current;
    if (ABS(delta) <= max_distance)
        return target;
    
    return current + SIGN(delta) * max_distance;
}
V2 move_towards(V2 current, V2 target, f32 delta_time, f32 speed)
{
    f32 max_distance = delta_time * speed;
    V2 delta         = target - current;
    f32 distance2    = length2(delta);
    
    if (distance2 <= SQUARE(max_distance)) 
        return target;
    
    f32 ratio = max_distance / _sqrt(distance2);
    return current + (delta * ratio);
}
V3 move_towards(V3 current, V3 target, f32 delta_time, f32 speed)
{
    f32 max_distance = delta_time * speed;
    V3 delta         = target - current;
    f32 distance2    = length2(delta);
    
    if (distance2 <= SQUARE(max_distance)) 
        return target;
    
    f32 ratio = max_distance / _sqrt(distance2);
    return current + (delta * ratio);
}
FUNCDEF inline V2 rotate_point_around_pivot(V2 point, V2 pivot, Quaternion q)
{
    V2 result = rotate_point_around_pivot(v3(point, 0), v3(pivot, 0), q).xy;
    return result;
}
FUNCDEF inline V3 rotate_point_around_pivot(V3 point, V3 pivot, Quaternion q)
{
    V3 result = (q * (point - pivot)) + pivot;
    return result;
}

FUNCDEF inline Range range(f32 min, f32 max) 
{
    Range result = {min, max}; 
    return result; 
}
FUNCDEF inline Rect2 rect2(V2  min, V2  max) 
{ 
    Rect2 result = {min, max}; 
    return result; 
}
FUNCDEF inline Rect2 rect2(f32 x0, f32 y0, f32 x1, f32 y1)
{
    Rect2 result = {x0, y0, x1, y1}; 
    return result; 
}
FUNCDEF inline Rect3 rect3(V3  min, V3  max)
{ 
    Rect3 result = {min, max}; 
    return result; 
}
FUNCDEF inline Rect3 rect3(f32 x0, f32 y0, f32 z0, f32 x1, f32 y1, f32 z1)
{
    Rect3 result = {x0, y0, z0, x1, y1, z1}; 
    return result; 
}

FUNCDEF inline V2 get_center(Rect2 rect)
{
    V2 result = (rect.min + rect.max) * 0.5f;
    return result;
}
FUNCDEF inline V3 get_center(Rect3 rect)
{
    V3 result = (rect.min + rect.max) * 0.5f;
    return result;
}

FUNCDEF inline V2 get_size(Rect2 rect)
{
    V2 result = rect.max - rect.min;
    return result;
}
FUNCDEF inline V3 get_size(Rect3 rect)
{
    V3 result = rect.max - rect.min;
    return result;
}

FUNCDEF inline f32 get_width(Rect2 rect)
{
    f32 result = rect.max.x - rect.min.x;
    return result;
}
FUNCDEF inline f32 get_height(Rect2 rect)
{
    f32 result = rect.max.y - rect.min.y;
    return result;
}

//
// @Note: From Unity!
//
FUNCDEF inline f32 repeat(f32 x, f32 max_y)
{
    // Loops the returned value (y a.k.a. result), so that it is never larger than max_y and never smaller than 0.
    //
    //    y
    //    |   /  /  /  /
    //    |  /  /  /  /
    //   x|_/__/__/__/_
    //
    f32 result = CLAMP(0.0f, x - _floor(x / max_y) * max_y, max_y);
    return result;
}
FUNCDEF inline f32 ping_pong(f32 x, f32 max_y)
{
    // PingPongs the returned value (y a.k.a. result), so that it is never larger than max_y and never smaller than 0.
    //
    //   y
    //   |   /\    /\    /\
    //   |  /  \  /  \  /  \
    //  x|_/____\/____\/____\_
    //
    x     = repeat(x, max_y * 2.0f);
    f32 result = max_y - ABS(x - max_y);
    return result;
}

FUNCDEF inline V3 bezier2(V3 p0, V3 p1, V3 p2, f32 t)
{
    V3 result = lerp(lerp(p0, t, p1), t, lerp(p1, t, p2));
    return result;
}

V4 hsv(f32 h, f32 s, f32 v)
{
    // @Note: From Imgui.
    //
    
    V4 result = {0, 0, 0, 1};
    
    if (s == 0.0f)
    {
        // Gray.
        result = {v, v, v, 1};
        return result;
    }
    
    h     = _mod(h, 1.0f) / (60.0f / 360.0f);
    s32 i = (s32)h;
    f32 f = h - (f32)i;
    f32 p = v * (1.0f - s);
    f32 q = v * (1.0f - s * f);
    f32 t = v * (1.0f - s * (1.0f - f));
    
    f32 r, g, b;
    switch (i)
    {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: default: r = v; g = p; b = q; break;
    }
    
    result = {r, g, b, 1};
    return result;
}

M4x4_Inverse look_at(V3 pos, V3 at, V3 up)
{
    // @Note: This function creates a "view matrix". 
    // The inverse of the view matrix is the camera "model matrix" which we will construct from inputs.
    // The forward of the view matrix is the inverse of the camera model matrix.
    //
    // To understand why we do "-dot(axis, pos)" to invert position, look at HMH EP.362.
    
    ASSERT(length2(pos - at) >= 1e-6f);
    
    // Construct the camera's model matrix axes.
    // (at - pos) is front vector, negate it to get z-axis.
    V3 zaxis = normalize(-(at - pos));
    V3 xaxis = normalize(cross(up, zaxis));
    V3 yaxis = cross(zaxis, xaxis);
    
    M4x4_Inverse result =
    {
        {{ // The transform itself (inverse of camera model matrix).
                { xaxis.x,  xaxis.y,  xaxis.z, -dot(xaxis, pos)},
                { yaxis.x,  yaxis.y,  yaxis.z, -dot(yaxis, pos)},
                { zaxis.x,  zaxis.y,  zaxis.z, -dot(zaxis, pos)},
                {    0.0f,     0.0f,     0.0f, 1.0f}
            }},
        
        {{ // The inverse (camera model matrix).
                { xaxis.x,  yaxis.x, zaxis.x, pos.x},
                { xaxis.y,  yaxis.y, zaxis.y, pos.y},
                { xaxis.z,  yaxis.z, zaxis.z, pos.z},
                {    0.0f,     0.0f,    0.0f,  1.0f}
            }}
    };
    
    //M4x4 identity = result.forward*result.inverse;
    
    return result;
}
M4x4_Inverse perspective(f32 fov_radians, f32 aspect, f32 n, f32 f)
{
    // @Note: Right-handed; NDC z in range [0, 1] (compatible with D3D and Vulkan).
    // For OpenGL you can call glClipControl() to switch between z range [-1, 1] and [0, 1].
    
    ASSERT(aspect != 0);
    
    f32 cotan = 1.0f / _tan(fov_radians * 0.5f);
    f32 x = cotan / aspect;
    f32 y = cotan;
    f32 z = -(f)     / (f - n);
    f32 e = -(f * n) / (f - n);
    M4x4_Inverse result =
    {
        {{ // The transform itself.
                {   x, 0.0f,  0.0f, 0.0f},
                {0.0f,    y,  0.0f, 0.0f},
                {0.0f, 0.0f,     z,    e},
                {0.0f, 0.0f, -1.0f, 0.0f},
            }},
        
        {{ // Its inverse.
                {1.0f/x,   0.0f,    0.0f,  0.0f},
                {  0.0f, 1.0f/y,    0.0f,  0.0f},
                {  0.0f,   0.0f,    0.0f, -1.0f},
                {  0.0f,   0.0f,  1.0f/e,   z/e},
            }},
    };
    
    //M4x4 identity = result.forward*result.inverse;
    
    return result;
}
M4x4_Inverse infinite_perspective(f32 fov_radians, f32 aspect, f32 n)
{
    // @Note: Reversed-z infinite perspective projection. 
    //
    // For this to work, do the following:
    // - Make sure NDC z is in range [0, 1]. Call glClipControl() if using OpenGL.
    // - Create a floating point depth buffer.
    // - Flip depth comparison function to GREATER.
    // - Clear depth buffer to value 0.0f (instead of 1.0f).
    // - Adjust all calculations which rely on the assumption that a normalized post-projection depth of zero lies in front of the viewer
    //
    // @Note: There are more things to keep in mind before using infinite far plane; please read before using:
    // https://iolite-engine.com/blog_posts/reverse_z_cheatsheet
    
    ASSERT(aspect != 0);
    
    f32 cotan = 1.0f / _tan(fov_radians * 0.5f);
    f32 x = cotan / aspect;
    f32 y = cotan;
    M4x4_Inverse result =
    {
        {{ // The transform itself.
                {   x, 0.0f,  0.0f, 0.0f},
                {0.0f,    y,  0.0f, 0.0f},
                {0.0f, 0.0f,  0.0f,    n},
                {0.0f, 0.0f, -1.0f, 0.0f},
            }},
        
        {{ // Its inverse.
                {1.0f/x,   0.0f,    0.0f,  0.0f},
                {  0.0f, 1.0f/y,    0.0f,  0.0f},
                {  0.0f,   0.0f,    0.0f, -1.0f},
                {  0.0f,   0.0f,  1.0f/n,  0.0f},
            }},
    };
    
    //M4x4 identity = result.forward*result.inverse;
    
    return result;
}
M4x4_Inverse orthographic_3d(f32 left, f32 right, f32 bottom, f32 top, f32 n, f32 f)
{
    // @Note: Right-handed; NDC z in range [0, 1] (compatible with D3D and Vulkan).
    // For OpenGL you can call glClipControl() to switch between z range [-1, 1] and [0, 1].
    
    ASSERT(left != right);
    ASSERT(bottom != top);
    
    f32 x  =  2.0f / (right - left);
    f32 y  =  2.0f / (top - bottom);
    f32 z  = -1.0f / (f - n);
    
    f32 tx = -(right + left) / (right - left);
    f32 ty = -(top + bottom) / (top - bottom);
    f32 tz = -(n) / (f - n);
    M4x4_Inverse result = 
    {
        {{ // The transform itself.
                {   x, 0.0f, 0.0f, tx},
                {0.0f,    y, 0.0f, ty},
                {0.0f, 0.0f,    z, tz},
                {0.0f, 0.0f, 0.0f,  1}
            }},
        {{ // Its inverse.
                {1.0f/x,   0.0f,   0.0f, -tx/x},
                {  0.0f, 1.0f/y,   0.0f, -ty/y},
                {  0.0f,   0.0f, 1.0f/z, -tz/z},
                {  0.0f,   0.0f,   0.0f,     1}
            }}
    };
    
    //M4x4 identity = result.forward*result.inverse;
    
    return result;
}
M4x4_Inverse orthographic_2d(f32 left, f32 right, f32 bottom, f32 top)
{
    ASSERT(left != right);
    ASSERT(bottom != top);
    
    f32 x  =  2.0f / (right - left);
    f32 y  =  2.0f / (top - bottom);
    
    f32 tx = -(right + left) / (right - left);
    f32 ty = -(top + bottom) / (top - bottom);
    M4x4_Inverse result = 
    {
        {{ // The transform itself.
                {   x, 0.0f, 0.0f, tx},
                {0.0f,    y, 0.0f, ty},
                {0.0f, 0.0f,-1.0f,  0},
                {0.0f, 0.0f, 0.0f,  1}
            }},
        {{ // Its inverse.
                {1.0f/x,   0.0f,   0.0f, -tx/x},
                {  0.0f, 1.0f/y,   0.0f, -ty/y},
                {  0.0f,   0.0f,  -1.0f,     0},
                {  0.0f,   0.0f,   0.0f,     1}
            }}
    };
    
    //M4x4 identity = result.forward*result.inverse;
    
    return result;
}

void calculate_tangents(V3 normal, V3 *tangent_out, V3 *bitangent_out)
{
    // @Note: This is specific to our coordinate system, of course.
    
    V3 t = cross(V3_Y_AXIS, normal);
    if (length(t) <= 0.001f)
        t = cross(normal, V3_Z_AXIS);
    
    V3 b = cross(normal, t);
    
    *tangent_out   = normalize(t);
    *bitangent_out = normalize(b);
}

#if COMPILER_GCC
#    pragma GCC diagnostic pop
#elif COMPILER_CLANG
#    pragma clang diagnostic pop
#endif

/////////////////////////////////////////
//~
// PRNG Implementation
//
// @Note: From Matrins: https://gist.github.com/mmozeiko/1561361cd4105749f80bb0b9223e9db8
#define PCG_DEFAULT_MULTIPLIER_64 6364136223846793005ULL
#define PCG_DEFAULT_INCREMENT_64  1442695040888963407ULL
FUNCDEF inline Random_PCG random_seed(u64 seed)
{
    Random_PCG result;
    result.state  = 0ULL;
    random_next(&result);
    result.state += seed;
    random_next(&result);
    
    return result;
}
FUNCDEF inline u32 random_next(Random_PCG *rng)
{
    u64 state  = rng->state;
    rng->state = state * PCG_DEFAULT_MULTIPLIER_64 + PCG_DEFAULT_INCREMENT_64;
    
    // XSH-RR
    u32 value = (u32)((state ^ (state >> 18)) >> 27);
    s32 rot   = state >> 59;
    return rot ? (value >> rot) | (value << (32 - rot)) : value;
}
FUNCDEF inline u64 random_next64(Random_PCG *rng)
{
	u64 value = random_next(rng);
	value   <<= 32;
	value    |= random_next(rng);
	return value;
}
FUNCDEF inline f32 random_nextf(Random_PCG *rng)
{
    // @Note: returns float in [0, 1) interval.
    
    u32 x = random_next(rng);
    return (f32)(s32)(x >> 8) * 0x1.0p-24f;
}
FUNCDEF inline f64 random_nextd(Random_PCG *rng)
{
	// @Note: returns double in [0, 1) interval.
    
    u64 x = random_next64(rng);
    return (f64)(s64)(x >> 11) * 0x1.0p-53;
}
FUNCDEF inline u32 random_range(Random_PCG *rng, u32 min, u32 max)
{
    // @Note: Returns value in [min, max) interval.
    
    u32 bound     = max - min;
    u32 threshold = -(s32)bound % bound;
    
    for (;;)
    {
        u32 r = random_next(rng);
        if (r >= threshold)
        {
            return min + (r % bound);
        }
    }
}
FUNCDEF inline f32 random_rangef(Random_PCG *rng, f32 min, f32 max)
{
    // @Note: Returns value in [min, max) interval.
    f32 result = lerp(min, random_nextf(rng), max);
    return result;
}
FUNCDEF inline V2 random_range_v2(Random_PCG *rng, V2 min, V2 max)
{
    // @Note: Returns value in [min, max) interval.
    
    V2 result;
    result.x = lerp(min.x, random_nextf(rng), max.x);
    result.y = lerp(min.y, random_nextf(rng), max.y);
    return result;
}
FUNCDEF inline V3 random_range_v3(Random_PCG *rng, V3 min, V3 max)
{
    // @Note: Returns value in [min, max) interval.
    
    V3 result;
    result.x = lerp(min.x, random_nextf(rng), max.x);
    result.y = lerp(min.y, random_nextf(rng), max.y);
    result.z = lerp(min.z, random_nextf(rng), max.z);
    return result;
}

/////////////////////////////////////////
//~
// Memory Arena Implementation
//
FUNCDEF Arena* arena_init(u64 max_size /*= ARENA_MAX_DEFAULT*/)
{
    Arena *result = 0;
    if (max_size >= ARENA_COMMIT_SIZE) {
        // Reserve additional bytes to account for Arena header since we're storing it inside.
        //
        u32 header_size = ALIGN_UP(sizeof(Arena), 64);
        max_size       += header_size;
        void *memory    = os->reserve(max_size);
        if (os->commit(memory, ARENA_COMMIT_SIZE)) {
            result              = (Arena *)memory;
            result->max         = max_size;
            result->used        = header_size;
            result->commit_used = ARENA_COMMIT_SIZE;
        }
    }
    ASSERT(result != 0);
    return result;
}
FUNCDEF inline void arena_free(Arena *arena)
{
    os->release(arena);
}
FUNCDEF void* arena_push(Arena *arena, u64 size, u64 alignment)
{
    void *result = 0;
    u64 s = ALIGN_UP(arena->used + size, alignment); 
    if (s <= arena->max) {
        if (s > arena->commit_used) {
            // Commit more pages.
            u64 commit_size     = (size + (ARENA_COMMIT_SIZE-1));
            commit_size        -= (commit_size % ARENA_COMMIT_SIZE);
            if (os->commit(((u8*)arena) + arena->commit_used, commit_size))
                arena->commit_used += commit_size;
        }
        
        if (s <= arena->commit_used) {
            result      = ((u8*)arena) + arena->used;
            arena->used = s;
        }
    }
    ASSERT(result != 0);
    return result;
}
FUNCDEF inline void* arena_push_zero(Arena *arena, u64 size, u64 alignment)
{
    void *result = arena_push(arena, size, alignment);
    MEMORY_ZERO(result, size);
    return result;
}
FUNCDEF inline void arena_pop(Arena *arena, u64 size)
{
    // @Todo: Decommit memory.
    
    // @Note: Make sure we don't clear arena details/header by accident.
    u64 header_size = ALIGN_UP(sizeof(Arena), 64);
    size = CLAMP_UPPER(arena->used - header_size, size);
    arena->used -= size;
}
FUNCDEF inline void arena_reset(Arena *arena)
{
    arena_pop(arena, arena->used);
}
FUNCDEF inline Arena_Temp arena_temp_begin(Arena *arena)
{
    Arena_Temp result = {arena, arena->used};
    return result;
}
FUNCDEF inline void arena_temp_end(Arena_Temp temp)
{
    if (temp.arena->used >= temp.used)
        temp.arena->used = temp.used;
}

threadvar Arena *scratch_pool[ARENA_SCRATCH_COUNT] = {};
Arena_Temp get_scratch(Arena **conflict_array, s32 count)
{
    // Initialize arenas on first visit.
    if (scratch_pool[0] == 0) {
        for (s32 i = 0; i < ARENA_SCRATCH_COUNT; i++)
            scratch_pool[i] = arena_init();
    }
    
    // Get non-conflicting arena.
    Arena_Temp result = {};
    for (s32 i = 0; i < ARENA_SCRATCH_COUNT; i++) {
        b32 is_used = FALSE;
        for (s32 j = 0; j < count; j++) {
            if (scratch_pool[i] == conflict_array[j]) {
                is_used = TRUE;
                break;
            }
        }
        
        if (!is_used) {
            result = arena_temp_begin(scratch_pool[i]);
            break;
        }
    }
    
    return result;
}

/////////////////////////////////////////
//~
// String8 Implementation
//
FUNCDEF String8 str8(u8 *data, u64 count)
{ 
    String8 result = {data, count};
    return result;
}
FUNCDEF String8 str8_cstring(const char *c_string)
{
    String8 result;
    result.data  = (u8 *) c_string;
    result.count = str8_length(c_string);
    return result;
}
FUNCDEF u64 str8_length(const char *c_string)
{
    u64 result = 0;
    while (*c_string++) result++;
    return result;
}
FUNCDEF String8 str8_copy(Arena *arena, String8 s)
{
    String8 result;
    result.count = s.count;
    result.data  = PUSH_ARRAY(arena, u8, result.count + 1);
    MEMORY_COPY(result.data, s.data, result.count);
    result.data[result.count] = 0;
    return result;
}
FUNCDEF String8 str8_cat(Arena *arena, String8 a, String8 b)
{
    String8 result;
    result.count = a.count + b.count;
    result.data  = PUSH_ARRAY(arena, u8, result.count + 1);
    MEMORY_COPY(result.data, a.data, a.count);
    MEMORY_COPY(result.data + a.count, b.data, b.count);
    result.data[result.count] = 0;
    return result;
}
FUNCDEF b32 str8_empty(String8 s)
{
    b32 result = (!s.data || !s.count);
    return result;
}
FUNCDEF b32 str8_match(String8 a, String8 b)
{
    if (a.count != b.count) return FALSE;
    for(u64 i = 0; i < a.count; i++)
    {
        if (a.data[i] != b.data[i]) return FALSE;
    }
    return TRUE;
}

/////////////////////////////////////////
//~
// String Helper Functions Implementation
//

//~ String ctor helpers
// @Note: Assumes passed strings are immutable! We won't allocate memory for the result string!
FUNCDEF String8 str8_skip(String8 str, u64 amount)
{
    amount         = CLAMP_UPPER(str.count, amount);
    u64 remaining  = str.count - amount;
    String8 result = {str.data + amount, remaining};
    return result;
}

//~ String character helpers
FUNCDEF inline b32 is_spacing(char c)
{
    b32 result = ((c == ' ')  ||
                  (c == '\t') ||
                  (c == '\v') ||
                  (c == '\f'));
    return result;
}
FUNCDEF inline b32 is_end_of_line(char c)
{
    b32 result = ((c == '\n') ||
                  (c == '\r'));
    return result;
}
FUNCDEF inline b32 is_whitespace(char c)
{
    b32 result = (is_spacing(c) || is_end_of_line(c));
    return result;
}
FUNCDEF inline b32 is_alpha(char c)
{
    b32 result = (((c >= 'A') && (c <= 'Z')) ||
                  ((c >= 'a') && (c <= 'z')));
    return result;
}
FUNCDEF inline b32 is_numeric(char c)
{
    b32 result = (((c >= '0') && (c <= '9')));
    return result;
}
FUNCDEF inline b32 is_alphanumeric(char c)
{
    b32 result = (is_alpha(c) || is_numeric(c));
    return result;
}
FUNCDEF inline b32 is_slash(char c)
{
    b32 result = ((c == '\\') || (c == '/'));
    return result;
}

//~ String path helpers
// @Note: Assumes passed strings are immutable! We won't allocate memory for the result string!
FUNCDEF String8 chop_extension(String8 path)
{
    String8 result = path;
    if (path.count <= 0)
        return result;
    
    u64 last_period_pos = path.count;
    for (s64 i = path.count - 1; i >= 0 ; i--) {
        if (path[i] == '.') {
            last_period_pos = i;
            break;
        }
    }
    
    result.count = last_period_pos;
    return result;
}
FUNCDEF String8 extract_file_name(String8 path)
{
    // Result includes extension.
    String8 result = path;
    if (path.count <= 0)
        return result;
    
    s64 last_slash_pos = -1;
    for (s64 i = path.count - 1; i >= 0; i--) {
        if (is_slash(path[i])) {
            last_slash_pos = i;
            break;
        }
    }
    
    result = str8_skip(path, last_slash_pos + 1);
    return result;
}
FUNCDEF String8 extract_base_name(String8 path)
{
    // Result doesn't include extension.
    String8 result = extract_file_name(path);
    result         = chop_extension(result);
    return result;
}
FUNCDEF String8 extract_parent_folder(String8 path)
{
    // Result includes extension.
    String8 result = path;
    if (path.count <= 0)
        return result;
    
    u64 last_slash_pos = path.count;
    for (s64 i = path.count - 1; i >= 0; i--) {
        if (is_slash(path[i])) {
            last_slash_pos = i;
            break;
        }
    }
    
    // (+ 1) to include the slash.
    result.count = last_slash_pos + 1;
    return result;
}

//~ String misc helpers
FUNCDEF inline void advance(String8 *s, u64 count)
{
    count     = CLAMP_UPPER(s->count, count);
    s->data  += count;
    s->count -= count;
}
FUNCDEF inline void get(String8 *s, void *data, u64 size)
{
    ASSERT(size <= s->count);
    
    MEMORY_COPY(data, s->data, size);
    advance(s, size);
}
FUNCDEF u32 get_hash(String8 s)
{
    // djb2 hash from http://www.cse.yorku.ca/~oz/hash.html
    
    u32 hash = 5381;
    
    for(s32 i = 0; i < s.count; i++)
    {
        /* hash * 33 + s[i] */
        hash = ((hash << 5) + hash) + *(s.data + i);
    }
    
    return hash;
}

/////////////////////////////////////////
//~
// String Format Implementation
//
GLOBAL char decimal_digits[]   = "0123456789";
GLOBAL char lower_hex_digits[] = "0123456789abcdef";
GLOBAL char upper_hex_digits[] = "0123456789ABCDEF";

FUNCDEF void put_char(String8 *dest, char c)
{
    if (dest->count) {
        dest->count--;
        *dest->data++ = c;
    }
}
FUNCDEF void put_c_string(String8 *dest, const char *c_string)
{
    while (*c_string) put_char(dest, *c_string++);
}
FUNCDEF void u64_to_ascii(String8 *dest, u64 value, u32 base, char *digits)
{
    ASSERT(base != 0);
    
    u8 *start = dest->data;
    do
    {
        put_char(dest, digits[value % base]);
        value /= base;
    } while (value != 0);
    u8 *end  = dest->data;
    
    // Reverse.
    while (start < end) {
        end--;
        SWAP(*end, *start, char);
        start++;
    }
}
FUNCDEF void f64_to_ascii(String8 *dest, f64 value, u32 precision)
{
    if (value < 0) {
        put_char(dest, '-');
        value = -value;
    }
    
    u64 integer_part = (u64) value;
    value           -= (f64) integer_part;
    
    u64_to_ascii(dest, integer_part, 10, decimal_digits);
    put_char(dest, '.');
    
    for(u32 precision_index = 0;
        precision_index < precision;
        precision_index++)
    {
        value       *= 10.0f;
        integer_part = (u64) value;
        value       -= (f64) integer_part;
        put_char(dest, decimal_digits[integer_part]);
    }
}
FUNCDEF u64 ascii_to_u64(char **at)
{
    u64 result = 0;
    char *tmp = *at;
    while (is_numeric(*tmp)) {
        result *= 10;
        result += *tmp - '0';
        tmp++;
    }
    *at = tmp;
    return result;
}
FUNCDEF u64 string_format_list(char *dest_start, u64 dest_count, const char *format, va_list arg_list)
{
    if (!dest_count) return 0;
    
    String8 dest_buffer = str8((u8*)dest_start, dest_count);
    
    char *at = (char *) format;
    while (*at) {
        if (*at == '%') {
            at++;
            
            b32 use_precision = FALSE;
            u32 precision     = 0;
            
            // Handle precision specifier.
            if (*at == '.') {
                at++;
                
                if (is_numeric(*at)) {
                    use_precision = TRUE;
                    precision     = (u32) ascii_to_u64(&at); // at is advanced inside the function.
                } else ASSERT(!"Invalid precision specifier!");
            }
            if (!use_precision) precision = 6;
            
            switch (*at)  {
                case 'b': {
                    b32 b = (b32) va_arg(arg_list, b32);
                    if (b) put_c_string(&dest_buffer, "True");
                    else   put_c_string(&dest_buffer, "False");
                } break;
                
                case 'c': {
                    char c = (char) va_arg(arg_list, s32);
                    put_char(&dest_buffer, c);
                } break;
                
                case 's': {
                    char *c_str = va_arg(arg_list, char *);
                    put_c_string(&dest_buffer, c_str);
                } break;
                
                case 'S': {
                    String8 s = va_arg(arg_list, String8);
                    for(u64 i = 0; i < s.count; i++) put_char(&dest_buffer, s.data[i]);
                } break;
                
                case 'i':
                case 'd': {
                    s64 value = (s64) va_arg(arg_list, s32);
                    
                    if (value < 0)  {
                        put_char(&dest_buffer, '-');
                        value = -value;
                    }
                    
                    u64_to_ascii(&dest_buffer, (u64)value, 10, decimal_digits);
                } break;
                
                case 'u': {
                    u64 value = (u64) va_arg(arg_list, u32);
                    u64_to_ascii(&dest_buffer, value, 10, decimal_digits);
                } break;
                
                case 'U': {
                    u64 value = va_arg(arg_list, u64);
                    u64_to_ascii(&dest_buffer, value, 10, decimal_digits);
                } break;
                
                case 'm': {
                    umm value = va_arg(arg_list, umm);
                    const char *suffix = "B";
                    
                    // Round up.
                    if (value >= KILOBYTES(1)) {
                        suffix = "KB";
                        value = (value + KILOBYTES(1) - 1) / KILOBYTES(1);
                    } else if (value >= MEGABYTES(1)) {
                        suffix = "MB";
                        value = (value + MEGABYTES(1) - 1) / MEGABYTES(1);
                    } else if (value >= GIGABYTES(1)) {
                        suffix = "GB";
                        value = (value + GIGABYTES(1) - 1) / GIGABYTES(1);
                    }
                    
                    u64_to_ascii(&dest_buffer, value, 10, decimal_digits);
                    put_c_string(&dest_buffer, suffix);
                } break;
                
                case 'f': {
                    f64 value = va_arg(arg_list, f64);
                    f64_to_ascii(&dest_buffer, value, precision);
                } break;
                
                case 'v': {
                    if ((at[1] != '2') && 
                        (at[1] != '3') && 
                        (at[1] != '4'))
                        ASSERT(!"Unrecognized vector specifier!");
                    
                    s32 vector_size = (s32)(at[1] - '0');
                    V4 v = {};
                    
                    if (vector_size == 2) {
                        V2 t = va_arg(arg_list, V2);
                        v    = {t.x, t.y, 0.0f, 0.0f};
                    } else if (vector_size == 3) {
                        V3 t = va_arg(arg_list, V3);
                        v    = {t.x, t.y, t.z, 0.0f};
                    } else if (vector_size == 4) {
                        V4 t = va_arg(arg_list, V4);
                        v    = t;
                    } else {
                        ASSERT(!"Unrecognized vector specifier!");
                    }
                    
                    put_char(&dest_buffer, '[');
                    for(s32 i = 0; i < vector_size; i++)
                    {
                        f64_to_ascii(&dest_buffer, v.I[i], precision);
                        if (i < (vector_size - 1)) put_c_string(&dest_buffer, ", ");
                    }
                    put_char(&dest_buffer, ']');
                    
                    at++;
                } break;
                
                case '%': {
                    put_char(&dest_buffer, '%');
                } break;
                
                default:
                {
                    ASSERT(!"Unrecognized format specifier!");
                } break;
            }
            
            if (*at) at++;
        } else {
            put_char(&dest_buffer, *at++);
        }
    }
    
    u64 result = dest_buffer.data - (u8*)dest_start;
    
    // Add null terminator.
    if (dest_buffer.count) 
        dest_buffer.data[0] = 0;
    else {
        dest_buffer.data[-1] = 0;
        result--;
    }
    
    return result;
}
FUNCDEF u64 string_format(char *dest_start, u64 dest_count, const char *format, ...)
{
    va_list arg_list;
    va_start(arg_list, format);
    u64 result = string_format_list(dest_start, dest_count, format, arg_list);
    va_end(arg_list);
    
    return result;
}
FUNCDEF void debug_print(const char *format, ...)
{
    char temp[4096];
    String8 s = {};
    
    va_list arg_list;
    va_start(arg_list, format);
    s.data  = (u8 *) temp;
    s.count = string_format_list(temp, sizeof(temp), format, arg_list);
    va_end(arg_list);
    
    os->print_to_debug_output(s);
}
FUNCDEF String8 sprint(Arena *arena, const char *format, ...)
{
    char temp[4096];
    String8 s = {};
    
    va_list arg_list;
    va_start(arg_list, format);
    s.data  = (u8 *) temp;
    s.count = string_format_list(temp, sizeof(temp), format, arg_list);
    va_end(arg_list);
    
    String8 result = str8_copy(arena, s);
    return result;
}

/////////////////////////////////////////
//~
// String Builder Implementation
//
FUNCDEF String_Builder sb_init(u64 capacity /*= SB_BLOCK_SIZE*/)
{
    String_Builder builder = {};
    builder.arena    = arena_init();
    builder.capacity = capacity;
    sb_reset(&builder);
    return builder;
}
FUNCDEF void sb_free(String_Builder *builder)
{
    arena_free(builder->arena);
}
FUNCDEF void sb_reset(String_Builder *builder)
{
    arena_reset(builder->arena);
    builder->length = 0;
    builder->start  = PUSH_ARRAY(builder->arena, u8, builder->capacity);
    builder->buffer = {builder->start, builder->capacity};
}
FUNCDEF void sb_append(String_Builder *builder, void *data, u64 size)
{
    if ((builder->length + size) > builder->capacity) {
        builder->capacity += (size + (SB_BLOCK_SIZE-1));
        builder->capacity -= (builder->capacity % SB_BLOCK_SIZE);
        u64 len = builder->length;
        sb_reset(builder);
        builder->length = len;
        advance(&builder->buffer, builder->length);
    }
    
    MEMORY_COPY(builder->buffer.data, data, size);
    advance(&builder->buffer, size);
    
    builder->length = builder->buffer.data - builder->start;
}
FUNCDEF void sb_appendf(String_Builder *builder, char *format, ...)
{
    char temp[4096];
    
    va_list arg_list;
    va_start(arg_list, format);
    u64 size = string_format_list(temp, sizeof(temp), format, arg_list);
    va_end(arg_list);
    
    sb_append(builder, temp, size);
}
FUNCDEF String8 sb_to_string(String_Builder *builder, Arena *arena)
{
    // Add null terminator.
    if (builder->buffer.count)
        builder->buffer.data[0] = 0;
    else {
        builder->buffer.data[-1] = 0;
        builder->length--;
    }
    
    String8 result = str8_copy(arena, str8(builder->start, builder->length));
    return result;
}

/////////////////////////////////////////
//
// Dynamic Array Implementation
//
// In the header part of this file.

/////////////////////////////////////////
//
// Hash Table Implementation
//
// In the header part of this file.

/////////////////////////////////////////
//~
// Sound Implementation
//
FUNCDEF void sound_update(Sound *sound, u32 samples_to_advance)
{
    sound->pos += samples_to_advance;
    if (sound->loop)
        sound->pos %= sound->count;
    else
        sound->pos = MIN(sound->pos, sound->count);
}
FUNCDEF void sound_mix(Sound *sound, f32 volume, f32 *samples_out, u32 samples_to_write)
{
    const s16 *s_samples = sound->samples;
    u32 s_count          = sound->count;
    u32 s_pos            = sound->pos;
    f32 s_volume         = sound->volume;
    b32 s_loop           = sound->loop;
    
    for (u32 i = 0; i < samples_to_write; i++) {
        if (s_loop) {
            if (s_pos == s_count)
                s_pos = 0;
        } else {
            if (s_pos >= s_count)
                break;
        }
        
        f32 sample      = s_samples[s_pos++] * (1.f / 32768.f);
        samples_out[0] += volume * s_volume * sample;
        samples_out[1] += volume * s_volume * sample;
        samples_out    += 2;
    }
}

/////////////////////////////////////////
//~
// OS Implementation
//
OS_State *os = 0;

FUNCDEF b32 key_pressed(Input_State *input, s32 key, b32 capture /* = FALSE */)
{
    b32 result = input->pressed[key];
    if (result && capture)
        input->pressed[key] = FALSE;
    return result;
}
FUNCDEF b32 key_held(Input_State *input, s32 key, b32 capture /* = FALSE */)
{
    b32 result = input->held[key];
    if (result && capture)
        input->pressed[key] = FALSE;
    return result;
}
FUNCDEF b32 key_released(Input_State *input, s32 key, b32 capture /* = FALSE */)
{
    b32 result = input->released[key];
    if (result && capture)
        input->released[key] = FALSE;
    return result;
}
FUNCDEF inline void reset_input(Input_State *input)
{
    // @Note: Call this after input usage code (frame end or end of accum loop).
    //
    // Key held state persists across frames.
    MEMORY_ZERO_ARRAY(input->pressed);
    MEMORY_ZERO_ARRAY(input->released);
    input->mouse_delta_raw = {};
    input->mouse_delta     = {};
    input->mouse_wheel_raw = {};
}
FUNCDEF void clear_input(Input_State *input)
{
    // @Note: Call this when app changes focus (like WM_ACTIVATE and WM_ACTIVATEAPP on windows)
    //
    // Clear everything.
    MEMORY_ZERO_ARRAY(input->pressed);
    MEMORY_ZERO_ARRAY(input->held);
    MEMORY_ZERO_ARRAY(input->released);
    input->mouse_delta_raw = {};
    input->mouse_delta     = {};
    input->mouse_wheel_raw = {};
}

FUNCDEF Rect2 aspect_ratio_fit(V2u render_dim, V2u window_dim)
{
    // @Note: From Handmade Hero E.322.
    
    Rect2 result = {};
    f32 rw = (f32)render_dim.w;
    f32 rh = (f32)render_dim.h;
    f32 ww = (f32)window_dim.w;
    f32 wh = (f32)window_dim.h;
    
    if ((rw > 0) && (rh > 0) && (ww > 0) && (wh > 0)) {
        f32 optimal_ww = wh * (rw / rh);
        f32 optimal_wh = ww * (rh / rw);
        
        if (optimal_ww > ww) {
            // Width-constrained display (top and bottom black bars).
            result.min.x = 0;
            result.max.x = ww;
            
            f32 empty_space = wh - optimal_wh;
            f32 half_empty  = _round(0.5f*empty_space);
            f32 used_height = _round(optimal_wh);
            
            result.min.y = half_empty; 
            result.max.y = result.min.y + used_height;
        } else {
            // Height-constrained display (left and right black bars).
            result.min.y = 0;
            result.max.y = wh;
            
            f32 empty_space = ww - optimal_ww;
            f32 half_empty  = _round(0.5f*empty_space);
            f32 used_height = _round(optimal_ww);
            
            result.min.x = half_empty; 
            result.max.x = result.min.x + used_height;
        }
    }
    
    return result;
}
FUNCDEF V3 unproject(V3 camera_position, f32 Zworld_distance_from_camera,
                     V3 mouse_ndc, M4x4_Inverse world_to_view, M4x4_Inverse view_to_proj)
{
    // @Note: Handmade Hero EP.373 and EP.374
    
    V3 camera_zaxis = get_z_axis(world_to_view.inverse);
    V3 new_p        = camera_position - Zworld_distance_from_camera*camera_zaxis;
    V4 probe_z      = v4(new_p, 1.0f);
    
    // Get probe_z in clip space.
    probe_z = view_to_proj.forward*world_to_view.forward*probe_z;
    
    // Undo the perspective divide.
    mouse_ndc.x *= probe_z.w;
    mouse_ndc.y *= probe_z.w;
    
    V4 mouse_clip = v4(mouse_ndc.x, mouse_ndc.y, probe_z.z, probe_z.w);
    V3 result     = (world_to_view.inverse*view_to_proj.inverse*mouse_clip).xyz;
    
    return result;
}

#endif // ORH_IMPLEMENTATION
