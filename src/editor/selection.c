#include "selection.h"
#include <string.h>
#include <float.h>
#include <math.h>

void selection_init(SelectionState *s) { s->selected_id = SELECTION_NONE; }
void selection_set(SelectionState *s, int id) { s->selected_id = id; }
void selection_clear(SelectionState *s) { s->selected_id = SELECTION_NONE; }
bool selection_has(const SelectionState *s) { return s->selected_id != SELECTION_NONE; }
int  selection_get(const SelectionState *s) { return s->selected_id; }

Ray ray_from_screen(Mat4 inv_vp, f32 sx, f32 sy, f32 vw, f32 vh) {
    f32 nx =  (2.0f * sx / vw) - 1.0f;
    f32 ny = -(2.0f * sy / vh) + 1.0f;   /* flip: screen Y-down → NDC Y-up */

    Vec4 near_clip = {{nx, ny, 0.0f, 1.0f}};
    Vec4 far_clip  = {{nx, ny, 1.0f, 1.0f}};

    Vec4 near_w = mat4_mul_vec4(inv_vp, near_clip);
    Vec4 far_w  = mat4_mul_vec4(inv_vp, far_clip);

    Vec3 near_p = vec3(near_w.x/near_w.w, near_w.y/near_w.w, near_w.z/near_w.w);
    Vec3 far_p  = vec3(far_w.x/far_w.w,   far_w.y/far_w.w,   far_w.z/far_w.w);

    return (Ray){ near_p, vec3_norm(vec3_sub(far_p, near_p)) };
}

bool ray_intersect_sphere(Ray r, Vec3 c, f32 radius, f32 *out_t) {
    Vec3 oc = vec3_sub(r.origin, c);
    f32  a  = vec3_dot(r.dir, r.dir);
    f32  b  = 2.0f * vec3_dot(oc, r.dir);
    f32  cv = vec3_dot(oc, oc) - radius * radius;
    f32  d  = b*b - 4.0f*a*cv;
    if (d < 0.0f) return false;
    *out_t = (-b - sqrtf(d)) / (2.0f * a);
    return *out_t >= 0.0f;
}

bool ray_intersect_aabb(Ray r, Vec3 mn, Vec3 mx, f32 *out_t) {
    f32 tmin = -FLT_MAX, tmax = FLT_MAX;
    for (int i = 0; i < 3; i++) {
        f32 inv = 1.0f / r.dir.v[i];
        f32 t0  = (mn.v[i] - r.origin.v[i]) * inv;
        f32 t1  = (mx.v[i] - r.origin.v[i]) * inv;
        if (inv < 0.0f) { f32 tmp = t0; t0 = t1; t1 = tmp; }
        if (t0 > tmin) tmin = t0;
        if (t1 < tmax) tmax = t1;
        if (tmax < tmin) return false;
    }
    *out_t = tmin >= 0.0f ? tmin : tmax;
    return *out_t >= 0.0f;
}