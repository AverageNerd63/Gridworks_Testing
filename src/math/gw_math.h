#pragma once
#include <math.h>
#include "../core/types.h"

/* ---- Types ------------------------------------------------------------ */

typedef union { struct { f32 x, y; };       f32 v[2]; } Vec2;
typedef union { struct { f32 x, y, z; };    f32 v[3]; } Vec3;
typedef union { struct { f32 x, y, z, w; }; f32 v[4]; } Vec4;
typedef union { struct { f32 x, y, z, w; }; f32 v[4]; } Quat;

/* Column-major, matches Vulkan/GLSL layout */
typedef struct { f32 m[4][4]; } Mat4;

/* ---- Constants -------------------------------------------------------- */

#define GW_PI      3.14159265358979323846f
#define GW_DEG2RAD (GW_PI / 180.0f)
#define GW_RAD2DEG (180.0f / GW_PI)

/* ---- Vec2 ------------------------------------------------------------- */

static inline Vec2 vec2(f32 x, f32 y)            { return (Vec2){{x, y}}; }
static inline Vec2 vec2_add(Vec2 a, Vec2 b)       { return (Vec2){{a.x+b.x, a.y+b.y}}; }
static inline Vec2 vec2_sub(Vec2 a, Vec2 b)       { return (Vec2){{a.x-b.x, a.y-b.y}}; }
static inline Vec2 vec2_scale(Vec2 a, f32 s)      { return (Vec2){{a.x*s, a.y*s}}; }
static inline f32  vec2_dot(Vec2 a, Vec2 b)       { return a.x*b.x + a.y*b.y; }
static inline f32  vec2_len(Vec2 a)               { return sqrtf(vec2_dot(a, a)); }
static inline Vec2 vec2_norm(Vec2 a)              { f32 l = vec2_len(a); return l ? vec2_scale(a, 1.0f/l) : a; }

/* ---- Vec3 ------------------------------------------------------------- */

static inline Vec3 vec3(f32 x, f32 y, f32 z)     { return (Vec3){{x, y, z}}; }
static inline Vec3 vec3_add(Vec3 a, Vec3 b)       { return (Vec3){{a.x+b.x, a.y+b.y, a.z+b.z}}; }
static inline Vec3 vec3_sub(Vec3 a, Vec3 b)       { return (Vec3){{a.x-b.x, a.y-b.y, a.z-b.z}}; }
static inline Vec3 vec3_scale(Vec3 a, f32 s)      { return (Vec3){{a.x*s, a.y*s, a.z*s}}; }
static inline f32  vec3_dot(Vec3 a, Vec3 b)       { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline f32  vec3_len(Vec3 a)               { return sqrtf(vec3_dot(a, a)); }
static inline Vec3 vec3_norm(Vec3 a)              { f32 l = vec3_len(a); return l ? vec3_scale(a, 1.0f/l) : a; }
static inline Vec3 vec3_lerp(Vec3 a, Vec3 b, f32 t) {
    return vec3_add(vec3_scale(a, 1.0f - t), vec3_scale(b, t));
}
static inline Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return (Vec3){{
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    }};
}

/* ---- Vec4 ------------------------------------------------------------- */

static inline Vec4 vec4(f32 x, f32 y, f32 z, f32 w) { return (Vec4){{x, y, z, w}}; }
static inline Vec4 vec4_add(Vec4 a, Vec4 b)  { return (Vec4){{a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w}}; }
static inline Vec4 vec4_scale(Vec4 a, f32 s) { return (Vec4){{a.x*s, a.y*s, a.z*s, a.w*s}}; }

/* ---- Mat4 ------------------------------------------------------------- */

Mat4 mat4_identity(void);
Mat4 mat4_mul(Mat4 a, Mat4 b);
Mat4 mat4_transpose(Mat4 m);
Mat4 mat4_translate(Vec3 t);
Mat4 mat4_scale_v(Vec3 s);
Mat4 mat4_rotate(Vec3 axis, f32 radians);
Mat4 mat4_perspective(f32 fov_y_rad, f32 aspect, f32 near, f32 far);
Mat4 mat4_look_at(Vec3 eye, Vec3 center, Vec3 up);
Vec4 mat4_mul_vec4(Mat4 m, Vec4 v);
Mat4 mat4_invert(Mat4 m);

/* ---- Quat ------------------------------------------------------------- */

Quat quat_identity(void);
Quat quat_from_axis_angle(Vec3 axis, f32 radians);
Quat quat_mul(Quat a, Quat b);
Quat quat_norm(Quat q);
Quat quat_slerp(Quat a, Quat b, f32 t);
Mat4 quat_to_mat4(Quat q);
Vec3 quat_rotate_vec3(Quat q, Vec3 v);