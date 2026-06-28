#include "viewport_camera.h"
#include <math.h>

void viewport_camera_init(ViewportCamera *cam) {
    cam->focus             = vec3(0.0f, 0.0f, 0.0f);
    cam->distance          = 5.0f;
    cam->pitch             = 0.4f;
    cam->yaw               = 0.8f;
    cam->fov_y             = 60.0f * GW_DEG2RAD;
    cam->near_z            = 0.1f;
    cam->far_z             = 1000.0f;
    cam->orbit_sensitivity = 0.005f;
    cam->pan_sensitivity   = 0.002f;
    cam->zoom_sensitivity  = 0.1f;
}

Vec3 viewport_camera_position(const ViewportCamera *cam) {
    f32 cp = cosf(cam->pitch), sp = sinf(cam->pitch);
    f32 cy = cosf(cam->yaw),   sy = sinf(cam->yaw);
    Vec3 offset = vec3(cy * cp, sp, sy * cp);
    return vec3_add(cam->focus, vec3_scale(offset, cam->distance));
}

void viewport_camera_update(ViewportCamera *cam, const InputState *in,
                             f32 dt, bool hovered) {
    (void)dt;
    if (!hovered) return;

    bool rmb = input_button_down(in, GW_MOUSE_RIGHT);
    bool mmb = input_button_down(in, GW_MOUSE_MIDDLE);
    f32  dx  = (f32)in->mouse_dx;
    f32  dy  = (f32)in->mouse_dy;

    if (rmb) {
        cam->yaw   += dx * cam->orbit_sensitivity;
        cam->pitch -= dy * cam->orbit_sensitivity;
        if (cam->pitch >  1.5f) cam->pitch =  1.5f;
        if (cam->pitch < -1.5f) cam->pitch = -1.5f;
    }

    if (mmb) {
        Vec3 pos   = viewport_camera_position(cam);
        Vec3 fwd   = vec3_norm(vec3_sub(cam->focus, pos));
        Vec3 world_up = vec3(0.0f, 1.0f, 0.0f);
        Vec3 right = vec3_norm(vec3_cross(fwd, world_up));
        Vec3 up    = vec3_cross(right, fwd);
        f32  s     = cam->distance * cam->pan_sensitivity;
        cam->focus = vec3_add(cam->focus, vec3_scale(right, -dx * s));
        cam->focus = vec3_add(cam->focus, vec3_scale(up,     dy * s));
    }

    if (in->scroll_delta) {
        cam->distance -= (f32)in->scroll_delta * cam->zoom_sensitivity * cam->distance;
        if (cam->distance < 0.1f) cam->distance = 0.1f;
    }
}

void viewport_camera_get_vp(const ViewportCamera *cam, f32 aspect,
                              Mat4 *out_view, Mat4 *out_proj) {
    Vec3 pos      = viewport_camera_position(cam);
    Vec3 world_up = vec3(0.0f, 1.0f, 0.0f);
    *out_view = mat4_look_at(pos, cam->focus, world_up);

    *out_proj = mat4_perspective(cam->fov_y, aspect, cam->near_z, cam->far_z);
    out_proj->m[1][1] = -out_proj->m[1][1];   /* flip Y for Vulkan clip space */
}