#include "gw_math.h"

/* ---- Mat4 ------------------------------------------------------------- */

Mat4 mat4_identity(void) {
    Mat4 m = {0};
    m.m[0][0] = m.m[1][1] = m.m[2][2] = m.m[3][3] = 1.0f;
    return m;
}

Mat4 mat4_mul(Mat4 a, Mat4 b) {
    Mat4 r = {0};
    for (int col = 0; col < 4; col++)
        for (int row = 0; row < 4; row++)
            for (int k = 0; k < 4; k++)
                r.m[col][row] += a.m[k][row] * b.m[col][k];
    return r;
}

Mat4 mat4_transpose(Mat4 m) {
    Mat4 r;
    for (int c = 0; c < 4; c++)
        for (int r2 = 0; r2 < 4; r2++)
            r.m[c][r2] = m.m[r2][c];
    return r;
}

Mat4 mat4_translate(Vec3 t) {
    Mat4 m = mat4_identity();
    m.m[3][0] = t.x;
    m.m[3][1] = t.y;
    m.m[3][2] = t.z;
    return m;
}

Mat4 mat4_scale_v(Vec3 s) {
    Mat4 m = mat4_identity();
    m.m[0][0] = s.x;
    m.m[1][1] = s.y;
    m.m[2][2] = s.z;
    return m;
}

Mat4 mat4_rotate(Vec3 axis, f32 rad) {
    Vec3  a = vec3_norm(axis);
    f32   c = cosf(rad), s = sinf(rad), t = 1.0f - c;
    Mat4  m = mat4_identity();
    m.m[0][0] = t*a.x*a.x + c;
    m.m[0][1] = t*a.x*a.y + s*a.z;
    m.m[0][2] = t*a.x*a.z - s*a.y;
    m.m[1][0] = t*a.x*a.y - s*a.z;
    m.m[1][1] = t*a.y*a.y + c;
    m.m[1][2] = t*a.y*a.z + s*a.x;
    m.m[2][0] = t*a.x*a.z + s*a.y;
    m.m[2][1] = t*a.y*a.z - s*a.x;
    m.m[2][2] = t*a.z*a.z + c;
    return m;
}

Mat4 mat4_perspective(f32 fov_y_rad, f32 aspect, f32 near, f32 far) {
    f32  f = 1.0f / tanf(fov_y_rad * 0.5f);
    Mat4 m = {0};
    m.m[0][0] =  f / aspect;
    m.m[1][1] =  f;
    m.m[2][2] =  far / (near - far);           /* Vulkan depth [0,1] */
    m.m[2][3] = -1.0f;
    m.m[3][2] =  (near * far) / (near - far);
    return m;
}

Mat4 mat4_look_at(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = vec3_norm(vec3_sub(center, eye));
    Vec3 r = vec3_norm(vec3_cross(f, up));
    Vec3 u = vec3_cross(r, f);
    Mat4 m = mat4_identity();
    m.m[0][0] =  r.x; m.m[1][0] =  r.y; m.m[2][0] =  r.z;
    m.m[0][1] =  u.x; m.m[1][1] =  u.y; m.m[2][1] =  u.z;
    m.m[0][2] = -f.x; m.m[1][2] = -f.y; m.m[2][2] = -f.z;
    m.m[3][0] = -vec3_dot(r, eye);
    m.m[3][1] = -vec3_dot(u, eye);
    m.m[3][2] =  vec3_dot(f, eye);
    return m;
}

Vec4 mat4_mul_vec4(Mat4 m, Vec4 v) {
    return (Vec4){{
        m.m[0][0]*v.x + m.m[1][0]*v.y + m.m[2][0]*v.z + m.m[3][0]*v.w,
        m.m[0][1]*v.x + m.m[1][1]*v.y + m.m[2][1]*v.z + m.m[3][1]*v.w,
        m.m[0][2]*v.x + m.m[1][2]*v.y + m.m[2][2]*v.z + m.m[3][2]*v.w,
        m.m[0][3]*v.x + m.m[1][3]*v.y + m.m[2][3]*v.z + m.m[3][3]*v.w,
    }};
}

Mat4 mat4_invert(Mat4 m) {
    /* flatten to row-major for cofactor computation */
    f32 a[16];
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++)
            a[r*4+c] = m.m[c][r];

    f32 inv[16];
    inv[ 0]=  a[5]*a[10]*a[15]-a[5]*a[11]*a[14]-a[9]*a[6]*a[15]+a[9]*a[7]*a[14]+a[13]*a[6]*a[11]-a[13]*a[7]*a[10];
    inv[ 4]= -a[4]*a[10]*a[15]+a[4]*a[11]*a[14]+a[8]*a[6]*a[15]-a[8]*a[7]*a[14]-a[12]*a[6]*a[11]+a[12]*a[7]*a[10];
    inv[ 8]=  a[4]*a[9] *a[15]-a[4]*a[11]*a[13]-a[8]*a[5]*a[15]+a[8]*a[7]*a[13]+a[12]*a[5]*a[11]-a[12]*a[7]*a[9];
    inv[12]= -a[4]*a[9] *a[14]+a[4]*a[10]*a[13]+a[8]*a[5]*a[14]-a[8]*a[6]*a[13]-a[12]*a[5]*a[10]+a[12]*a[6]*a[9];
    inv[ 1]= -a[1]*a[10]*a[15]+a[1]*a[11]*a[14]+a[9]*a[2]*a[15]-a[9]*a[3]*a[14]-a[13]*a[2]*a[11]+a[13]*a[3]*a[10];
    inv[ 5]=  a[0]*a[10]*a[15]-a[0]*a[11]*a[14]-a[8]*a[2]*a[15]+a[8]*a[3]*a[14]+a[12]*a[2]*a[11]-a[12]*a[3]*a[10];
    inv[ 9]= -a[0]*a[9] *a[15]+a[0]*a[11]*a[13]+a[8]*a[1]*a[15]-a[8]*a[3]*a[13]-a[12]*a[1]*a[11]+a[12]*a[3]*a[9];
    inv[13]=  a[0]*a[9] *a[14]-a[0]*a[10]*a[13]-a[8]*a[1]*a[14]+a[8]*a[2]*a[13]+a[12]*a[1]*a[10]-a[12]*a[2]*a[9];
    inv[ 2]=  a[1]*a[6] *a[15]-a[1]*a[7] *a[14]-a[5]*a[2]*a[15]+a[5]*a[3]*a[14]+a[13]*a[2]*a[7] -a[13]*a[3]*a[6];
    inv[ 6]= -a[0]*a[6] *a[15]+a[0]*a[7] *a[14]+a[4]*a[2]*a[15]-a[4]*a[3]*a[14]-a[12]*a[2]*a[7] +a[12]*a[3]*a[6];
    inv[10]=  a[0]*a[5] *a[15]-a[0]*a[7] *a[13]-a[4]*a[1]*a[15]+a[4]*a[3]*a[13]+a[12]*a[1]*a[7] -a[12]*a[3]*a[5];
    inv[14]= -a[0]*a[5] *a[14]+a[0]*a[6] *a[13]+a[4]*a[1]*a[14]-a[4]*a[2]*a[13]-a[12]*a[1]*a[6] +a[12]*a[2]*a[5];
    inv[ 3]= -a[1]*a[6] *a[11]+a[1]*a[7] *a[10]+a[5]*a[2]*a[11]-a[5]*a[3]*a[10]-a[9] *a[2]*a[7] +a[9] *a[3]*a[6];
    inv[ 7]=  a[0]*a[6] *a[11]-a[0]*a[7] *a[10]-a[4]*a[2]*a[11]+a[4]*a[3]*a[10]+a[8] *a[2]*a[7] -a[8] *a[3]*a[6];
    inv[11]= -a[0]*a[5] *a[11]+a[0]*a[7] *a[9] +a[4]*a[1]*a[11]-a[4]*a[3]*a[9] -a[8] *a[1]*a[7] +a[8] *a[3]*a[5];
    inv[15]=  a[0]*a[5] *a[10]-a[0]*a[6] *a[9] -a[4]*a[1]*a[10]+a[4]*a[2]*a[9] +a[8] *a[1]*a[6] -a[8] *a[2]*a[5];

    f32 det = a[0]*inv[0] + a[1]*inv[4] + a[2]*inv[8] + a[3]*inv[12];
    if (det == 0.0f) return mat4_identity();
    det = 1.0f / det;

    Mat4 res;
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++)
            res.m[c][r] = inv[r*4+c] * det;
    return res;
}

/* ---- Quat ------------------------------------------------------------- */

Quat quat_identity(void) {
    return (Quat){{0.0f, 0.0f, 0.0f, 1.0f}};
}

Quat quat_from_axis_angle(Vec3 axis, f32 rad) {
    f32 s = sinf(rad * 0.5f);
    return (Quat){{ axis.x*s, axis.y*s, axis.z*s, cosf(rad * 0.5f) }};
}

Quat quat_mul(Quat a, Quat b) {
    return (Quat){{
        a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
        a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
        a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w,
        a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z,
    }};
}

Quat quat_norm(Quat q) {
    f32 l = sqrtf(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
    return l ? (Quat){{q.x/l, q.y/l, q.z/l, q.w/l}} : quat_identity();
}

Quat quat_slerp(Quat a, Quat b, f32 t) {
    f32 dot = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
    if (dot < 0.0f) {
        b = (Quat){{-b.x,-b.y,-b.z,-b.w}};
        dot = -dot;
    }
    if (dot > 0.9995f) {
        Quat r = {{
            a.x + t*(b.x-a.x), a.y + t*(b.y-a.y),
            a.z + t*(b.z-a.z), a.w + t*(b.w-a.w)
        }};
        return quat_norm(r);
    }
    f32 theta0 = acosf(dot);
    f32 theta  = theta0 * t;
    f32 sa = sinf(theta0), sb = sinf(theta0 - theta);
    f32 sc = sinf(theta);
    return (Quat){{
        (a.x*sb + b.x*sc) / sa,
        (a.y*sb + b.y*sc) / sa,
        (a.z*sb + b.z*sc) / sa,
        (a.w*sb + b.w*sc) / sa,
    }};
}

Mat4 quat_to_mat4(Quat q) {
    f32 xx=q.x*q.x, yy=q.y*q.y, zz=q.z*q.z;
    f32 xy=q.x*q.y, xz=q.x*q.z, yz=q.y*q.z;
    f32 wx=q.w*q.x, wy=q.w*q.y, wz=q.w*q.z;
    Mat4 m = mat4_identity();
    m.m[0][0]=1-2*(yy+zz); m.m[0][1]=2*(xy+wz);   m.m[0][2]=2*(xz-wy);
    m.m[1][0]=2*(xy-wz);   m.m[1][1]=1-2*(xx+zz); m.m[1][2]=2*(yz+wx);
    m.m[2][0]=2*(xz+wy);   m.m[2][1]=2*(yz-wx);   m.m[2][2]=1-2*(xx+yy);
    return m;
}

Vec3 quat_rotate_vec3(Quat q, Vec3 v) {
    Vec3 u  = {{q.x, q.y, q.z}};
    f32  s  = q.w;
    Vec3 r  = vec3_add(
                vec3_add(vec3_scale(u, 2.0f * vec3_dot(u, v)),
                         vec3_scale(v, s*s - vec3_dot(u, u))),
                vec3_scale(vec3_cross(u, v), 2.0f * s));
    return r;
}